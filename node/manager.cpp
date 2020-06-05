//
// Created by 沈俊儒 on 2020/5/9.
//

#include <protocol.h>
#include <manager.h>
#include <cstdio>
#include <fcntl.h>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/stat.h>
#include "unistd.h"
#include <set>
#include <vector>
#include <cstring>

#define DEBUG

#define BASE_ADDR 0x01
#define NW_MODE MSK_250_KB
#define NW_FREQ MHZ_433
#define CHANNEL 1
#define RKEY 0x97
#define TAG_ADDR_OFFSET 0x20
#define MAX_TAG_ADDR 0xFD
#define NEW_TAG_ADDR 0xFE

#define CONFIG_FILE_NEW "/home/pi/svr/.intellikeeperc_upd"
#define CONFIG_FILE "/home/pi/svr/.intellikeeperc"

const uint32_t base_id      = 0x00010203;
const uint8_t  readers[][2] = {
        // id, addr
        {0x00, 0x02},
        {0x01, 0x03},
};
#define N_READER sizeof(readers) / 2

Protocol protocol(BASE_ADDR, NW_MODE, NW_FREQ, CHANNEL, RKEY); // NOLINT
bool config_loaded = false;

void ConfigManager::check_config() {
    if (!config_loaded) {
        int fd = open(CONFIG_FILE, O_RDONLY); // NOLINT

        if (fd == -1) {
            attempt_resolve_delta();
            return;
        }

        struct stat st{};
        stat(CONFIG_FILE, &st);
        // 如果大小不是6的倍数，说明有问题
        if (st.st_size % 6) {
            close(fd);
            printf("Invalid file size.\n");
            abort();
        }

        // 配置映射到内存，内存自动创建和回收
        auto *buf = (uint8_t *) mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

        // 取出所有配置，并保存到配置表
        for (int i = 0; i < st.st_size; i += 6) {
            uint8_t addr = buf[i];
            uint8_t is_active = buf[i + 1];
            uint32_t id = *(uint32_t *) (buf + i + 2);

            config_table[id] = config_table_item(addr, (bool) is_active);
            addr2id[addr] = id;
        }

        munmap(buf, st.st_size);
        close(fd);
    }

    attempt_resolve_delta();
}

void ConfigManager::attempt_resolve_delta() {
    int fd = open(CONFIG_FILE_NEW, O_RDONLY); // NOLINT
    if (fd == -1) {
        // No delta
        return;
    }

    printf("!!! Resolving delta...\n");
    struct stat st{};
    stat(CONFIG_FILE_NEW, &st);
    // 如果大小不是5的倍数，说明有问题
    if (st.st_size % 5) {
        close(fd);
        printf("Invalid file size.\n");
        abort();
    }

    // 配置映射到内存，内存自动创建和回收
    auto *buf = (uint8_t *) mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    // 取出旧配置的所有地址，便于计算
    std::set<uint8_t> addrs;
    for (auto &addr : addr2id) {
        addrs.insert(addr.first);
    }

    int mex = TAG_ADDR_OFFSET - 1;

    std::set<uint8_t> ids;
    // 先考虑新增的ID的情况，或者更新is_active的情况
    for (int i = 0; i < st.st_size; i += 5) {
        uint8_t is_active = buf[i];
        uint32_t id = *(uint32_t*) (buf + i + 1);
        ids.insert(id);

        // 第一种情况：是新增加的ID
        if (!config_table.count(id)) {
            // 寻找下一个可用地址
            for (int t = mex + 1; t <= MAX_TAG_ADDR; t++) {
                if (!addrs.count(t)) {
                    mex = t;
                    break;
                }
            }
            // 发送请求，新增ID
            if (!TagManager::set_addr(mex)) {
                // 如果没有成功，直接跳过这个
                // 下次用户可以重新尝试
                continue;
            }
            config_table[id].addr = mex;
            addr2id[mex] = id;
        }

        // 更改is_active的情况
        config_table[id].is_active = is_active;
    }

    // 考虑删除的情况
    std::vector<uint32_t> removes;
    for (auto &conf : config_table) {
        if (!ids.count(conf.first)) {
            if (!TagManager::reset_addr(conf.second.addr)) {
                // 删除失败，等待用户下次重试
                continue;
            }
            removes.emplace_back(conf.first);
            addr2id.erase(conf.second.addr);
        }
    }
    for (auto &id : removes) {
        config_table.erase(id);
    }

    munmap(buf, st.st_size);
    close(fd);

    // 将增量更新后的config_table保存到旧配置文件
    unsigned int new_len = config_table.size() * 6;
    auto generated = new uint8_t[new_len];
    uint8_t* cur = generated;
    for (auto &conf : config_table) {
        *(cur++) = conf.second.addr;
        *(cur++) = (uint8_t) conf.second.is_active;
        memcpy(cur, &conf.first, 4);
        cur += 4;
    }
    // 重新创建旧配置文件后，用mmap映射写入
    fd = open(CONFIG_FILE, O_CREAT | O_TRUNC | O_RDWR); // NOLINT
    truncate(CONFIG_FILE, new_len);
    void *dst_ptr = mmap(nullptr, new_len, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0); // NOLINT
    memcpy(dst_ptr, generated, new_len);
    munmap(dst_ptr, new_len);
    // 释放内存
    delete[] generated;
    // 删除新文件
    remove(CONFIG_FILE_NEW);
}

config_table_item::config_table_item(uint8_t addr, bool isActive) : addr(addr), is_active(isActive) {}

config_table_item::config_table_item() = default;

uint32_t TagManager::detect_new() {
    uint8_t data[FIFOBUFFER];
    memcpy(data, &base_id, 4);
    data[4] = 0x01;

    auto resp = new struct response;
    protocol.request(resp, NEW_TAG_ADDR, data, 5, 3, 2000);
    if (!resp->is_available()) {
        delete resp;
        return (uint32_t) 0xffffffff;
    }
    if (resp->len != 6 || resp->body[0] != 0x00) {
        delete resp;
        return (uint32_t) 0xffffffff;
    }
    uint32_t res = *(uint32_t*) (resp->body + 2);
    delete resp;
    return res;
}

bool TagManager::reset_addr(uint8_t addr) {
    uint8_t data[FIFOBUFFER];
    memcpy(data, &base_id, 4);
    data[4] = 0xFE;

    auto resp = new struct response;
    protocol.request(resp, addr, data, 5, 3, 2000);
    if (!resp->is_available()) {
        delete resp;
        return false;
    }
    if (resp->len != 2) {
        delete resp;
        return false;
    }
    bool res = resp->body[0] == 0x00 && resp->body[1] == 0x00;
    delete resp;
    return res;
}

bool TagManager::set_addr(uint8_t new_addr) {
    uint8_t data[FIFOBUFFER];
    memcpy(data, &base_id, 4);
    data[4] = 0x02;
    data[5] = new_addr;

    auto resp = new struct response;
    protocol.request(resp, NEW_TAG_ADDR, data, 6, 3, 2000);
    if (!resp->is_available()) {
        delete resp;
        return false;
    }
    if (resp->len != 2) {
        delete resp;
        return false;
    }
    bool res = resp->body[0] == 0x00 && resp->body[1] == 0x00;
    delete resp;
    return res;
}

bool ReaderManager::new_tag(uint32_t tag_id, uint8_t addr) {
    uint8_t data[FIFOBUFFER];
    memcpy(data, &base_id, 4);
    bool result = true;
    // 通知每一个reader添加标签
    for (auto reader : readers) {
        data[4] = reader[0];
        data[5] = 0x02; // 增量更新
        data[6] = 0x01; // 新增标签
        memcpy(data + 7, &tag_id, 4);
        data[11] = addr;

        auto resp = new struct response;
        protocol.request(resp, reader[1], data, 12, 3, 3000);
        if (resp->len != 2 || resp->body[0] != 0x00 || (resp->body[1] != 0x00 && resp->body[1] != 0xFE)) {
            result = false;
        }
        delete resp;
    }
    return result;
}

bool ReaderManager::remove_tag(uint32_t tag_id) {
    uint8_t data[FIFOBUFFER];
    memcpy(data, &base_id, 4);
    bool result = true;
    // 通知每一个reader添加标签
    for (auto reader : readers) {
        data[4] = reader[0];
        data[5] = 0x02; // 增量更新
        data[6] = 0x00; // 删除标签
        memcpy(data + 7, &tag_id, 4);

        auto resp = new struct response;
        protocol.request(resp, reader[1], data, 11, 3, 2000);
        if (resp->len != 2 || resp->body[0] != 0x00 || (resp->body[1] != 0x00 && resp->body[1] != 0xFE)) {
            result = false;
        }
        delete resp;
    }
    return result;
}

bool ReaderManager::clear_config() {
    uint8_t data[FIFOBUFFER];
    memcpy(data, &base_id, 4);
    bool result = true;
    // 通知每一个reader添加标签
    for (auto reader : readers) {
        data[4] = reader[0];
        data[5] = 0x02; // 增量更新
        data[6] = 0x02; // 清空配置
        // 填充位置
        uint32_t fill = 0x00000000;
        memcpy(data + 7, &fill, 4);

        auto resp = new struct response;
        protocol.request(resp, reader[1], data, 11, 3, 5000);
        if (resp->len != 2 || resp->body[0] != 0x00 || resp->body[1] != 0x00) {
            result = false;
        }
        delete resp;
    }
    return result;
}

void ReaderManager::handshake(uint8_t reader, uint8_t addr) {
    uint8_t data[FIFOBUFFER];
    memcpy(data, &base_id, 4);
    // 通知每一个reader添加标签
    data[4] = reader;
    data[5] = 0x00; // 打招呼

    auto resp = new struct response;
    protocol.request(resp, addr, data, 6, 3, 5000);
    delete resp;
}

response* ReaderManager::read_tag(uint8_t reader, uint8_t reader_addr,  uint8_t* addrs, uint8_t addrs_len) {
    uint8_t data[FIFOBUFFER];
    memcpy(data, &base_id, 4);
    data[4] = reader;
    data[5] = 0x01;

    // 填充需求的地址
    printf("%d\n", addrs_len);
    memcpy(data + 6, addrs, addrs_len);

    auto resp = new response;
    protocol.request(resp, reader_addr, data, 6 + addrs_len, 3, 2000);
    if (!resp->is_available() || resp->len < 1 || resp->body[0] != 0x00) {
        delete resp;
        return nullptr;
    }
    return resp;
}

void ReaderManager::read_tags() {
    uint8_t data[FIFOBUFFER];
    memcpy(data, &base_id, 4);

    // 清空所有标签的距离测量值
    for (auto item : config_table) {
        item.second.n_dis_measurement = 0;
    }

    // 通知每一个reader添加标签
    for (int i = 0; i < N_READER; i++) {
        data[4] = readers[i][0];
        data[5] = 0x01; // 主动阅读标签

        std::vector<uint8_t> tag_addrs;

        // 获取地址顺序
        auto resp = fetch_address(i);
        if (!resp->is_available() || resp->body[0] != 0x00) {
            delete resp;
            continue;
        }
        for (int j = 1; j < resp->len; j++) {
            tag_addrs.emplace_back(resp->body[j]);
        }
        delete resp;

        // 获取RSSI值列表
        resp = new struct response;
        protocol.request(resp, readers[i][1], data, 6, 3, 2000);
        if (!resp->is_available() || resp->body[0] != 0x00) {
            delete resp;
            continue;
        }
        for (int j = 1; j < resp->len; j++) {
            if (!addr2id.count(tag_addrs[j - 1]) || !config_table.count(addr2id[tag_addrs[j - 1]]))  {
                continue;
            }
            auto &item = config_table[addr2id[tag_addrs[j - 1]]];
            item.dis_measurement[item.n_dis_measurement++] = resp->body[j];
            printf("d! %x %x\n", addr2id[tag_addrs[j - 1]], resp->body[j]);
        }
        delete resp;
    }
}

response* ReaderManager::fetch_address(int reader_idx) {
    uint8_t data[FIFOBUFFER];
    memcpy(data, &base_id, 4);
    data[4] = readers[reader_idx][0];
    data[5] = 0x03;

    auto resp = new struct response;
    protocol.request(resp, readers[reader_idx][1], data, 6, 3, 2000);
    return resp;
}

void TagManager::handshake(uint8_t addr) {
    uint8_t data[FIFOBUFFER];
    memcpy(data, &base_id, 4);
    // 通知每一个reader添加标签
    data[4] = 0x00; // 打招呼

    auto resp = new struct response;
    protocol.request(resp, addr, data, 5, 3, 5000);
    delete resp;
}