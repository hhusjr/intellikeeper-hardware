/* manually include protocol_arduino.cpp here */

#include <EEPROM.h>

const uint32_t base_id = 0x00010203;
const uint8_t      reader_id = 0x00;

#define EEPROM_ADDRESS_LABELS_BEGIN 0x160
#define EEPROM_ADDRESS_LABELS_MAX   5 * 30
#define ACK_BYTE 0x32

/*
 * 主控节点
 * （1）监听基站请求
 * （2）在基站的请求下
 *      a、主动阅读EEPROM中存储的标签
 *      b、在EEPROM内存储标签的地址信息，并且在基站请求的情况下不断更新
 */

void callback(uint8_t* response_buf, uint8_t &len, request* req) {
    /*
     * 基站请求格式：
     * 基站ID（4字节）+主控节点ID（1字节）+指令码（1字节）+操作数
     * 0123 4 5 ...
     * xxxx y y o
     */
    if (req->len < 6) {
        // 表示出现指令格式错误
        response_buf[0] = 0xFF;
        len = 1;
        return;
    }

    // 校验基站ID
    uint32_t* _base_id = (uint32_t*) req->body; // 一次读取四个字节，作为一个uint32_t

    if (*_base_id != base_id) {
        // 表示出现基站ID不匹配错误
        response_buf[0] = 0xFE;
        len = 1;
        return;
    }

    // 校验主控节点ID
    if (req->body[4] != reader_id) {
        // 表示出现主控节点ID不匹配错误
        response_buf[0] = 0xFD;
        len = 1;
        return;
    }

    // 指令码
    uint8_t op = req->body[5];
    // 主体首地址
    uint8_t* container = req->body + 6;
    uint8_t container_len = req->len - 6;
    // 当前考虑的位置
    len = 0;
    response_buf[len++] = 0x00; // 第一位0x00表示请求合法
    switch (op) {
        /*
         * 打招呼
         */
        case 0x00: {
            /*
             * !!! INSERT
                pinMode(2, OUTPUT);
                digitalWrite(2, HIGH);
                in setup() function!!!!!!!!
             */
            for (int i = 0; i < 4; i++) {
                digitalWrite(2, LOW);
                delay(150);
                digitalWrite(2, HIGH);
                delay(75);
            }
            digitalWrite(2, HIGH);
            response_buf[len++] = 0;
            return;
        }
        /*
         * 主动阅读标签
         * 操作数：基站给出的需要阅读的标签地址
         * 返回：按照请求给出的顺序逐个返回信号强度RSSI值的绝对值。如果当前标签不可达，则该值为0xFF
         */
        case 0x01: {
            // 对于每一个标签地址
            for (uint8_t offset = 0; offset < container_len; offset++) {
                uint8_t addr = container[offset];
                // 发送阅读请求
                // send_request -> response *resp, uint8_t addr, uint8_t *data, uint8_t len, uint8_t retries = 3, int wait_for_res_time_limit = 2000
                auto resp = new struct response;
                // 这里申请局部数组，保存在栈区，不用释放内存
                uint8_t data[5];
                // 基站地址
                memcpy(data, &base_id, 4); // 拷贝四个字节到data
                // 指令码
                data[4] = 0x01;
                // 发送请求，每个标签最多等待100ms
                send_request(resp, addr, data, sizeof(data), 3, 100);
                // 与标签无法联通，或者标签不合法情况
                if (!resp->is_available() || resp->len != 6 || resp->body[0] != 0x00 || resp->body[1] != ACK_BYTE) {
                    response_buf[len++] = 0xFF;
                    delete resp;
                    continue;
                }
                // 与标签可以联通的情况
                response_buf[len++] = (uint8_t) (-resp->rssi_dbm);
                delete resp;
            }
            return;
        }

        /*
         * 增量更新标签
         * 操作数：第一个字节为0，表示删除某个ID的标签。第一个字节非0，表示新增某个ID的标签。需要给出新增标签的ID与地址。
         * 返回：在基站的请求下，更新EEPROM中的标签信息。如果不合法的请求，则返回0xFF，如果删除不存在的标签或者新增已有标签，返回0xFE，如果操作成功，返回0x00
         */
        case 0x02: {
            // 首先校验该请求是否合法
            if (container_len < 5) {
                response_buf[len++] = 0xFF;
                return;
            }
            uint32_t target = *(uint32_t*) (container + 1);
            // 首先查找该标签是否存在以及位置
            uint8_t cnt = (uint8_t) EEPROM.read(EEPROM_ADDRESS_LABELS_BEGIN);
            int tar_off = -1;
            for (int offset = 1; offset <= 5 * cnt; offset += 5) {
                // 获取标签ID（四个部分） hijl
                uint32_t tag_real_id_l = (uint32_t) EEPROM.read(EEPROM_ADDRESS_LABELS_BEGIN + offset + 1);
                uint32_t tag_real_id_j = (uint32_t) EEPROM.read(EEPROM_ADDRESS_LABELS_BEGIN + offset + 2);
                uint32_t tag_real_id_i = (uint32_t) EEPROM.read(EEPROM_ADDRESS_LABELS_BEGIN + offset + 3);
                uint32_t tag_real_id_h = (uint32_t) EEPROM.read(EEPROM_ADDRESS_LABELS_BEGIN + offset + 4);
                uint32_t tag_real_id = (tag_real_id_h << 24) + (tag_real_id_i << 16) + (tag_real_id_j << 8) + tag_real_id_l;
                if (tag_real_id == target) {
                    tar_off = offset;
                    break;
                }
            }
            uint8_t action = container[0];
            switch (action) {
                // 删除操作
                case 0x00: {
                    // 不存在该ID
                    if (tar_off == -1) {
                        response_buf[len++] = 0xFE;
                        return;
                    }
                    // 删除，后面的往前移动即可
                    for (int offset = tar_off + 5; offset <= 5 * cnt; offset += 5) {
                        for (int i = 0; i < 5; i++) {
                            uint8_t replacement = EEPROM.read(EEPROM_ADDRESS_LABELS_BEGIN + offset + i);
                            EEPROM.write(EEPROM_ADDRESS_LABELS_BEGIN + offset + i - 5, replacement);
                        }
                    }
                    // 总数-1
                    EEPROM.write(EEPROM_ADDRESS_LABELS_BEGIN, cnt - 1);
                    // 成功
                    response_buf[len++] = 0x00;
                    // 输出
                    Serial.println("DELETE Tag, current config: ");
                    for (int i = 0; i <= cnt * 5 - 5; i++) {
                        cc1100.uart_puthex_byte(EEPROM.read(EEPROM_ADDRESS_LABELS_BEGIN + i));
                        Serial.print(" ");
                    }
                    Serial.println();
                    return;
                }

                // 添加操作
                case 0x01: {
                    // 必须给定新地址
                    if (container_len < 6) {
                        response_buf[len++] = 0xFF;
                        return;
                    }
                    // 已经存在该ID
                    if (tar_off != -1) {
                        response_buf[len++] = 0xFE;
                        return;
                    }
                    // 个数已满
                    if (cnt == EEPROM_ADDRESS_LABELS_MAX) {
                        response_buf[len++] = 0xFD;
                        return;
                    }
                    // 在末尾添加
                    EEPROM.write(EEPROM_ADDRESS_LABELS_BEGIN + 1 + 5 * cnt, container[5]);
                    for (int i = 1; i <= 4; i++) {
                        EEPROM.write(EEPROM_ADDRESS_LABELS_BEGIN + 1 + 5 * cnt + i, container[i]);
                    }
                    // 总数+1
                    EEPROM.write(EEPROM_ADDRESS_LABELS_BEGIN, cnt + 1);
                    // 成功
                    response_buf[len++] = 0x00;
                    // 输出
                    Serial.println("ADDED Tag, current config: ");
                    for (int i = 0; i <= 5 + cnt * 5; i++) {
                        cc1100.uart_puthex_byte(EEPROM.read(EEPROM_ADDRESS_LABELS_BEGIN + i));
                        Serial.print(" ");
                    }
                    Serial.println();
                    return;
                }

                // 清空配置
                case 0x02: {
                    // 总数+1
                    EEPROM.write(EEPROM_ADDRESS_LABELS_BEGIN, 0);
                    // 成功
                    response_buf[len++] = 0x00;
                    // 输出
                    Serial.println("CLEARED Tag, current config: ");
                    for (int i = 0; i <= 0; i++) {
                        cc1100.uart_puthex_byte(EEPROM.read(EEPROM_ADDRESS_LABELS_BEGIN + i));
                        Serial.print(" ");
                    }
                    Serial.println();
                    return;
                }
            }
            // 返回0xFF表示操作非法
            response_buf[len++] = 0xFF;
            return;
        }

        // 按遍历顺序获取所有地址，无操作数
        case 0x03: {
            for (int offset = 1; offset <= 5 * ((int) EEPROM.read(EEPROM_ADDRESS_LABELS_BEGIN)); offset += 5) {
                // 获取标签地址（1个字节）
                uint8_t addr = (uint8_t) EEPROM.read(EEPROM_ADDRESS_LABELS_BEGIN + offset);
                response_buf[len++] = addr;
            }
            return;
        }

        default:
            break;
    }
    // 指令码错误
    response_buf[len++] = 0xFC;
    return;
}
