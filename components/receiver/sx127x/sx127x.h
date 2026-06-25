/**
 * @file    sx127x.h
 * @brief   SX1276/77/78/79 driver - public API (LoRa modem).
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 *  STATUS
 * ================================================================ */

/**
 * @brief Return status for all public API functions.
 */
typedef enum {
    SX127X_OK               =  0,
    SX127X_ERR_INVALID_ARG  = -1,   /**< NULL pointer or out-of-range parameter    */
    SX127X_ERR_SPI          = -2,   /**< SPI transfer returned non-zero            */
    SX127X_ERR_NO_DEVICE    = -3,   /**< Version register mismatch                 */
    SX127X_ERR_INVALID_MODE = -4,   /**< Operation not supported in current modem  */
    SX127X_ERR_CRC          = -5,   /**< Payload CRC error on received packet      */
    SX127X_ERR_NO_PACKET    = -6,   /**< RxDone IRQ not set (no packet ready)      */
} SX127x_Status;

/* ================================================================
 *  MODEM
 * ================================================================ */

typedef enum {
    SX127X_MODEM_LORA       = 0,
    SX127X_MODEM_FSK        = 1,          /**< Not implemented yet  */
} SX127x_Modem;

/* ================================================================
 *  LORA PARAMETERS
 * ================================================================ */

/** Signal bandwidth (RegModemConfig1[7:4]) */
typedef enum {
    SX127X_LORA_BW_7K8      = 0,
    SX127X_LORA_BW_10K4     = 1,
    SX127X_LORA_BW_15K6     = 2,
    SX127X_LORA_BW_20K8     = 3,
    SX127X_LORA_BW_31K25    = 4,
    SX127X_LORA_BW_41K7     = 5,
    SX127X_LORA_BW_62K5     = 6,
    SX127X_LORA_BW_125K     = 7,
    SX127X_LORA_BW_250K     = 8,
    SX127X_LORA_BW_500K     = 9,
} SX127x_LoRaBW;

/** Coding rate (RegModemConfig1[3:1]) */
typedef enum {
    SX127X_LORA_CR_4_5      = 1,
    SX127X_LORA_CR_4_6      = 2,
    SX127X_LORA_CR_4_7      = 3,
    SX127X_LORA_CR_4_8      = 4,
} SX127x_LoRaCR;

/** Spreading factor (RegModemConfig2[7:4]) */
typedef enum {
    SX127X_LORA_SF6         = 6,
    SX127X_LORA_SF7         = 7,
    SX127X_LORA_SF8         = 8,
    SX127X_LORA_SF9         = 9,
    SX127X_LORA_SF10        = 10,
    SX127X_LORA_SF11        = 11,
    SX127X_LORA_SF12        = 12,
} SX127x_LoRaSF;

/**
 * @brief LoRa modem configuration.
 */
typedef struct {
    uint32_t      frequency_hz;     /**< Carrier frequency, Hz (e.g. 433000000)       */
    SX127x_LoRaBW bandwidth;        /**< Signal bandwidth                              */
    SX127x_LoRaSF spreading_factor; /**< Spreading factor SF6-SF12                     */
    SX127x_LoRaCR coding_rate;      /**< Error coding rate                             */
    uint16_t      preamble_length;  /**< Preamble length in symbols (>=6, typical 8)   */
    int8_t        tx_power_dbm;     /**< TX output power dBm: 2-17, or 18-20           */
    uint8_t       sync_word;        /**< Network sync word (0x12=private, 0x34=public) */
    bool          crc_on;           /**< Enable CRC on payload                         */
    bool          implicit_header;  /**< true = implicit (fixed-length) header mode    */
} SX127x_LoRaConfig;

/* ================================================================
 *  CONFIG  (union - extendable to FSK)
 * ================================================================ */

/**
 * @brief Top-level radio configuration.
 *
 * Set `modem` first, then fill the matching union member.
 * Example:
 * @code
 *   SX127x_Config cfg = {
 *       .modem = SX127X_MODEM_LORA,
 *       .lora  = { ... },
 *   };
 * @endcode
 */
typedef struct {
    SX127x_Modem modem;
    union {
        SX127x_LoRaConfig lora;
        /* SX127x_FskConfig fsk; - reserved, not implemented */
    };
} SX127x_Config;

/* ================================================================
 *  PACKET
 * ================================================================ */

/**
 * @brief Received packet descriptor passed to on_rx_done callback.
 */
typedef struct {
    uint8_t  *data;     /**< Pointer to payload buffer              */
    uint8_t   length;   /**< Number of received bytes               */
    int16_t   rssi;     /**< Packet RSSI, dBm                       */
    int8_t    snr;      /**< Packet SNR, raw register units (0.25 dB/LSB) */
} SX127x_Packet;

/* ================================================================
 *  DEVICE STRUCT
 * ================================================================ */

/** Forward declaration for use in callback typedefs below. */
typedef struct SX127x_Dev SX127x_Dev;

/**
 * @brief HAL SPI transfer.
 *
 * The driver calls this for every register read/write.
 * @param reg  Register address byte (already OR'd with 0x80 for writes by the driver,
 *             AND'd with 0x7F for reads).
 * @param tx   Data to transmit after the address byte; NULL for reads.
 * @param rx   Buffer to receive data; NULL for writes.
 * @param len  Number of data bytes (not counting the address byte).
 * @return 0 on success, non-zero on error.
 */
typedef int (*SX127x_SpiTransfer)(uint8_t reg, const uint8_t *tx, uint8_t *rx, size_t len);

/**
 * @brief HAL nRESET control.
 *
 * @param level  0 = assert reset low, positive = release high, negative = float (Hi-Z input).
 */
typedef void (*SX127x_SetNReset)(int8_t level);

/** @brief HAL millisecond delay. */
typedef void (*SX127x_DelayMs)(uint32_t ms);

/** @brief HAL microsecond delay. */
typedef void (*SX127x_DelayUs)(uint32_t us);

/** @brief TX-done callback - called from SX127x_HandleDIO0() when TxDone IRQ fires. */
typedef void (*SX127x_OnTxDone)(SX127x_Dev *dev);

/**
 * @brief RX-done callback - called from SX127x_HandleDIO0() when RxDone IRQ fires.
 *
 * @param dev     Device instance.
 * @param packet  Received packet (data pointer valid only for the duration of this call).
 *                CRC error packets are silently dropped before reaching this callback.
 */
typedef void (*SX127x_OnRxDone)(SX127x_Dev *dev, const SX127x_Packet *packet);

/**
 * @brief SX127x device instance.
 *
 * Initialise the HAL function pointers and optional callbacks before calling
 * SX127x_Init(). All other fields are managed by the driver.
 */
struct SX127x_Dev {
    /* HAL (required) */
    SX127x_SpiTransfer  spi_transfer;   /**< Must not be NULL                          */
    SX127x_DelayMs      delay_ms;       /**< Must not be NULL                          */
    SX127x_DelayUs      delay_us;       /**< Must not be NULL                          */

    /* HAL (optional) */
    SX127x_SetNReset    set_nreset;     /**< NULL -> SX127x_Reset(MANUAL) returns error */

    /* Callbacks (optional) */
    SX127x_OnTxDone     on_tx_done;
    SX127x_OnRxDone     on_rx_done;

    /* State (read-only, managed by driver) */
    SX127x_Modem        modem;          /**< Active modem, set by SX127x_Init()        */
    uint8_t             mode;           /**< Current RegOpMode[2:0]                    */
    SX127x_Config       config;         /**< Copy of last applied config               */
};

/* ================================================================
 *  RESET
 * ================================================================ */

typedef enum {
    SX127X_RESET_POR,       /**< Power-on reset */
    SX127X_RESET_MANUAL     /**< Reset via nRESET */
} SX127x_ResetMode;

/* ================================================================
 *  PUBLIC API - lifecycle
 * ================================================================ */

/**
 * @brief Reset the chip via nRESET pin (MANUAL) or by waiting out VCC ramp-up (POR).
 */
SX127x_Status SX127x_Reset(SX127x_Dev *dev, SX127x_ResetMode mode);

/**
 * @brief Probe the chip, put it in LoRa+Sleep, apply baseline register defaults,
 *        then apply `config` and leave the device in Standby.
 *
 * Must be called once before any other API function (except SX127x_Reset()).
 */
SX127x_Status SX127x_Init(SX127x_Dev *dev, const SX127x_Config *config);

/**
 * @brief Apply a full LoRa configuration (frequency, BW, SF, CR, etc.) in one call.
 *
 * Equivalent to calling each SX127x_Set*() setter individually.
 */
SX127x_Status SX127x_SetConfig(SX127x_Dev *dev, const SX127x_Config *config);

/* ================================================================
 *  PUBLIC API - per-parameter setters
 * ================================================================ */

/** @brief Set carrier frequency in Hz (writes RegFrfMsb/Mid/Lsb). */
SX127x_Status SX127x_SetFrequency(SX127x_Dev *dev, uint32_t freq_hz);

/** @brief Set signal bandwidth; also re-applies the LowDataRateOptimize bit. */
SX127x_Status SX127x_SetBandwidth(SX127x_Dev *dev, SX127x_LoRaBW bw);

/**
 * @brief Set spreading factor; also applies the SF6 detection-optimize workaround
 *        and re-applies the LowDataRateOptimize bit.
 *
 * @note SF6 requires implicit header mode to already be enabled.
 */
SX127x_Status SX127x_SetSpreadingFactor(SX127x_Dev *dev, SX127x_LoRaSF sf);

/** @brief Set forward error correction coding rate. */
SX127x_Status SX127x_SetCodingRate(SX127x_Dev *dev, SX127x_LoRaCR cr);

/** @brief Set preamble length in symbols (minimum 6). */
SX127x_Status SX127x_SetPreambleLength(SX127x_Dev *dev, uint16_t preamble_length);

/** @brief Set network sync word (RegSyncWord), e.g. 0x12 private / 0x34 public. */
SX127x_Status SX127x_SetSyncWord(SX127x_Dev *dev, uint8_t sync_word);

/** @brief Enable or disable payload CRC generation/checking. */
SX127x_Status SX127x_SetCrc(SX127x_Dev *dev, bool enable);

/** @brief Enable or disable implicit (fixed-length) header mode. */
SX127x_Status SX127x_SetImplicitHeader(SX127x_Dev *dev, bool enable);

/**
 * @brief Set TX output power.
 *
 * @param dev        Device instance.
 * @param level_dbm  Desired power in dBm. With use_rfo=false (PA_BOOST): 2-17 dBm
 *                    normal range, 18-20 dBm enables High Power mode (datasheet 5.4.3).
 *                    With use_rfo=true (RFO pin): 0-14 dBm.
 * @param use_rfo     true = use RFO pin (max +14dBm), false = use PA_BOOST pin (max +20dBm).
 */
SX127x_Status SX127x_SetTxPower(SX127x_Dev *dev, int8_t level_dbm, bool use_rfo);

/**
 * @brief Set over-current protection trip point.
 * @param ma  Trip current in mA (clamped to 45-240). See datasheet RegOcp.
 */
SX127x_Status SX127x_SetOCP(SX127x_Dev *dev, uint8_t ma);

/**
 * @brief Set LNA gain manually, or 0 to re-enable AGC.
 * @param gain  0 = AGC auto, 1-6 = manual gain (1 = max gain, RegLna[7:5]).
 */
SX127x_Status SX127x_SetGain(SX127x_Dev *dev, uint8_t gain);

/**
 * @brief Enable/disable I/Q inversion (datasheet 4.1.4, RegInvertIQ/RegInvertIQ2).
 */
SX127x_Status SX127x_SetInvertIQ(SX127x_Dev *dev, bool invert);

/**
 * @brief Manually force the LowDataRateOptimize bit, bypassing the automatic
 *        symbol-time calculation normally applied by SetBandwidth/SetSpreadingFactor.
 */
SX127x_Status SX127x_SetLowDataRateOptimizeForced(SX127x_Dev *dev, bool enable);

/* ================================================================
 *  PUBLIC API - TX / RX
 * ================================================================ */

/**
 * @brief Load and transmit a packet. Returns immediately; TX completion is
 *        signalled via on_tx_done (if set) through SX127x_HandleDIO0().
 *
 * @return SX127X_ERR_INVALID_MODE if a transmission is already in progress.
 */
SX127x_Status SX127x_Send(SX127x_Dev *dev, const uint8_t *data, uint8_t len);

/** @brief Switch to RX mode (single-packet or continuous), DIO0 mapped to RxDone. */
SX127x_Status SX127x_StartReceive(SX127x_Dev *dev, bool continuous);

/** @brief Stop receiving and return to Standby. */
SX127x_Status SX127x_StopReceive(SX127x_Dev *dev);

/**
 * @brief Poll for and read a received packet (non-blocking).
 *
 * @return SX127X_ERR_NO_PACKET if RxDone has not fired yet,
 *         SX127X_ERR_CRC if the payload failed CRC.
 */
SX127x_Status SX127x_Receive(SX127x_Dev *dev, SX127x_Packet *packet);

/**
 * @brief Start Channel Activity Detection. Result delivered via DIO0 IRQ
 *        (SX127X_IRQ_CAD_DONE / SX127X_IRQ_CAD_DETECTED) - poll IRQ flags
 *        or use SX127x_HandleDIO0() with a custom check in on_rx_done.
 */
SX127x_Status SX127x_StartCAD(SX127x_Dev *dev);

/**
 * @brief Service the DIO0 IRQ line: clears the pending flag and dispatches
 *        on_rx_done or on_tx_done as appropriate. Call from an ISR or polling loop.
 */
void SX127x_HandleDIO0(SX127x_Dev *dev);

/* ================================================================
 *  PUBLIC API - mode control
 * ================================================================ */

/** @brief Enter Sleep mode (lowest power, FIFO not retained). */
SX127x_Status SX127x_Sleep(SX127x_Dev *dev);

/** @brief Enter Standby mode (idle, FIFO retained, ready for TX/RX). */
SX127x_Status SX127x_Standby(SX127x_Dev *dev);

/** @brief Check whether a transmission is currently in progress. */
bool          SX127x_IsTransmitting(SX127x_Dev *dev);

/* ================================================================
 *  PUBLIC API - status / diagnostics
 * ================================================================ */

/** @brief Instantaneous channel RSSI (not last-packet RSSI). */
SX127x_Status SX127x_GetRSSI(SX127x_Dev *dev, int16_t *out_rssi);

/** @brief RSSI of the last received packet (RegPktRssiValue). */
SX127x_Status SX127x_GetPacketRSSI(SX127x_Dev *dev, int16_t *out_rssi);

/** @brief SNR of the last received packet, raw register units (multiply by 0.25 for dB). */
SX127x_Status SX127x_GetPacketSNR(SX127x_Dev *dev, int8_t *out_snr_raw);

/** @brief Read RegRssiWideband - cheap entropy source (datasheet 4.1.5). */
SX127x_Status SX127x_Random(SX127x_Dev *dev, uint8_t *out_value);

/**
 * @brief Dump all common+LoRa registers (0x00-0x7F) via a user-supplied callback.
 * @param print_fn  Invoked once per register: print_fn(addr, value).
 */
void SX127x_DumpRegisters(SX127x_Dev *dev, void (*print_fn)(uint8_t addr, uint8_t value));

/**
 * @brief Convert SX127x_Status code to a human-readable string.
 *
 * @return Pointer to a string literal, or "(unknown)" for unrecognised codes.
 */
const char *SX127x_StatusStr(SX127x_Status status);

#ifdef __cplusplus
}
#endif