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

// Bit Stuffing
size_t bit_stuff(uint8_t *dest, uint8_t *src, size_t len) {
    size_t dest_index = 0;
    int bit_count = 0;
    uint8_t current_byte = 0;
    int bit_pos = 0; // 當前處理的 bit 位置

    size_t i =0;
    int j = 0;
    bool bit = 0;
    for (i = 0; i < len; i++) {
        for (j = 7; j >= 0; j--) { // 逐 bit 檢查
            bit = (src[i] >> j) & 1;
            current_byte = (current_byte << 1) | bit;
            bit_pos++;

            // **更新 bit_count**
            if (bit) {
                bit_count++;
            } else {
                bit_count = 0;
            }

            // **檢查是否需要stuffing bit**
            if (bit_count == 5) {
                bit_count = 0;  // **重置計數**
                current_byte = (current_byte << 1) | 0;
                bit_pos++;
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

// HDLC Frame
void write_hdlc_frame(uint8_t *frame, size_t len) {

    if (len < 4) {
        printf("Frame too short!\n");
        return;
    }
    if (len > MAX_FRAME_SIZE-16) {
        printf("Frame too large!\n");
        return;
    }

     // FCS target : addr + control + payload 
    uint16_t calculated_fcs = crc16_ccitt(frame, len-2);
    // printf("  fcs: 0x%04X\n", calculated_fcs);
    frame[len-2] = calculated_fcs & 0xFF;
    frame[len-1] = (calculated_fcs >> 8) & 0xFF;

    printf("  fcsed data: ");
    for (size_t i = 0; i < len; i++) {
        printf("0x%02X, ", frame[i]);
    }
    printf("\n");

    uint8_t stuffed_frame[MAX_FRAME_SIZE];
    size_t stuffed_len = bit_stuff(stuffed_frame, frame, len);

    printf("  hdlc data : ");
    printf("0x%02X, ", HDLC_FLAG);
    for (size_t i = 0; i < stuffed_len; i++) {
        printf("0x%02X, ", stuffed_frame[i]);
    }
    printf("0x%02X, ", HDLC_FLAG);
    printf("\n");
}

// org data  : 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00, 
// fcsed data: 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x9A, 0xFD,
// hdlc data : 0x7E, 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFB, 0xCD, 0x7D, 0x40, 0x7E, 
int app_main() {

    // addr    0x1
    // control 0x3
    // data    0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    // fcs     0x00, 0x00 //auto filled later
    uint8_t test_frame[] = {0x1, 0x3, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x00};
  
    printf("  org data  : ");
    for (size_t i = 0; i < sizeof(test_frame); i++) {
        printf("0x%02X, ", test_frame[i]);
    }
    printf("\n");

    write_hdlc_frame(test_frame, sizeof(test_frame));

    return 0;
}
