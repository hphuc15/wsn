#ifndef SX127X_DEFS_H
#define SX127X_DEFS_H

#include "sx127x_defs_lora.h"
/* #include "sx127x_defs_fsk.h" */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SX127X_VERSION_DEFAULT      (0x12u) /**< Default value */


/* ================================================================
 *  COMMON REGISTERS  (LoRa & FSK/OOK shared)
 *  Ref: SX1276 datasheet Table 85
 * ================================================================ */

#define SX127X_REGFIFO              (0x00u) /**< FIFO read/write access */
#define SX127X_REGOPMODE            (0x01u) /**< Operating mode & LoRa/FSK selection */
#define SX127X_REGFRFMSB            (0x06u) /**< FRF[23:16] */
#define SX127X_REGFRFMID            (0x07u) /**< FRF[15:8]  */
#define SX127X_REGFRFLSB            (0x08u) /**< FRF[7:0]   */
#define SX127X_REGPACONFIG          (0x09u) /**< PA selection and output power */
#define SX127X_REGPARAMP            (0x0Au) /**< PA ramp time, low phase noise PLL */
#define SX127X_REGOCP               (0x0Bu) /**< Over current protection */
#define SX127X_REGLNA               (0x0Cu) /**< LNA settings */
#define SX127X_REGDIOMAPPING1       (0x40u) /**< Mapping of DIO0–DIO3 */
#define SX127X_REGDIOMAPPING2       (0x41u) /**< Mapping of DIO4–DIO5, CLK OUT */
#define SX127X_REGVERSION           (0x42u) /**< Semtech ID — always 0x12 */


/* ================================================================
 *  OPMODE REGISTER — bit manipulation
 * ================================================================ */

#define SX127X_REGOPMODE_MODE_MASK              (0x07u)                                     /**< Mask bits [2:0]: Mode */
#define SX127X_REGOPMODE_SET_MODEM(reg, modem)  ((uint8_t)(((reg) & (0x7Fu)) | (modem)))    /**< Set modem (LoRa/FSK) bit */
#define SX127X_REGOPMODE_SET_MODE(reg, mode)    ((uint8_t)(((reg) & (0xF8u)) | (mode)))     /**< Set Mode bits [2:0] */
#define SX127X_REGOPMODE_BUILD(modem, mode)     ((uint8_t)((modem) | (mode)))               /**< Build RegOpMode byte */


/**
 * FREQUENCY — FRF calculation
 * FRF = freq_hz / Fstep,  Fstep = Fxosc / 2^19
 */

#define SX127X_FXOSC_HZ             (32000000UL)

#define SX127X_CALC_FRF(freq_hz)    ((uint32_t)(((uint64_t)(freq_hz) << 19) / SX127X_FXOSC_HZ))

#define SX127X_FRFMSB(frf)          (uint8_t)(((frf) >> 16) & 0xFFu)
#define SX127X_FRFMID(frf)          (uint8_t)(((frf) >>  8) & 0xFFu)
#define SX127X_FRFLSB(frf)          (uint8_t)(((frf) >>  0) & 0xFFu)


/* ================================================================
 *  ENUMS
 * ================================================================ */


typedef enum {
    SX127X_OK               = 0,    /**< SX127x_Status value indicating success (no error) */
    SX127X_FAIL             = -1,   /**< Generic SX127x_Status code indicating failure */
    SX127X_ERR_SPI          = -2,
    SX127X_ERR_INVALID_ARGS = -3,
    SX127X_ERR_INVALID_MODE = -4,
    SX127X_ERR_NO_DEVICE    = -5,
    SX127X_ERR_RESET        = -6,
    SX127X_ERR_TIMEOUT      = -7,
    SX127X_ERR_CRC          = -8,

} SX127x_Status;

typedef enum {
    SX127X_MODEM_LORA   = 0x80u,    /**< LongRangeMode = 1 */
    SX127X_MODEM_FSK    = 0x00u,    /**< LongRangeMode = 0 */
} SX127x_Modem;

typedef enum {
    SX127X_RESET_POR    = 0,        /**< Power-on reset - Section 7.2.1 */
    SX127X_RESET_MANUAL = 1,        /**< Manual reset through NRESET pin - Section 7.2.2 */
} SX127x_ResetMechanism;

typedef union {
    SX127x_LoRa_OpMode lora;
    // SX127x_FSK_OpMode fsk;
} SX127x_OpMode;


typedef union{
    SX127x_LoRa_Config lora;
    // SX127x_FSK_Config fsk;
} SX127x_Config;

typedef struct {
    uint8_t *data;
    uint8_t length;
} SX127x_Payload;


typedef struct SX127x_Dev SX127x_Dev;

struct SX127x_Dev{
    SX127x_Modem modem;
    SX127x_OpMode mode;
    /* HAL */
    int (*spi_transfer)(uint8_t reg, uint8_t *tx, uint8_t *rx, size_t len);
    void (*set_nreset)(int8_t level);                                                           /**< Set level pin 7 (NRESET) of chip SX127x, -1 on floating */
    void (*delay_ms)(uint32_t ms);
    void (*delay_us)(uint32_t us);
    /* Callbacks */
    void (*rxdone_cb)(SX127x_Dev *dev, uint8_t *data, uint8_t len, int16_t rssi, int8_t snr);
    void (*txdone_cb)(SX127x_Dev *dev);
};



#endif /* SX127X_DEFS_H */