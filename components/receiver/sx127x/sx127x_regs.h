/**
 * @file    sx127x_regs.h
 * @brief   SX1276/77/78/79 register map - LoRa & common registers.
 *
 * References:
 *   - SX1276/77/78/79 Datasheet, Rev 7 (Semtech)
 *
 * FSK/OOK register section is present but not documented here;
 * add when FSK modem is implemented.
 */
#pragma once

/* ================================================================
 *  COMMON REGISTERS  (both LoRa and FSK)
 * ================================================================ */

#define SX127X_REG_FIFO                     0x00
#define SX127X_REG_OP_MODE                  0x01
#define SX127X_REG_FRF_MSB                  0x06
#define SX127X_REG_FRF_MID                  0x07
#define SX127X_REG_FRF_LSB                  0x08
#define SX127X_REG_PA_CONFIG                0x09
#define SX127X_REG_PA_RAMP                  0x0A
#define SX127X_REG_OCP                      0x0B
#define SX127X_REG_LNA                      0x0C
#define SX127X_REG_DIO_MAPPING1             0x40
#define SX127X_REG_DIO_MAPPING2             0x41
#define SX127X_REG_VERSION                  0x42
#define SX127X_REG_TCXO                     0x4B
#define SX127X_REG_PA_DAC                   0x4D
#define SX127X_REG_FORMER_TEMP              0x5B
#define SX127X_REG_AGC_REF                  0x61
#define SX127X_REG_AGC_THRESH1              0x62
#define SX127X_REG_AGC_THRESH2              0x63
#define SX127X_REG_AGC_THRESH3              0x64

/* ================================================================
 *  LORA REGISTERS  (valid when LongRangeMode = 1)
 * ================================================================ */

#define SX127X_LORA_REG_FIFO_ADDR_PTR          0x0D
#define SX127X_LORA_REG_FIFO_TX_BASE_ADDR      0x0E
#define SX127X_LORA_REG_FIFO_RX_BASE_ADDR      0x0F
#define SX127X_LORA_REG_FIFO_RX_CURRENT_ADDR   0x10
#define SX127X_LORA_REG_IRQ_FLAGS_MASK          0x11
#define SX127X_LORA_REG_IRQ_FLAGS               0x12
#define SX127X_LORA_REG_RX_NB_BYTES            0x13
#define SX127X_LORA_REG_RX_HEADER_CNT_MSB      0x14
#define SX127X_LORA_REG_RX_HEADER_CNT_LSB      0x15
#define SX127X_LORA_REG_RX_PACKET_CNT_MSB      0x16
#define SX127X_LORA_REG_RX_PACKET_CNT_LSB      0x17
#define SX127X_LORA_REG_MODEM_STAT             0x18
#define SX127X_LORA_REG_PKT_SNR_VALUE          0x19
#define SX127X_LORA_REG_PKT_RSSI_VALUE         0x1A
#define SX127X_LORA_REG_RSSI_VALUE             0x1B
#define SX127X_LORA_REG_HOP_CHANNEL            0x1C
#define SX127X_LORA_REG_MODEM_CONFIG1          0x1D
#define SX127X_LORA_REG_MODEM_CONFIG2          0x1E
#define SX127X_LORA_REG_SYMB_TIMEOUT_LSB       0x1F
#define SX127X_LORA_REG_PREAMBLE_MSB           0x20
#define SX127X_LORA_REG_PREAMBLE_LSB           0x21
#define SX127X_LORA_REG_PAYLOAD_LENGTH         0x22
#define SX127X_LORA_REG_MAX_PAYLOAD_LENGTH     0x23
#define SX127X_LORA_REG_HOP_PERIOD             0x24
#define SX127X_LORA_REG_FIFO_RX_BYTE_ADDR      0x25
#define SX127X_LORA_REG_MODEM_CONFIG3          0x26
#define SX127X_LORA_REG_FEI_MSB                0x28
#define SX127X_LORA_REG_FEI_MID                0x29
#define SX127X_LORA_REG_FEI_LSB                0x2A
#define SX127X_LORA_REG_RSSI_WIDEBAND          0x2C
#define SX127X_LORA_REG_DETECT_OPTIMIZE        0x31
#define SX127X_LORA_REG_INVERT_IQ              0x33
#define SX127X_LORA_REG_DETECTION_THRESHOLD    0x37
#define SX127X_LORA_REG_SYNC_WORD              0x39
#define SX127X_LORA_REG_INVERT_IQ2             0x3B

/* ================================================================
 *  RegOpMode (0x01)
 * ================================================================ */

#define SX127X_OPMODE_LONG_RANGE_MODE       (1u << 7)   /**< 1 = LoRa, 0 = FSK/OOK   */
#define SX127X_OPMODE_LOW_FREQ_MODE_ON      (1u << 3)   /**< 1 = LF (<=779 MHz)        */
#define SX127X_OPMODE_MODE_MASK             0x07u
#define SX127X_OPMODE_MODE_SHIFT            0

#define SX127X_OPMODE_MODE_SET(reg, mode) \
    (((reg) & ~SX127X_OPMODE_MODE_MASK) | ((mode) & SX127X_OPMODE_MODE_MASK))

/** LoRa operating modes (RegOpMode[2:0]) */
#define SX127X_MODE_SLEEP                   0x00u
#define SX127X_MODE_STANDBY                 0x01u
#define SX127X_MODE_FSTX                    0x02u
#define SX127X_MODE_TX                      0x03u
#define SX127X_MODE_FSRX                    0x04u
#define SX127X_MODE_RXCONTINUOUS            0x05u
#define SX127X_MODE_RXSINGLE                0x06u
#define SX127X_MODE_CAD                     0x07u

/* ================================================================
 *  RegIrqFlags (0x12)
 * ================================================================ */

#define SX127X_IRQ_CAD_DETECTED             (1u << 0)
#define SX127X_IRQ_FHSS_CHANGE_CHANNEL     (1u << 1)
#define SX127X_IRQ_CAD_DONE                 (1u << 2)
#define SX127X_IRQ_TX_DONE                  (1u << 3)
#define SX127X_IRQ_VALID_HEADER             (1u << 4)
#define SX127X_IRQ_PAYLOAD_CRC_ERROR        (1u << 5)
#define SX127X_IRQ_RX_DONE                  (1u << 6)
#define SX127X_IRQ_RX_TIMEOUT               (1u << 7)
#define SX127X_IRQ_ALL                      0xFFu

/* ================================================================
 *  RegModemConfig1 (0x1D)
 * ================================================================ */

#define SX127X_MC1_BW_MASK                  0xF0u
#define SX127X_MC1_BW_SHIFT                 4
#define SX127X_MC1_CR_MASK                  0x0Eu
#define SX127X_MC1_CR_SHIFT                 1
#define SX127X_MC1_IMPLICIT_HEADER          (1u << 0)

/* ================================================================
 *  RegModemConfig2 (0x1E)
 * ================================================================ */

#define SX127X_MC2_SF_MASK                  0xF0u
#define SX127X_MC2_SF_SHIFT                 4
#define SX127X_MC2_TX_CONTINUOUS            (1u << 3)
#define SX127X_MC2_RX_PAYLOAD_CRC_ON        (1u << 2)
#define SX127X_MC2_SYMB_TIMEOUT_MSB_MASK    0x03u

/* ================================================================
 *  RegModemConfig3 (0x26)
 * ================================================================ */

#define SX127X_MC3_LOW_DATA_RATE_OPTIMIZE   (1u << 3)
#define SX127X_MC3_AGC_AUTO_ON              (1u << 2)

/* ================================================================
 *  RegPaConfig (0x09)
 * ================================================================ */

#define SX127X_PA_SELECT_BOOST              (1u << 7)   /**< PA_BOOST pin  */
#define SX127X_PA_MAX_POWER_MASK            0x70u
#define SX127X_PA_MAX_POWER_SHIFT           4
#define SX127X_PA_OUTPUT_POWER_MASK         0x0Fu

/* ================================================================
 *  RegPaDac (0x4D)
 * ================================================================ */

#define SX127X_PADAC_DEFAULT                0x84u
#define SX127X_PADAC_20DBM                  0x87u       /**< Enable +20 dBm mode  */

/* ================================================================
 *  RegOcp (0x0B)
 * ================================================================ */

#define SX127X_OCP_ON                       (1u << 5)
#define SX127X_OCP_TRIM_MASK                0x1Fu
/** OcpTrim encoding:
 *  Imax = 45 + 5*OcpTrim  mA  for OcpTrim <= 15
 *  Imax = -30 + 10*OcpTrim mA for 15 < OcpTrim <= 27
 *  Imax = 240 mA for OcpTrim = 27 */
#define SX127X_OCP_TRIM_120MA               0x0Bu       /**< ~120 mA  */
#define SX127X_OCP_TRIM_240MA               0x1Bu       /**< ~240 mA, +20 dBm mode  */

/* ================================================================
 *  RegLna (0x0C)
 * ================================================================ */

#define SX127X_LNA_GAIN_MASK                0xE0u
#define SX127X_LNA_GAIN_SHIFT               5
#define SX127X_LNA_GAIN_MAX                 (1u << 5)   /**< G1 = maximum gain  */
#define SX127X_LNA_BOOST_HF_MASK            0x03u
#define SX127X_LNA_BOOST_HF_ON              0x03u       /**< +150% LNA current, HF port  */
#define SX127X_LNA_BOOST_HF_OFF             0x00u       /**< default LNA current  */

/* ================================================================
 *  RegDetectOptimize (0x31) / RegDetectionThreshold (0x37)
 *  p.114 of SX1276/77/78/79 datasheet: SF6 requires different values than SF7-SF12
 * ================================================================ */

#define SX127X_DETECT_OPTIMIZE_SF6          0xC5u
#define SX127X_DETECT_OPTIMIZE_SF7_12       0xC3u
#define SX127X_DETECT_THRESHOLD_SF6         0x0Cu
#define SX127X_DETECT_THRESHOLD_SF7_12      0x0Au

/* ================================================================
 *  RegInvertIQ (0x33) / RegInvertIQ2 (0x3B)
 * ================================================================ */

#define SX127X_INVERT_IQ_RX_NORMAL          0x27u
#define SX127X_INVERT_IQ_RX_INVERTED        0x66u
#define SX127X_INVERT_IQ2_NORMAL            0x1Du
#define SX127X_INVERT_IQ2_INVERTED          0x19u

/* ================================================================
 *  RSSI offset (datasheet section 2.5.2)
 * ================================================================ */

#define SX127X_RSSI_OFFSET_LF_PORT          164
#define SX127X_RSSI_OFFSET_HF_PORT          157
#define SX127X_RF_MID_BAND_THRESHOLD_HZ      525000000UL

/* ================================================================
 *  Max LoRa payload (RegPayloadLength is 8-bit)
 * ================================================================ */

#define SX127X_MAX_PKT_LENGTH               255u

/* ================================================================
 *  Chip version
 * ================================================================ */

#define SX127X_VERSION_EXPECTED             0x12u

/* ================================================================
 *  Oscillator / frequency step
 * ================================================================ */

#define SX127X_FXOSC_HZ                     32000000UL
#define SX127X_FSTEP_SHIFT                  19          /**< Fstep = Fosc / 2^19  */