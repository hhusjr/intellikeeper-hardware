//
// Created by 沈俊儒 on 2020/5/7.
//

#ifndef INTELLIKEEPER_BASE_CC1101_NETWORK_RASPI_H
#define INTELLIKEEPER_BASE_CC1101_NETWORK_RASPI_H

#include <cstdint>
#include "cc1100_raspi.h"

enum nw_freq {
    MHZ_315 = 1,
    MHZ_433,
    MHZ_868,
    MHZ_915
};

enum nw_mode {
    GFSK_1_2_KB = 1,
    GFSK_38_4_KB,
    GFSK_100_KB,
    MSK_250_KB,
    MSK_500_KB,
    OOK_4_8_KB
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

struct request {
    uint8_t* body = nullptr;
    uint8_t len = -1;

    ~request() {
        delete[] body;
    }
};

class Protocol {
private:
    CC1100 cc1100;
    uint8_t my_addr;
    uint8_t key{};
    uint8_t rx_fifo[FIFOBUFFER]{};

public:
    uint8_t response_buf[FIFOBUFFER]{};

public:
    explicit Protocol(uint8_t my_addr, nw_mode mode, nw_freq freq, int channel, uint8_t rkey);
    void request(response *resp, uint8_t addr, uint8_t *data, uint8_t len, uint8_t retries = 3, int wait_for_res_time_limit = 2000);
    void listen(void callback(uint8_t* response, uint8_t& len, struct request*), uint8_t retries = 3);
};

#endif //INTELLIKEEPER_BASE_CC1101_NETWORK_RASPI_H
