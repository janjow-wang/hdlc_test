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

// Bit Destuffing 
size_t bit_destuff(uint8_t *dest, uint8_t *src, size_t len) {
    size_t dest_index = 0;
    int bit_count = 0;
    uint8_t current_byte = 0;
    int bit_pos = 0; // current collected bits

    bool start_flag_found = 0;
    bool end_flag_found = 0;
    bool payload_found = 0;
    uint8_t org_byte = 0;
    int skip_bit = 0;
    int skip_bit_force = 0; //use to test !byte_align case

    int i =0;
    int j = 0;
    bool bit = 0;
    for (i = 0; i < len; i++) {
        // if (i == 14){
        //     printf("i=%d 0x%02x\n", i, src[i]);
        // }
        for (j = 7; j >= 0; j--) { // 逐 bit 檢查
            bit = (src[i] >> j) & 1;

            org_byte = (org_byte << 1) | bit;

            if (skip_bit_force) //use to test !byte_align case
            {
                skip_bit_force--;
                continue;
            }

            if (skip_bit) //skip 7 bits, goto next byte position
            {
                skip_bit--;
                continue;
            }

            if (org_byte == HDLC_FLAG)
            {
                if (payload_found == true)
                {
                    // 檢查 FCS
                    // uint16_t received_fcs = (dest[dest_index - 1] << 8) | dest[dest_index - 2];
                    // uint16_t calculated_fcs = crc16_ccitt(dest, dest_index - 2);
                    // if (received_fcs == calculated_fcs) {
                    //     printf("  FCS: 0x%04X (Valid)\n", received_fcs);
                    // } else {
                    //     printf("  FCS: 0x%04X (Invalid, Expected: 0x%04X)\n", received_fcs, calculated_fcs);
                    // }

                    printf("  Data: ");
                    for (size_t i = 0; i < dest_index; i++) {
                        printf("%02X ", dest[i]);
                    }
                    printf("\n");                                

                    org_byte = 0;
                    start_flag_found = false;
                    payload_found = false;
                    bit_count = 0;
                    bit_pos = 0;
                    current_byte = 0;
                    dest_index = 0;
                    continue;
                }

                // if (start_flag_found == false)
                {
                    org_byte = 0;
                    start_flag_found = true;
                    skip_bit = 7;
                    continue;
                }
            }
            else
            {
                if (start_flag_found == false)
                {
                    continue;
                }

                // payload found, go back 8 bit for de-stuffing flow
                if (payload_found == false)
                {
                    if (j > 0)
                    {
                        i = i - 1;
                    }
                    j = 8 - j;

                    org_byte = 0;
                    payload_found = true;
                    continue;
                }
            }

            // **檢查是否需要去除 stuffing bit**
            if (bit_count == 5) {
                if (!bit) {  // **如果是 stuffing `0`，跳過它**
                    bit_count = 0;  // **重置計數**
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

    // last remaining bits are ignored, no padding
    // // **處理最後不足 8 bit 的數據**
    // if (bit_pos > 0) {
    //     current_byte <<= (8 - bit_pos);
    //     dest[dest_index++] = current_byte;
    // }

    return dest_index;
}

// 解析 HDLC Frame
void read_hdlc_frame(uint8_t *frame, size_t len) {
    uint8_t destuffed_frame[MAX_FRAME_SIZE];
    size_t destuffed_len = bit_destuff(destuffed_frame, frame, len);
}


// int app_main() { // for esp32
int main() { // for gcc

    // org data : 01 03 AA BB CC DD EE FF 9A FD
    // uint8_t test_frame[] = {0x7E, 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFB, 0xCD, 0x7D, 0x5F, 0x80}; // 帶有比特填充的資料
    // uint8_t test_frame[] = {0x7E, 0x7E, 0x7E, 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFB, 0xCD, 0x7D, 0x5F, 0x80}; // 帶有比特填充的資料
    uint8_t test_frame[] = {
    0x7E, 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFB, 0xCD, 0x7D, 0x5F, 0x80,
    0x7E, 0x7E, 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFB, 0xCD, 0x7D, 0x5F, 0x80,
    0x7E, 0x7E, 0x7E, 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFB, 0xCD, 0x7D, 0x5F, 0x80,
    0x7E, 0x7E, 0x7E, 0x7E, 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFB, 0xCD, 0x7D, 0x5F, 0x80}; // 帶有比特填充的資料

    read_hdlc_frame(test_frame, sizeof(test_frame));

    return 0;
}
