#ifndef RECEIVER_H
#define RECEIVER_H

#include <stdint.h>
#include <stddef.h>

/* ================================================================
 *  Packet format
 * ================================================================ */

typedef struct __attribute__((packed)) {
    uint8_t  device_id;
    uint16_t temp_c;          /* x0.1 degC */
    uint16_t humi_rh;         /* x0.1 %RH */
    uint16_t soil_humi_vwc;   /* x0.1 %VWC */
} Receiver_Packet;

/* ================================================================
 *  Public API
 * ================================================================ */

int receiver_init(void);
int receiver_start(void);
int receiver_stop(void);

/**
 * @brief Block until a packet arrives or timeout.
 *
 * @param out         Destination for the received packet.
 * @param timeout_ms  0 = wait forever.
 * @return 0 on success, -1 on timeout/error.
 */
int receiver_read(Receiver_Packet *out, uint32_t timeout_ms);

/**
 * @brief Serialize a packet to a JSON string.
 *
 * Output format: {"device_id":1,"temp_c":26.0,"humi_rh":58.4,"soil_humi_vwc":29.2}
 *
 * @param pkt      Packet to serialize.
 * @param out_buf  Destination buffer.
 * @param buf_len  Size of out_buf. 96 bytes is enough for this struct.
 * @return Number of bytes written (excluding null terminator) on
 *         success, -1 if buf_len was too small or pkt/out_buf is NULL.
 */
int Receiver_ToJson(const Receiver_Packet *pkt, char *out_buf, size_t buf_len);

#endif /* RECEIVER_H */