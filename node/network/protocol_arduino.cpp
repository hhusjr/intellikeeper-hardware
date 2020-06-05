#include <avr/sleep.h>
#include <cc1100_arduino.h>
#include <EnableInterrupt.h>

volatile int sleep_enable = 0;
uint32_t rf_timecode = 0;
uint32_t rf_timecode_backup = 0;

/*
 * ATTENTION!!!
 * 这里全都是全局变量，随时可能都会被修改！！！
 * 如果需要连续多次receieve（比如第一次receieve后再receieve一次，然后想访问第一次receieve的结果，注意及时memcpy到自己的变量中去！！！）
 */
uint8_t tx_fifo[FIFOBUFFER], rx_fifo[FIFOBUFFER], response_buf[FIFOBUFFER];
uint8_t My_addr, pktlen; // My_addr由驱动程序自动更新
uint8_t rx_addr, sender, lqi;
int8_t rssi_dbm;
volatile uint8_t cc1101_packet_available;
uint8_t key = 0x97;
uint8_t retries = 3;

CC1100 cc1100;

#define MAX_KEY_POOL_SIZE 64
struct key_pool_item {
    key_pool_item* next = nullptr;
    uint32_t val = 0;
};
struct {
    key_pool_item* key_pool_header = nullptr;
    key_pool_item* key_pool_tail = nullptr;
    int pool_size = 0;

    int size() {
        return pool_size;
    }

    void emplace_back(uint32_t key) {
        auto item = new key_pool_item;
        item->val = key;
        if (key_pool_header == nullptr) {
            key_pool_header = item;
            key_pool_tail = item;
        } else {
            key_pool_tail->next = item;
            key_pool_tail = item;
        }
        pool_size++;
    }

    bool contains(uint32_t key) {
        auto p = key_pool_header;
        while (p != nullptr) {
            if (p->val == key) {
                return true;
            }
            p = p->next;
        }
        return false;
    }

    void pop_front() {
        if (key_pool_header == nullptr) {
            return;
        }
        auto tmp = key_pool_header;
        key_pool_header = key_pool_header->next;
        delete tmp;
        pool_size--;
    }
} key_pool;

struct request {
    uint8_t* body = nullptr;
    uint8_t len = -1;

    ~request() {
        delete[] body;
    }
};

struct response {
    uint8_t* body = nullptr;
    int8_t rssi_dbm = 0;
    uint8_t len = -1;
    uint8_t request_from = -1;

    bool is_available() {
        return body != nullptr;
    }

    ~response() {
        delete[] body;
    }
};

void write_hex_arr(uint8_t* a, int len) {
    for (int i = 0; i < len; i++) {
        cc1100.uart_puthex_byte(a[i]);
        Serial.print(F(" "));
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    cc1100.begin(My_addr);

    cc1100.sidle();

    // 配置值
    cc1100.set_mode(0x04);
    cc1100.set_ISM(0x02);
    cc1100.set_channel(0x01);
    cc1100.set_output_power_level(0);
    cc1100.set_myaddr(0xFE);

    cc1100.spi_write_register(IOCFG2, 0x06);

    cc1100.show_main_settings();
    cc1100.show_register_settings();

    cc1100.receive();

    enableInterrupt(GDO2, rf_available_int, RISING);

    Serial.println(F("Intellikeeper on Arduino"));

    randomSeed(analogRead(0));
}

void callback(uint8_t* response_buf, uint8_t &len, request* req);

void send_request(response *resp, uint8_t addr, uint8_t *data, uint8_t len, uint8_t retries = 3, int wait_for_res_time_limit = 2000) {
    len += 3; // 开头的3个字节
    len += 6; // 后续6个请求描述字节
    auto *req_content = new uint8_t[len];
    uint8_t rkey[4];
    // 跳过开头两个字节
    uint8_t *req_header = req_content + 3;
    // 前两字节魔数，表示ik request
    *req_header = 0x69;
    *(req_header + 1) = 0x6b;
    // 然后四个字节的请求密钥
    for (int p = 2; p < 6; p++) {
        *(req_header + p) = rkey[p - 2] = random(0xFF); // NOLINT
    }
    // 剩余为Body
    memcpy(req_header + 6, data, len - 9);

    Serial.println(F("Ready to send request, expected request payload:"));
    write_hex_arr(req_content + 3, len - 3);
    Serial.println();

    // 加密除了魔数以外的部分
    for (int i = 5; i < len; i++) {
        *(req_content + i) ^=
                key;
    }

    Serial.println(F("Encoded payload:"));
    write_hex_arr(req_content + 3, len - 3);
    Serial.println();

    disableInterrupt(GDO2);
    uint8_t res = cc1100.sent_packet(My_addr, addr, req_content, len, retries);
    enableInterrupt(GDO2, rf_available_int, RISING);

    delete[] req_content;
    if (res != 1) {
        return;
    }
    Serial.println(F("Waiting for response..."));
    Serial.println();

    int wait_time = 0;
    while (true) {
        delay(1);
        if (wait_time++ > wait_for_res_time_limit) {
            Serial.println(F("Time limit exceeded."));
            Serial.println();
            return;
        }

        if (cc1101_packet_available == TRUE) {
            uint8_t res_len = pktlen;
            cc1101_packet_available = FALSE;
            resp->request_from = sender;
            resp->rssi_dbm = rssi_dbm;
            res_len++;
            if (res_len < 9) {
                return;
            }
            uint8_t *res_header = rx_fifo + 3;

            // 首先考察响应addr是否为请求的addr
            if (resp->request_from != addr) {
                continue;
            }

            // 响应的模数为大写的IK，如果不是则说明不是响应
            if (*res_header != 0x49 || *(res_header + 1) != 0x4b) {
                continue;
            }

            Serial.println(F("Got response payload:"));
            write_hex_arr(rx_fifo + 3, res_len - 3);
            Serial.println();

            // 对剩余内容解密
            for (int i = 5; i < res_len; i++) {
                *(rx_fifo + i) ^= key;
            }

            Serial.println(F("Decoded response payload:"));
            write_hex_arr(rx_fifo + 3, res_len - 3);
            Serial.println();

            Serial.println(F("Inspecting key..."));
            Serial.println();

            // 校验响应密钥
            bool is_correct_key = true;
            for (int i = 0; i < 4; i++) {
                if (rkey[i] != *(res_header + 2 + i)) {
                    is_correct_key = false;
                    break;
                }
            }
            if (!is_correct_key) {
                Serial.println(F("Invalid response key..."));
                Serial.println();
                continue;
            }
            // 返回剩余部分
            resp->len = res_len - 9;
            resp->body = new uint8_t[resp->len];
            memcpy(resp->body, res_header + 6, resp->len);

            Serial.println(F("Success. Response:"));
            write_hex_arr(resp->body, resp->len);
            Serial.println();

            return;
        }
    }
}

void loop() {
    // 进入监听模式
    if (cc1101_packet_available == TRUE) {
        cc1101_packet_available = FALSE;
        // 跳过前三位

        // !!!!!!!!!!!!!!
        int cur_pktlen = pktlen, cur_sender = sender;
        auto cur_rx_fifo = new uint8_t[cur_pktlen + 1];
        memcpy(cur_rx_fifo, rx_fifo, cur_pktlen + 1);
        // !!!!!!!!!!!!!!

        uint8_t *req_header = cur_rx_fifo + 3;

        // 考察模数是否为ik request对应的模数
        if (*req_header != 0x69 || *(req_header + 1) != 0x6b) {
            delete[] cur_rx_fifo; // DO NOT FORGET
            return;
        }

        Serial.println(F("Got request payload:"));
        write_hex_arr(cur_rx_fifo + 3, cur_pktlen - 2);
        Serial.println();

        // 解密
        for (int i = 5; i <= cur_pktlen; i++) {
            *(cur_rx_fifo + i) ^= key;
        }

        Serial.println(F("Decoded request payload:"));
        write_hex_arr(cur_rx_fifo + 3, cur_pktlen - 2);
        Serial.println();

        // 校验Key一段时期内的唯一性
        uint32_t cur_key = *(uint32_t*) (cur_rx_fifo + 5);
        if (key_pool.contains(cur_key)) {
            Serial.println("!!!DUPLICATE REQUEST!!!\n");
            delete[] cur_rx_fifo; // DO NOT FORGET
            return;
        }
        if (key_pool.size() >= MAX_KEY_POOL_SIZE) {
            key_pool.pop_front();
        }
        key_pool.emplace_back(cur_key);

        // 调用回调函数
        auto req = new struct request;
        req->len = cur_pktlen - 8;
        req->body = new uint8_t[req->len];
        memcpy(req->body, req_header + 6, req->len);

        uint8_t res_len = -1;

        Serial.println(F("Calling callback function..."));
        Serial.println();

        callback(response_buf, res_len, req);

        delete req;

        if (res_len < 0) {
            Serial.println(F("Invalid response."));
            Serial.println();
            delete[] cur_rx_fifo; // DO NOT FORGET
            return;
        }

        Serial.println(F("Preparing for sending response."));
        Serial.println();

        // 响应
        int final_res_len = res_len + 9;
        auto res = new uint8_t[final_res_len];
        auto res_header = res + 3;
        // 前两字节魔数，表示IK response
        *res_header = 0x49;
        *(res_header + 1) = 0x4b;
        // 然后是四个字节的Key
        memcpy(res_header + 2, req_header + 2, 4);
        // 然后是剩余内容
        memcpy(res_header + 6, response_buf, res_len);

        Serial.println(F("Response payload:"));
        write_hex_arr(res + 3, final_res_len - 3);
        Serial.println();

        // 加密
        for (int i = 5; i < final_res_len; i++) {
            *(res + i) ^= key;
        }

        Serial.println(F("Encoded response payload:"));
        write_hex_arr(res + 3, final_res_len - 3);
        Serial.println();

        // 发出
        disableInterrupt(GDO2);
        uint8_t response_ok = cc1100.sent_packet(My_addr, cur_sender, res, final_res_len, retries);
        enableInterrupt(GDO2, rf_available_int, RISING);
        if (response_ok != 1) {
            delete[] res;
            delete[] cur_rx_fifo; // DO NOT FORGET
            return;
        }

        rf_timecode_backup = millis();
        delete[] res;
        delete[] cur_rx_fifo; // DO NOT FORGET
    }

    // 5s没有接收到新信息就进入睡眠状态
    if (millis() - rf_timecode_backup > 5000) {
        rf_timecode_backup = millis();

        sleep_enable = 1;
        Serial.println(F("Sleep MCU"));
        delay(100);
        set_sleep_mode(SLEEP_MODE_STANDBY);
        cli();
        sleep_enable();
        sei();
        sleep_cpu();
        sleep_disable();

        enableInterrupt(GDO2, rf_available_int, RISING);
    }
}

void rf_available_int(void) {
    disableInterrupt(GDO2);
    sei();
    uint32_t time_stamp = millis();

    sleep_disable();
    sleep_enable = 0;

    if (cc1100.packet_available() == TRUE) {
        if (cc1100.get_payload(rx_fifo, pktlen, rx_addr, sender, rssi_dbm, lqi) == TRUE) {
            cc1101_packet_available = TRUE;
        } else {
            cc1101_packet_available = FALSE;                               //set flag that an package is corrupted
        }
    }

    enableInterrupt(GDO2, rf_available_int, RISING);
}