//
// Created by 沈俊儒 on 2020/5/9.
//
#include <map>

#ifndef INTELLIKEEPER_BASE_MANAGER_H
#define INTELLIKEEPER_BASE_MANAGER_H

#define MAX_N_READER 10

struct config_table_item {
    uint8_t addr = 0xFF;
    bool is_active = true;

    uint8_t n_dis_measurement = 0, dis_measurement[MAX_N_READER]{};

    config_table_item(uint8_t addr, bool isActive);
    config_table_item();
};
static std::map<uint32_t, config_table_item> config_table;
static std::map<uint8_t, uint32_t> addr2id;

class ConfigManager {
public:
    static void check_config();
    static void attempt_resolve_delta();
};

class TagManager {
public:
    static uint32_t detect_new();
    static bool set_addr(uint8_t new_addr);
    static bool reset_addr(uint8_t addr);
    static void handshake(uint8_t addr);
};

class ReaderManager {
public:
    static response* read_tag(uint8_t reader, uint8_t reader_addr, uint8_t* addrs, uint8_t addrs_len);
    static void read_tags();
    static bool clear_config();
    static void handshake(uint8_t reader, uint8_t addr);
    static response* fetch_address(int reader_idx);
    static bool new_tag(uint32_t tag_id, uint8_t addr);
    static bool remove_tag(uint32_t tag_id);
};

#endif //INTELLIKEEPER_BASE_MANAGER_H
