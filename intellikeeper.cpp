#include <protocol.h>
#include <manager.h>
#include "manager.cpp"
#include <cstdio>

#define OUT_FILE_PATH "/home/pi/data/.base.output"

void callback(uint8_t *resp, uint8_t &len, request *) {
    printf("cb");
    len = 0;
    resp[len++] = 1;
}

int main() {
    int action;
    scanf("%d", &action);

    switch (action) {
        case -1: {
            printf("Work as a listener\n");
            protocol.listen(callback);
            break;
        }
        case 0: {
            printf("Reader Handshake\n");
            int reader, addr;
            scanf("%d %d", &reader, &addr);
            ReaderManager::handshake((uint8_t) reader, (uint8_t) addr);
            break;
        }
        case 1: {
            printf("New tag\n");
            uint32_t new_id = TagManager::detect_new();
            if (new_id == 0xffffffff) {
                break;
            }
            FILE* fd = fopen(OUT_FILE_PATH, "w");
            fprintf(fd, "%08x", new_id);
            fclose(fd);
            printf("%08x\n", new_id);
            break;
        }
        case 2: {
            printf("Tag handshake\n");
            int addr;
            scanf("%d", &addr);
            TagManager::handshake((uint8_t) addr);
            break;
        }
        case 3: {
            printf("Tag address create\n");
            int addr;
            scanf("%d", &addr);
            TagManager::set_addr((uint8_t) addr);
            break;
        }
        case 4: {
            printf("Tag read\n");
            int reader, reader_addr, addrs_len;
            scanf("%d %d %d", &reader, &reader_addr, &addrs_len);

            auto addrs = new uint8_t[addrs_len];
            for (int i = 0; i < addrs_len; i++) {
                int x;
                scanf("%d", &x);
                addrs[i] = (uint8_t) x;
            }
            
            auto res = ReaderManager::read_tag((uint8_t) reader, (uint8_t) reader_addr, addrs, (uint8_t) addrs_len);
            if (res == nullptr) {
                delete[] addrs;
                break;
            }
            
            FILE* fd = fopen(OUT_FILE_PATH, "w");
            int len = (int) (res->len - 1);
            fprintf(fd, "%d ", len);
            printf("%d ", len);
            for (int i = 0; i < len; i++) {
                fprintf(fd, "%d ", (int) res->body[i + 1]);
                printf("%d ", (int) res->body[i + 1]);
            }
            printf("\n");
            fclose(fd);

            delete [] addrs;
            delete res;

            break;
        }
        default:
            break;
    }

    return 0;
}
