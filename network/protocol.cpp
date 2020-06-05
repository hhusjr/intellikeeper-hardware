//
// Created by 沈俊儒 on 2020/5/7.
//

#include <cstring>
#include "protocol.h"
#include "cc1100_raspi.h"
#include <cstdlib>
#include "wiringPi.h"
#include <random>

#define DEBUG

#ifdef DEBUG

#include <cstdio>
#include <ctime>

#endif

uint8_t My_addr; // NOLINT
int cc1100_freq_select, cc1100_mode_select, cc1100_channel_select; // NOLINT
int cc1100_debug;

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

Protocol::Protocol(uint8_t my_addr, nw_mode mode, nw_freq freq, int channel, uint8_t rkey) {
    cc1100_mode_select = (int) mode;
    cc1100_freq_select = (int) freq;
    cc1100_channel_select = channel;
    this->my_addr = my_addr;
    My_addr = my_addr;

    wiringPiSetup();
    cc1100.begin(My_addr);
    cc1100.sidle();
#ifdef DEBUG
    printf("Driver debug enabled.\n");
    cc1100_debug = true;
    printf("CC1100 Begin\n");
    printf("\n");
#endif

    cc1100.set_output_power_level(0);
    cc1100.receive();
    this->key = rkey;
    cc1100.show_main_settings();
    cc1100.show_register_settings();
#ifdef DEBUG
    printf("Established connection, switched to receive mode.\n");
    printf("\n");
#endif
}

void Protocol::request(response *resp, uint8_t addr, uint8_t *data, uint8_t len, uint8_t retries, int wait_for_res_time_limit) {
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
    std::random_device rd;
    for (int p = 2; p < 6; p++) {
        *(req_header + p) = rkey[p - 2] = rd() % 0xff;
    }
    // 剩余为Body
    memcpy(req_header + 6, data, len - 9);
#ifdef DEBUG
    printf("Ready to send request, expected request payload:\n");
    for (int i = 3; i < len; i++) {
        printf("%x ", *(req_content + i));
    }
    printf("\n");
#endif
    // 加密除了魔数以外的部分
    for (int i = 5; i < len; i++) {
        *(req_content + i) ^= this->key;
    }
#ifdef DEBUG
    printf("Encoded payload:\n");
    for (int i = 3; i < len; i++) {
        printf("%x ", *(req_content + i));
    }
    printf("\n");
#endif

    uint8_t res = cc1100.sent_packet(this->my_addr, addr, req_content, len, retries);

    delete[] req_content;
    if (res != 1) {
        return;
    }
    // 等待响应
#ifdef DEBUG
    printf("Waiting for response...\n");
    printf("\n");
#endif
    int wait_time = 0;
    while (true) {
        delay(1);
        if (wait_time++ > wait_for_res_time_limit) {
#ifdef DEBUG
            printf("Time limit exceeded.\n");
            printf("\n");
#endif
            return;
        }

        if (cc1100.packet_available()) {
            uint8_t res_len = -1, lqi;
            cc1100.get_payload(rx_fifo, res_len, this->my_addr, resp->request_from, resp->rssi_dbm, lqi);
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
#ifdef DEBUG
            printf("Got response payload, addr %x\n", resp->request_from);
            for (int i = 3; i < res_len; i++) {
                printf("%x ", *(rx_fifo + i));
            }
            printf("\n");
#endif
            // 对剩余内容解密
            for (int i = 5; i < res_len; i++) {
                *(rx_fifo + i) ^= this->key;
            }
#ifdef DEBUG
            printf("Decoded payload:\n");
            for (int i = 3; i < res_len; i++) {
                printf("%x ", *(rx_fifo + i));
            }
            printf("\n");
            printf("Inspecting key...\n");
            printf("\n");
#endif
            // 校验响应密钥
            bool is_correct_key = true;
            for (int i = 0; i < 4; i++) {
                if (rkey[i] != *(res_header + 2 + i)) {
                    is_correct_key = false;
                    break;
                }
            }
            if (!is_correct_key) {
#ifdef DEBUG
                printf("Invalid response key.\n");
                printf("\n");
#endif
                continue;
            }
            // 返回剩余部分
            resp->len = res_len - 9;
            resp->body = new uint8_t[resp->len];
            memcpy(resp->body, res_header + 6, resp->len);
#ifdef DEBUG
            printf("Success. Response: \n");
            for (int i = 0; i < resp->len; i++) {
                printf("%x ", resp->body[i]);
            }
            printf("\n");
#endif
            return;
        }
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

void Protocol::listen(void callback(uint8_t *response, uint8_t &len, struct request *), uint8_t retries) {
#ifdef DEBUG
    printf("Listener ready.\n");
    printf("\n");
#endif

    while (true) {
        delay(2);

        if (cc1100.packet_available()) {
            uint8_t req_len = -1, sender, lqi, request_from;
            int8_t rssi_dbm;
            cc1100.get_payload(rx_fifo, req_len, this->my_addr, request_from, rssi_dbm, lqi);

            uint8_t *req_header = rx_fifo + 3;
            if (*req_header != 0x69 || *(req_header + 1) != 0x6b) {
                continue;
            }
#ifdef DEBUG
            printf("Got request payload:\n");
            for (int i = 3; i < req_len + 1; i++) {
                printf("%x ", *(rx_fifo + i));
            }
            printf("\n");
#endif
            for (int i = 5; i < req_len + 1; i++) {
                *(rx_fifo + i) ^= this->key;
            }
#ifdef DEBUG
            printf("Decoded request payload:\n");
            for (int i = 3; i < req_len + 1; i++) {
                printf("%x ", *(rx_fifo + i));
            }
            printf("\n");
#endif

            // 校验Key一段时期内的唯一性
            uint32_t cur_key = *(uint32_t*) (rx_fifo + 5);
            if (key_pool.contains(cur_key)) {
#ifdef DEBUG
                printf("!!!DUPLICATE REQUEST!!!\n");
#endif
                return;
            }
            if (key_pool.size() >= MAX_KEY_POOL_SIZE) {
                key_pool.pop_front();
            }
            key_pool.emplace_back(cur_key);

            auto req = new struct request;
            req->len = req_len - 8;
            req->body = new uint8_t[req->len];
            memcpy(req->body, req_header + 6, req->len);

            uint8_t res_len = -1;

#ifdef DEBUG
            printf("Calling callback function...\n");
            printf("\n");
#endif
            callback(response_buf, res_len, req);

            delete req;

            if (res_len < 0) {
#ifdef DEBUG
                printf("Invalid response.\n");
                printf("\n");
#endif
                continue;
            }

#ifdef DEBUG
            printf("Preparing for sending response.\n");
            printf("\n");
#endif

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
#ifdef DEBUG
            printf("Response payload:\n");
            for (int i = 5; i < final_res_len; i++) {
                printf("%x ", *(res + i));
            }
            printf("\n");
#endif
            // 加密
            for (int i = 5; i < final_res_len; i++) {
                *(res + i) ^= this->key;
            }

#ifdef DEBUG
            printf("Encoded response payload:\n");
            for (int i = 3; i < final_res_len; i++) {
                printf("%x ", *(res + i));
            }
            printf("\n");
#endif
            // 发出
            uint8_t response_ok = cc1100.sent_packet(this->my_addr, request_from, res, final_res_len, retries);
            if (response_ok != 1) {
                continue;
            }
        }
    }
}

#pragma clang diagnostic pop
