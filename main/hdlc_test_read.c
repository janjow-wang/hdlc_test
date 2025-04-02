#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define HDLC_FLAG 0x7E
#define MAX_FRAME_SIZE 1024

// CRC-16-CCITT Polynomial (0x1021)
uint16_t crc16_ccitt(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc ^= (*data++) << 8;
        for (int i = 0; i < 8; i++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

// Bit Destuffing (移除比特填充)
size_t bit_destuff(uint8_t *dest, uint8_t *src, size_t len) {
    size_t dest_index = 0;
    int bit_count = 0;
    uint8_t current_byte = 0;
    int bit_pos = 0; // 當前處理的 bit 位置

    size_t i =0;
    int j = 0;
    bool bit = 0;
    for (i = 0; i < len; i++) {
        if (i == 11){
            printf("i=%d 0x%02x\n", i, src[i]);
        }
        for (j = 7; j >= 0; j--) { // 逐 bit 檢查
            bit = (src[i] >> j) & 1;

            // **檢查是否需要去除 stuffing bit**
            if (bit_count == 5) {
                if (!bit) {  // **如果是 stuffing `0`，跳過它**
                    bit_count = 0;  // **重置計數**
                    // bit_pos++;
                    continue;  // **這次真的跳過 stuffing `0`**
                }
                // **如果是 `1`，則 bit stuffing 不適用，繼續計數**
                bit_count = 0;
            }

            // **將 "stuffing 0 後的 bit" 存入 current_byte**
            current_byte = (current_byte << 1) | bit;
            bit_pos++;

            // **更新 bit_count**
            if (bit) {
                bit_count++;
            } else {
                bit_count = 0;
            }

            // **滿 8 bit，存入 `dest`**
            if (bit_pos == 8) {
                dest[dest_index++] = current_byte;
                bit_pos = 0;
                current_byte = 0;
            }
        }
    }

    // **處理最後不足 8 bit 的數據**
    if (bit_pos > 0) {
        current_byte <<= (8 - bit_pos);
        dest[dest_index++] = current_byte;
    }

    return dest_index;
}

// 解析 HDLC Frame
void read_hdlc_frame(uint8_t *frame, size_t len) {
    uint8_t destuffed_frame[MAX_FRAME_SIZE];
    size_t destuffed_len = bit_destuff(destuffed_frame, &frame[1], len-2);

    printf("  Data: ");
    for (size_t i = 0; i < destuffed_len; i++) {
        printf("%02X ", destuffed_frame[i]);
    }
    printf("\n");

    // if (destuffed_len < 6) {  // 至少要有 FLAG(1) + ADDR(1) + CTRL(1) + FCS(2) + FLAG(1)
    //     printf("Frame too short!\n");
    //     return;
    // }
    
    // if (destuffed_frame[0] != HDLC_FLAG || destuffed_frame[destuffed_len - 1] != HDLC_FLAG) {
    //     printf("Invalid frame: missing flag bytes.\n");
    //     return;
    // }
    
    // uint8_t address = destuffed_frame[1];
    // uint8_t control = destuffed_frame[2];
    // size_t data_len = destuffed_len - 6; // 扣除 FLAG, ADDR, CTRL, FCS

    // printf("HDLC Frame Parsed:\n");
    // printf("  Address: 0x%02X\n", address);
    // printf("  Control: 0x%02X\n", control);
    
    // if (data_len > 0) {
    //     printf("  Data: ");
    //     for (size_t i = 0; i < data_len; i++) {
    //         printf("%02X ", destuffed_frame[3 + i]);
    //     }
    //     printf("\n");
    // } else {
    //     printf("  No Data\n");
    // }
    
    // // 檢查 FCS
    // uint16_t received_fcs = (destuffed_frame[destuffed_len - 3] << 8) | destuffed_frame[destuffed_len - 2];
    // uint16_t calculated_fcs = crc16_ccitt(destuffed_frame + 1, destuffed_len - 4);
    // if (received_fcs == calculated_fcs) {
    //     printf("  FCS: 0x%04X (Valid)\n", received_fcs);
    // } else {
    //     printf("  FCS: 0x%04X (Invalid, Expected: 0x%04X)\n", received_fcs, calculated_fcs);
    // }
}

// Data: 01 03 AA BB CC DD EE FF 9A FD 00 
int app_main() {

    uint8_t test_frame[] = {0x7E, 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFB, 0xCD, 0x7D, 0x40, 0x7E}; // 帶有比特填充的資料

    read_hdlc_frame(test_frame, sizeof(test_frame));

    return 0;
}
