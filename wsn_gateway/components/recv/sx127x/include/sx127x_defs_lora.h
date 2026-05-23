#ifndef SX127X_DEFS_LORA_H
#define SX127X_DEFS_LORA_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief LoRa Register (valid when LongRangeMode = 1)
 * Ref: SX1276 datasheet Table 85
 */
#define SX127X_LORA_REGFIFOADDRPTR          (0x0Du) /**< SPI interface address pointer in FIFO data buffer. */
#define SX127X_LORA_REGFIFOTXBASEADDR       (0x0Eu) /**< Write base address in FIFO data buffer for TX modulator. */
#define SX127X_LORA_REGFIFORXBASEADDR       (0x0Fu) /**< Read base address in FIFO data buffer for RX demodulator. */
#define SX127X_LORA_REGFIFORXCURRENTADDR    (0x10u) /**< Start address (in data buffer) of last packet received. */
#define SX127X_LORA_REGIRQFLAGSMASK         (0x11u)
#define SX127X_LORA_REGIRQFLAGS             (0x12u)
#define SX127X_LORA_REGRXNBBYTES            (0x13u) /**< Number of payload bytes of latest packet received. */
#define SX127X_LORA_REGPKTSNRVALUE          (0x19u) /**< Estimation of SNR on last packet received. In two’s compliment format mutiplied by 4. */
#define SX127X_LORA_REGPKTRSSIVALUE         (0x1Au) /**< RSSI of the latest packet received (dBm). */
#define SX127X_LORA_REGMODEMCONFIG1         (0x1Du)
#define SX127X_LORA_REGMODEMCONFIG2         (0x1Eu)
#define SX127X_LORA_REGSYMBTIMEOUTLSB       (0x1Fu) /**< RX Time-Out LSB. (MSB is RegModemConfig[1:0]). */
#define SX127X_LORA_REGPREAMBLEMSB          (0x20u) /**< Preamble length MSB, */
#define SX127X_LORA_REGPREAMBLELSB          (0x21u) /**< Preamble Length LSB */
#define SX127X_LORA_REGPAYLOADLENGTH        (0x22u) /**< Payload length in bytes. */
#define SX127X_LORA_REGMODEMCONFIG3         (0x26u)

/* IRQ flags (RegIrqFlags 0x12) */
#define SX127X_LORA_IRQ_RXTIMEOUT           (0x80u) /**< Bit 7 */
#define SX127X_LORA_IRQ_RXDONE              (0x40u) /**< Bit 6 */
#define SX127X_LORA_IRQ_PAYLOADCRCERROR     (0x20u) /**< Bit 5 */
#define SX127X_LORA_IRQ_VALIDHEADER         (0x10u) /**< Bit 4 */
#define SX127X_LORA_IRQ_TXDONE              (0x08u) /**< Bit 3 */
#define SX127X_LORA_IRQ_CADDONE             (0x04u) /**< Bit 2 */
#define SX127X_LORA_IRQ_CADDETECTED         (0x01u) /**< Bit 0 */




#define SX127X_LORA_REGSYNCWORD             (0x39u)


/**
 * @brief LoRa bandwidth (BW) configuration options.
 */
typedef enum {
    SX127X_LORA_BW_7K8      = 0, /**< 7.8 kHz */
    SX127X_LORA_BW_10K4     = 1, /**< 10.4 kHz */
    SX127X_LORA_BW_15K6     = 2, /**< 15.6 kHz */
    SX127X_LORA_BW_20K8     = 3, /**< 20.8 kHz */
    SX127X_LORA_BW_31K25    = 4, /**< 31.25 kHz */
    SX127X_LORA_BW_41K7     = 5, /**< 41.7 kHz */
    SX127X_LORA_BW_62K5     = 6, /**< 62.5 kHz */
    SX127X_LORA_BW_125K     = 7, /**< 125 kHz */
    SX127X_LORA_BW_250K     = 8, /**< 250 kHz */
    SX127X_LORA_BW_500K     = 9, /**< 500 kHz */
} SX127x_LoRa_BW;

/**
 * @brief LoRa spreading factor (SF) configuration options.
 * Note: SF6 requires implicit header mode.
 */
typedef enum {
    SX127X_LORA_SF_6        = 6,  /**< 64 chips / symbol */
    SX127X_LORA_SF_7        = 7,  /**< 128 chips / symbol */
    SX127X_LORA_SF_8        = 8,  /**< 256 chips / symbol */
    SX127X_LORA_SF_9        = 9,  /**< 512 chips / symbol */
    SX127X_LORA_SF_10       = 10, /**< 1024 chips / symbol */
    SX127X_LORA_SF_11       = 11, /**< 2048 chips / symbol */
    SX127X_LORA_SF_12       = 12, /**< 4096 chips / symbol */
} SX127x_LoRa_SF;

/**
 * @brief LoRa error coding rate (CR) configuration options.
 */
typedef enum {
    SX127X_LORA_CR_4_5      = 1, /**< Coding rate 4/5 */
    SX127X_LORA_CR_4_6      = 2, /**< Coding rate 4/6 */
    SX127X_LORA_CR_4_7      = 3, /**< Coding rate 4/7 */
    SX127X_LORA_CR_4_8      = 4, /**< Coding rate 4/8 */
} SX127x_LoRa_CR;

/**
 * @brief LoRa Configuration
 */
typedef struct {
    uint32_t            frequency_hz;       /**< Center frequency in Hz (e.g. 433000000) */
    SX127x_LoRa_BW      bandwidth;          /**< LoRa bandwidth configuration */
    SX127x_LoRa_SF      spreading_factor;   /**< LoRa spreading factor (6 to 12) */
    SX127x_LoRa_CR      coding_rate;        /**< LoRa error coding rate */
    uint16_t            preamble_length;    /**< Preamble length in symbols */
    uint8_t             tx_power_dbm;       /**< Transmit power in dBm (2 to 17) */
    uint8_t             sync_word;          /**< LoRa sync word: 0x12 (Private), 0x34 (Public/LoRaWAN) */
    bool                crc_enable;         /**< Enable payload CRC check/generation */
    bool                implicit_header;    /**< Enable implicit header mode (fixed payload length) */
} SX127x_LoRa_Config;

/**
 * @brief LoRa Operation Mode, bits [2:0] of RegOpMode register.
 * Use this enum like mode field in macro SX127X_OPMODE_SET_MODE(reg, mode).
 */
typedef enum {
    SX127X_LORA_MODE_SLEEP          = 0x00, /**< 000: SLEEP */
    SX127X_LORA_MODE_STANDBY        = 0x01, /**< 001: STDBY */
    SX127X_LORA_MODE_FSTX           = 0x02, /**< 010: Frequency synthesis TX (FSTX) */
    SX127X_LORA_MODE_TX             = 0x03, /**< 011: Transmit (TX) */
    SX127X_LORA_MODE_FSRX           = 0x04, /**< 100: Frequency syntheris RX (FSRX) */
    SX127X_LORA_MODE_RXCONTINUOUS   = 0x05, /**< 101: Receive continuous (RXCONTINUOUS) */
    SX127X_LORA_MODE_RXSINGLE       = 0x06, /**< 110: Receive single (RXSINGLE) */
    SX127X_LORA_MODE_CAD            = 0x07  /**< 111: Channel activity detection (CAD) */
} SX127x_LoRa_OpMode;


#endif /* SX127X_DEFS_LORA_H */