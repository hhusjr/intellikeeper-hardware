/* manually include protocol_arduino.cpp here */

const uint32_t base_id  = 0x00010203;
const uint32_t tag_id = 0x00000001;

#define ACK_BYTE 0x32

/*
 * 标签
 * （1）监听基站或者主控节点请求
 * （2）在基站的请求下，更改自身的地址，或者将自身复位
 * （3）在主控节点的请求下，响应主控节点的阅读请求
 */

void callback(uint8_t* response_buf, uint8_t &len, request* req) {
    /*
     * 请求格式：
     * 基站ID（4字节）+指令码（1字节）+操作数
     */
    if (req->len < 5) {
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

    // 指令码
    uint8_t op = req->body[4];
    // 主体首地址
    uint8_t* container = req->body + 5;
    uint8_t container_len = req->len - 5;
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
            for (int i = 0; i < 2; i++) {
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
         * 响应阅读信息
         * 操作数：无
         * 返回：如果成功，则ACK_BYTE加该标签自身的ID
         */
        case 0x01: {
            response_buf[len++] = ACK_BYTE;
            memcpy(response_buf + 2, &tag_id, 4);
            len += 4;
            return;
        }

        /*
         * 修改自身的地址
         * 操作数：一个字节，新地址
         * 返回：如果操作成功，返回0x00，否则0xFF
         */
        case 0x02: {
            if (container_len != 1) {
                response_buf[len++] = 0xFF;
                return;
            }
            cc1100.set_myaddr(container[0]);
            response_buf[len++] = 0x00;
            return;
        }

        /*
         * 地址复位
         * 操作数：无
         * 返回：如果操作成功，返回0x00
         */
        case 0xFE: {
            cc1100.set_myaddr(0xFE);
            response_buf[len++] = 0x00;
            return;
        }

        default:
            break;
    }
    // 指令码错误
    response_buf[len++] = 0xFC;
    return;
}
