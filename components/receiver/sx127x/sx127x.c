/**
 * @file    sx127x.c
 * @brief   SX1276/77/78/79 driver implementation (LoRa modem only).
 *
 * All register access goes through sx127x_read_reg()/sx127x_write_reg(),
 * which call dev->spi_transfer(). See sx127x.h for the HAL contract.
 */

#include "sx127x.h"
#include "sx127x_regs.h"

/* ================================================================
 *  INTERNAL HELPERS (static, not exposed via sx127x.h)
 * ================================================================ */

/** @brief Write a single register (sets MSB to mark a write transfer). */
static SX127x_Status sx127x_write_reg(SX127x_Dev *dev, uint8_t reg_addr, uint8_t value) {
    if (!dev || !dev->spi_transfer) {
        return SX127X_ERR_INVALID_ARG;
    }

    reg_addr = (reg_addr & 0x7Fu) | 0x80u;

    return (dev->spi_transfer(reg_addr, &value, NULL, 1) == 0) ? SX127X_OK : SX127X_ERR_SPI;
}

/** @brief Read a single register (clears MSB to mark a read transfer). */
static SX127x_Status sx127x_read_reg(SX127x_Dev *dev, uint8_t reg_addr, uint8_t *value) {
    if (!dev || !dev->spi_transfer || !value) {
        return SX127X_ERR_INVALID_ARG;
    }

    reg_addr &= 0x7Fu;

    return (dev->spi_transfer(reg_addr, NULL, value, 1) == 0) ? SX127X_OK : SX127X_ERR_SPI;
}

/**
 * @brief Write register, but skip if a previous error was already recorded.
 *
 * Lets several writes be chained and checked once at the end:
 * @code
 *   SX127x_Status s = SX127X_OK;
 *   sx127x_write_reg_checked(dev, REG_A, val_a, &s);
 *   sx127x_write_reg_checked(dev, REG_B, val_b, &s);
 *   if (s != SX127X_OK) return s;
 * @endcode
 */
static void sx127x_write_reg_checked(SX127x_Dev *dev, uint8_t reg, uint8_t data, SX127x_Status *status) {
    if (*status != SX127X_OK) {
        return;
    }
    *status = sx127x_write_reg(dev, reg, data);
}

/**
 * @brief Recompute and apply LowDataRateOptimize (RegModemConfig3 bit 3) based on
 *        the bandwidth/SF currently written to the chip.
 *
 * Datasheet 4.1.1.6: LDO must be enabled when symbol duration > 16ms.
 * symbol_duration_ms = 1000 * 2^SF / BW(Hz).
 *
 * Re-reads MC1/MC2 from the chip instead of trusting dev->config, so it stays
 * correct even if a prior write partially failed.
 */
static SX127x_Status sx127x_update_ldo_flag(SX127x_Dev *dev) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }

    SX127x_Status s;
    uint8_t mc1 = 0, mc2 = 0, mc3 = 0;

    s = sx127x_read_reg(dev, SX127X_LORA_REG_MODEM_CONFIG1, &mc1);
    if (s != SX127X_OK) {
        return s;
    }

    s = sx127x_read_reg(dev, SX127X_LORA_REG_MODEM_CONFIG2, &mc2);
    if (s != SX127X_OK) {
        return s;
    }

    uint8_t bw_code = (mc1 & SX127X_MC1_BW_MASK) >> SX127X_MC1_BW_SHIFT;
    uint8_t sf      = (mc2 & SX127X_MC2_SF_MASK) >> SX127X_MC2_SF_SHIFT;

    uint32_t bw_hz;
    switch (bw_code) {
        case SX127X_LORA_BW_7K8:   bw_hz = 7800;   break;
        case SX127X_LORA_BW_10K4:  bw_hz = 10400;  break;
        case SX127X_LORA_BW_15K6:  bw_hz = 15600;  break;
        case SX127X_LORA_BW_20K8:  bw_hz = 20800;  break;
        case SX127X_LORA_BW_31K25: bw_hz = 31250;  break;
        case SX127X_LORA_BW_41K7:  bw_hz = 41700;  break;
        case SX127X_LORA_BW_62K5:  bw_hz = 62500;  break;
        case SX127X_LORA_BW_125K:  bw_hz = 125000; break;
        case SX127X_LORA_BW_250K:  bw_hz = 250000; break;
        case SX127X_LORA_BW_500K:  bw_hz = 500000; break;
        default:
            return SX127X_ERR_INVALID_ARG;
    }

    /* symbol_duration_us = 1e6 * 2^sf / bw_hz, integer-safe */
    uint32_t symbol_duration_us = (uint32_t)(((uint64_t)1000000u * (1u << sf)) / bw_hz);
    bool ldo_on = symbol_duration_us > 16000u;

    s = sx127x_read_reg(dev, SX127X_LORA_REG_MODEM_CONFIG3, &mc3);
    if (s != SX127X_OK) {
        return s;
    }

    if (ldo_on) {
        mc3 |= SX127X_MC3_LOW_DATA_RATE_OPTIMIZE;
    } else {
        mc3 &= ~SX127X_MC3_LOW_DATA_RATE_OPTIMIZE;
    }

    return sx127x_write_reg(dev, SX127X_LORA_REG_MODEM_CONFIG3, mc3);
}

/* ================================================================
 *  LIFECYCLE
 * ================================================================ */

SX127x_Status SX127x_Reset(SX127x_Dev *dev, SX127x_ResetMode mode) {
    if (!dev || !dev->delay_ms) {
        return SX127X_ERR_INVALID_ARG;
    }

    if (mode == SX127X_RESET_MANUAL) {
        if (!dev->set_nreset || !dev->delay_us) {
            return SX127X_ERR_INVALID_ARG;
        }
        dev->set_nreset(0);
        dev->delay_us(100);
        dev->set_nreset(1);
        dev->delay_ms(5);
    } else {
        /* POR: chip resets itself on VCC ramp-up, just wait. */
        dev->delay_ms(10);
    }

    return SX127X_OK;
}

SX127x_Status SX127x_Init(SX127x_Dev *dev, const SX127x_Config *config) {
    if (!dev || !config) {
        return SX127X_ERR_INVALID_ARG;
    }
    if (config->modem != SX127X_MODEM_LORA) {
        return SX127X_ERR_INVALID_MODE;  /* FSK not implemented */
    }

    SX127x_Status s;
    uint8_t version = 0;

    s = sx127x_read_reg(dev, SX127X_REG_VERSION, &version);
    if (s != SX127X_OK) {
        return s;
    }
    if (version != SX127X_VERSION_EXPECTED) {
        return SX127X_ERR_NO_DEVICE;
    }

    /* Sleep + LoRa mode must be set together; LongRangeMode can only be
     * changed while in Sleep (datasheet 4.2). */
    s = sx127x_write_reg(dev, SX127X_REG_OP_MODE, SX127X_OPMODE_LONG_RANGE_MODE | SX127X_MODE_SLEEP);
    if (s != SX127X_OK) {
        return s;
    }
    dev->delay_ms(10);
    dev->mode  = SX127X_MODE_SLEEP;
    dev->modem = SX127X_MODEM_LORA;

    s = sx127x_write_reg(dev, SX127X_LORA_REG_FIFO_TX_BASE_ADDR, 0x00);
    if (s != SX127X_OK) {
        return s;
    }
    s = sx127x_write_reg(dev, SX127X_LORA_REG_FIFO_RX_BASE_ADDR, 0x00);
    if (s != SX127X_OK) {
        return s;
    }

    /* LNA boost HF, AGC auto on by default */
    uint8_t lna = 0;
    s = sx127x_read_reg(dev, SX127X_REG_LNA, &lna);
    if (s != SX127X_OK) {
        return s;
    }
    s = sx127x_write_reg(dev, SX127X_REG_LNA, lna | SX127X_LNA_BOOST_HF_ON);
    if (s != SX127X_OK) {
        return s;
    }
    s = sx127x_write_reg(dev, SX127X_LORA_REG_MODEM_CONFIG3, SX127X_MC3_AGC_AUTO_ON);
    if (s != SX127X_OK) {
        return s;
    }

    dev->config = *config;

    s = SX127x_SetConfig(dev, config);
    if (s != SX127X_OK) {
        return s;
    }

    return SX127x_Standby(dev);
}

SX127x_Status SX127x_SetConfig(SX127x_Dev *dev, const SX127x_Config *config) {
    if (!dev || !config) {
        return SX127X_ERR_INVALID_ARG;
    }
    if (config->modem != SX127X_MODEM_LORA) {
        return SX127X_ERR_INVALID_MODE;
    }

    SX127x_Status s;
    const SX127x_LoRaConfig *lora = &config->lora;

    s = SX127x_SetFrequency(dev, lora->frequency_hz);
    if (s != SX127X_OK) {
        return s;
    }

    s = SX127x_SetImplicitHeader(dev, lora->implicit_header);
    if (s != SX127X_OK) {
        return s;
    }

    s = SX127x_SetBandwidth(dev, lora->bandwidth);
    if (s != SX127X_OK) {
        return s;
    }

    s = SX127x_SetCodingRate(dev, lora->coding_rate);
    if (s != SX127X_OK) {
        return s;
    }

    s = SX127x_SetSpreadingFactor(dev, lora->spreading_factor);
    if (s != SX127X_OK) {
        return s;
    }

    s = SX127x_SetPreambleLength(dev, lora->preamble_length);
    if (s != SX127X_OK) {
        return s;
    }

    s = SX127x_SetSyncWord(dev, lora->sync_word);
    if (s != SX127X_OK) {
        return s;
    }

    s = SX127x_SetCrc(dev, lora->crc_on);
    if (s != SX127X_OK) {
        return s;
    }

    s = SX127x_SetTxPower(dev, lora->tx_power_dbm, false);
    if (s != SX127X_OK) {
        return s;
    }

    dev->config = *config;
    return SX127X_OK;
}

/* ================================================================
 *  PARAMETER SETTERS
 * ================================================================ */

SX127x_Status SX127x_SetFrequency(SX127x_Dev *dev, uint32_t freq_hz) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }
    if (freq_hz == 0) {
        return SX127X_ERR_INVALID_ARG;
    }

    SX127x_Status s;
    uint64_t frf = ((uint64_t)freq_hz << SX127X_FSTEP_SHIFT) / SX127X_FXOSC_HZ;

    s = sx127x_write_reg(dev, SX127X_REG_FRF_MSB, (uint8_t)((frf >> 16) & 0xFFu));
    if (s != SX127X_OK) {
        return s;
    }
    s = sx127x_write_reg(dev, SX127X_REG_FRF_MID, (uint8_t)((frf >> 8) & 0xFFu));
    if (s != SX127X_OK) {
        return s;
    }
    s = sx127x_write_reg(dev, SX127X_REG_FRF_LSB, (uint8_t)(frf & 0xFFu));
    if (s != SX127X_OK) {
        return s;
    }

    dev->config.lora.frequency_hz = freq_hz;
    return SX127X_OK;
}

SX127x_Status SX127x_SetBandwidth(SX127x_Dev *dev, SX127x_LoRaBW bw) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }
    if (bw > SX127X_LORA_BW_500K) {
        return SX127X_ERR_INVALID_ARG;
    }

    SX127x_Status s;
    uint8_t value;

    s = sx127x_read_reg(dev, SX127X_LORA_REG_MODEM_CONFIG1, &value);
    if (s != SX127X_OK) {
        return s;
    }

    value &= ~SX127X_MC1_BW_MASK;
    value |= ((uint8_t)bw << SX127X_MC1_BW_SHIFT) & SX127X_MC1_BW_MASK;

    s = sx127x_write_reg(dev, SX127X_LORA_REG_MODEM_CONFIG1, value);
    if (s != SX127X_OK) {
        return s;
    }

    s = sx127x_update_ldo_flag(dev);
    if (s != SX127X_OK) {
        return s;
    }

    dev->config.lora.bandwidth = bw;
    return SX127X_OK;
}

SX127x_Status SX127x_SetSpreadingFactor(SX127x_Dev *dev, SX127x_LoRaSF sf) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }
    if (sf < SX127X_LORA_SF6 || sf > SX127X_LORA_SF12) {
        return SX127X_ERR_INVALID_ARG;
    }
    if (sf == SX127X_LORA_SF6 && dev->config.lora.implicit_header == false) {
        return SX127X_ERR_INVALID_ARG;  /* SF6 requires implicit header mode */
    }

    SX127x_Status s = SX127X_OK;

    if (sf == SX127X_LORA_SF6) {
        sx127x_write_reg_checked(dev, SX127X_LORA_REG_DETECT_OPTIMIZE, SX127X_DETECT_OPTIMIZE_SF6, &s);
        sx127x_write_reg_checked(dev, SX127X_LORA_REG_DETECTION_THRESHOLD, SX127X_DETECT_THRESHOLD_SF6, &s);
    } else {
        sx127x_write_reg_checked(dev, SX127X_LORA_REG_DETECT_OPTIMIZE, SX127X_DETECT_OPTIMIZE_SF7_12, &s);
        sx127x_write_reg_checked(dev, SX127X_LORA_REG_DETECTION_THRESHOLD, SX127X_DETECT_THRESHOLD_SF7_12, &s);
    }
    if (s != SX127X_OK) {
        return s;
    }

    uint8_t value;
    s = sx127x_read_reg(dev, SX127X_LORA_REG_MODEM_CONFIG2, &value);
    if (s != SX127X_OK) {
        return s;
    }

    value &= ~SX127X_MC2_SF_MASK;
    value |= ((uint8_t)sf << SX127X_MC2_SF_SHIFT) & SX127X_MC2_SF_MASK;

    s = sx127x_write_reg(dev, SX127X_LORA_REG_MODEM_CONFIG2, value);
    if (s != SX127X_OK) {
        return s;
    }

    s = sx127x_update_ldo_flag(dev);
    if (s != SX127X_OK) {
        return s;
    }

    dev->config.lora.spreading_factor = sf;
    return SX127X_OK;
}

SX127x_Status SX127x_SetCodingRate(SX127x_Dev *dev, SX127x_LoRaCR cr) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }
    if (cr < SX127X_LORA_CR_4_5 || cr > SX127X_LORA_CR_4_8) {
        return SX127X_ERR_INVALID_ARG;
    }

    SX127x_Status s;
    uint8_t value;

    s = sx127x_read_reg(dev, SX127X_LORA_REG_MODEM_CONFIG1, &value);
    if (s != SX127X_OK) {
        return s;
    }

    value &= ~SX127X_MC1_CR_MASK;
    value |= ((uint8_t)cr << SX127X_MC1_CR_SHIFT) & SX127X_MC1_CR_MASK;

    s = sx127x_write_reg(dev, SX127X_LORA_REG_MODEM_CONFIG1, value);
    if (s != SX127X_OK) {
        return s;
    }

    dev->config.lora.coding_rate = cr;
    return SX127X_OK;
}

SX127x_Status SX127x_SetPreambleLength(SX127x_Dev *dev, uint16_t preamble_length) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }
    if (preamble_length < 6) {
        return SX127X_ERR_INVALID_ARG;
    }

    SX127x_Status s;

    s = sx127x_write_reg(dev, SX127X_LORA_REG_PREAMBLE_MSB, (uint8_t)((preamble_length >> 8) & 0xFFu));
    if (s != SX127X_OK) {
        return s;
    }
    s = sx127x_write_reg(dev, SX127X_LORA_REG_PREAMBLE_LSB, (uint8_t)(preamble_length & 0xFFu));
    if (s != SX127X_OK) {
        return s;
    }

    dev->config.lora.preamble_length = preamble_length;
    return SX127X_OK;
}

SX127x_Status SX127x_SetSyncWord(SX127x_Dev *dev, uint8_t sync_word) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }

    SX127x_Status s = sx127x_write_reg(dev, SX127X_LORA_REG_SYNC_WORD, sync_word);
    if (s != SX127X_OK) {
        return s;
    }

    dev->config.lora.sync_word = sync_word;
    return SX127X_OK;
}

SX127x_Status SX127x_SetCrc(SX127x_Dev *dev, bool enable) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }

    uint8_t value;
    SX127x_Status s = sx127x_read_reg(dev, SX127X_LORA_REG_MODEM_CONFIG2, &value);
    if (s != SX127X_OK) {
        return s;
    }

    if (enable) {
        value |= SX127X_MC2_RX_PAYLOAD_CRC_ON;
    } else {
        value &= ~SX127X_MC2_RX_PAYLOAD_CRC_ON;
    }

    s = sx127x_write_reg(dev, SX127X_LORA_REG_MODEM_CONFIG2, value);
    if (s != SX127X_OK) {
        return s;
    }

    dev->config.lora.crc_on = enable;
    return SX127X_OK;
}

SX127x_Status SX127x_SetImplicitHeader(SX127x_Dev *dev, bool enable) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }

    uint8_t value;
    SX127x_Status s = sx127x_read_reg(dev, SX127X_LORA_REG_MODEM_CONFIG1, &value);
    if (s != SX127X_OK) {
        return s;
    }

    if (enable) {
        value |= SX127X_MC1_IMPLICIT_HEADER;
    } else {
        value &= ~SX127X_MC1_IMPLICIT_HEADER;
    }

    s = sx127x_write_reg(dev, SX127X_LORA_REG_MODEM_CONFIG1, value);
    if (s != SX127X_OK) {
        return s;
    }

    dev->config.lora.implicit_header = enable;
    return SX127X_OK;
}

SX127x_Status SX127x_SetTxPower(SX127x_Dev *dev, int8_t level_dbm, bool use_rfo) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }

    SX127x_Status s;

    if (use_rfo) {
        if (level_dbm < 0) {
            level_dbm = 0;
        } else if (level_dbm > 14) {
            level_dbm = 14;
        }
        s = sx127x_write_reg(dev, SX127X_REG_PA_CONFIG, 0x70u | (uint8_t)level_dbm);
        if (s != SX127X_OK) {
            return s;
        }
    } else {
        if (level_dbm > 17) {
            if (level_dbm > 20) {
                level_dbm = 20;
            }
            level_dbm -= 3;  /* 18-20 dBm maps to OutputPower 15-17 in high-power mode */

            s = sx127x_write_reg(dev, SX127X_REG_PA_DAC, SX127X_PADAC_20DBM);
            if (s != SX127X_OK) {
                return s;
            }
            s = SX127x_SetOCP(dev, 140);
            if (s != SX127X_OK) {
                return s;
            }
        } else {
            if (level_dbm < 2) {
                level_dbm = 2;
            }
            s = sx127x_write_reg(dev, SX127X_REG_PA_DAC, SX127X_PADAC_DEFAULT);
            if (s != SX127X_OK) {
                return s;
            }
            s = SX127x_SetOCP(dev, 100);
            if (s != SX127X_OK) {
                return s;
            }
        }

        s = sx127x_write_reg(dev, SX127X_REG_PA_CONFIG, SX127X_PA_SELECT_BOOST | (uint8_t)(level_dbm - 2));
        if (s != SX127X_OK) {
            return s;
        }
    }

    dev->config.lora.tx_power_dbm = level_dbm;
    return SX127X_OK;
}

SX127x_Status SX127x_SetOCP(SX127x_Dev *dev, uint8_t ma) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }

    uint8_t ocp_trim;

    if (ma <= 120) {
        ocp_trim = (uint8_t)((ma - 45) / 5);
    } else if (ma <= 240) {
        ocp_trim = (uint8_t)((ma + 30) / 10);
    } else {
        ocp_trim = 27u;  /* 240 mA ceiling */
    }

    return sx127x_write_reg(dev, SX127X_REG_OCP, SX127X_OCP_ON | (ocp_trim & SX127X_OCP_TRIM_MASK));
}

SX127x_Status SX127x_SetGain(SX127x_Dev *dev, uint8_t gain) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }
    if (gain > 6) {
        gain = 6;
    }

    SX127x_Status s = SX127x_Standby(dev);
    if (s != SX127X_OK) {
        return s;
    }

    if (gain == 0) {
        return sx127x_write_reg(dev, SX127X_LORA_REG_MODEM_CONFIG3, SX127X_MC3_AGC_AUTO_ON);
    }

    s = sx127x_write_reg(dev, SX127X_LORA_REG_MODEM_CONFIG3, 0x00);  /* disable AGC */
    if (s != SX127X_OK) {
        return s;
    }

    uint8_t lna;
    s = sx127x_read_reg(dev, SX127X_REG_LNA, &lna);
    if (s != SX127X_OK) {
        return s;
    }

    lna &= ~SX127X_LNA_BOOST_HF_MASK;
    lna |= SX127X_LNA_BOOST_HF_ON;

    lna &= ~SX127X_LNA_GAIN_MASK;
    lna |= ((uint8_t)gain << SX127X_LNA_GAIN_SHIFT) & SX127X_LNA_GAIN_MASK;

    return sx127x_write_reg(dev, SX127X_REG_LNA, lna);
}

SX127x_Status SX127x_SetInvertIQ(SX127x_Dev *dev, bool invert) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }

    SX127x_Status s;
    if (invert) {
        s = sx127x_write_reg(dev, SX127X_LORA_REG_INVERT_IQ, SX127X_INVERT_IQ_RX_INVERTED);
        if (s != SX127X_OK) {
            return s;
        }
        s = sx127x_write_reg(dev, SX127X_LORA_REG_INVERT_IQ2, SX127X_INVERT_IQ2_INVERTED);
    } else {
        s = sx127x_write_reg(dev, SX127X_LORA_REG_INVERT_IQ, SX127X_INVERT_IQ_RX_NORMAL);
        if (s != SX127X_OK) {
            return s;
        }
        s = sx127x_write_reg(dev, SX127X_LORA_REG_INVERT_IQ2, SX127X_INVERT_IQ2_NORMAL);
    }
    return s;
}

SX127x_Status SX127x_SetLowDataRateOptimizeForced(SX127x_Dev *dev, bool enable) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }

    uint8_t mc3 = 0;
    SX127x_Status s = sx127x_read_reg(dev, SX127X_LORA_REG_MODEM_CONFIG3, &mc3);
    if (s != SX127X_OK) {
        return s;
    }

    if (enable) {
        mc3 |= SX127X_MC3_LOW_DATA_RATE_OPTIMIZE;
    } else {
        mc3 &= ~SX127X_MC3_LOW_DATA_RATE_OPTIMIZE;
    }

    return sx127x_write_reg(dev, SX127X_LORA_REG_MODEM_CONFIG3, mc3);
}

/* ================================================================
 *  TX / RX
 * ================================================================ */

bool SX127x_IsTransmitting(SX127x_Dev *dev) {
    if (!dev) {
        return false;
    }

    uint8_t op_mode = 0;
    if (sx127x_read_reg(dev, SX127X_REG_OP_MODE, &op_mode) != SX127X_OK) {
        return false;
    }

    if ((op_mode & SX127X_OPMODE_MODE_MASK) == SX127X_MODE_TX) {
        return true;
    }

    uint8_t irq_flags = 0;
    if (sx127x_read_reg(dev, SX127X_LORA_REG_IRQ_FLAGS, &irq_flags) == SX127X_OK) {
        if (irq_flags & SX127X_IRQ_TX_DONE) {
            sx127x_write_reg(dev, SX127X_LORA_REG_IRQ_FLAGS, SX127X_IRQ_TX_DONE);
        }
    }

    return false;
}

SX127x_Status SX127x_Send(SX127x_Dev *dev, const uint8_t *data, uint8_t len) {
    if (!dev || !data) {
        return SX127X_ERR_INVALID_ARG;
    }
    if (len == 0) {
        return SX127X_ERR_INVALID_ARG;
    }
    if (SX127x_IsTransmitting(dev)) {
        return SX127X_ERR_INVALID_MODE;
    }

    SX127x_Status s = SX127x_Standby(dev);
    if (s != SX127X_OK) {
        return s;
    }

    s = sx127x_write_reg(dev, SX127X_LORA_REG_FIFO_ADDR_PTR, 0x00);
    if (s != SX127X_OK) {
        return s;
    }
    s = sx127x_write_reg(dev, SX127X_LORA_REG_PAYLOAD_LENGTH, 0x00);
    if (s != SX127X_OK) {
        return s;
    }

    for (uint8_t i = 0; i < len; i++) {
        s = sx127x_write_reg(dev, SX127X_REG_FIFO, data[i]);
        if (s != SX127X_OK) {
            return s;
        }
    }

    s = sx127x_write_reg(dev, SX127X_LORA_REG_PAYLOAD_LENGTH, len);
    if (s != SX127X_OK) {
        return s;
    }

    if (dev->on_tx_done) {
        s = sx127x_write_reg(dev, SX127X_REG_DIO_MAPPING1, 0x40);  /* DIO0 -> TxDone */
        if (s != SX127X_OK) {
            return s;
        }
    }

    s = sx127x_write_reg(dev, SX127X_REG_OP_MODE, SX127X_OPMODE_LONG_RANGE_MODE | SX127X_MODE_TX);
    if (s != SX127X_OK) {
        return s;
    }

    dev->mode = SX127X_MODE_TX;
    return SX127X_OK;
}

SX127x_Status SX127x_StartReceive(SX127x_Dev *dev, bool continuous) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }

    SX127x_Status s = sx127x_write_reg(dev, SX127X_REG_DIO_MAPPING1, 0x00);  /* DIO0 -> RxDone */
    if (s != SX127X_OK) {
        return s;
    }

    s = sx127x_write_reg(dev, SX127X_LORA_REG_FIFO_ADDR_PTR, 0x00);
    if (s != SX127X_OK) {
        return s;
    }

    uint8_t mode = continuous ? SX127X_MODE_RXCONTINUOUS : SX127X_MODE_RXSINGLE;
    s = sx127x_write_reg(dev, SX127X_REG_OP_MODE, SX127X_OPMODE_LONG_RANGE_MODE | mode);
    if (s != SX127X_OK) {
        return s;
    }

    dev->mode = mode;
    return SX127X_OK;
}

SX127x_Status SX127x_StopReceive(SX127x_Dev *dev) {
    return SX127x_Standby(dev);
}

SX127x_Status SX127x_StartCAD(SX127x_Dev *dev) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }

    SX127x_Status s = sx127x_write_reg(dev, SX127X_REG_DIO_MAPPING1, 0x80);  /* DIO0 -> CadDone */
    if (s != SX127X_OK) {
        return s;
    }

    s = sx127x_write_reg(dev, SX127X_REG_OP_MODE, SX127X_OPMODE_LONG_RANGE_MODE | SX127X_MODE_CAD);
    if (s != SX127X_OK) {
        return s;
    }

    dev->mode = SX127X_MODE_CAD;
    return SX127X_OK;
}

SX127x_Status SX127x_Receive(SX127x_Dev *dev, SX127x_Packet *packet) {
    if (!dev || !packet || !packet->data) {
        return SX127X_ERR_INVALID_ARG;
    }

    SX127x_Status s;
    uint8_t irq_flags = 0;

    s = sx127x_read_reg(dev, SX127X_LORA_REG_IRQ_FLAGS, &irq_flags);
    if (s != SX127X_OK) {
        return s;
    }

    if ((irq_flags & SX127X_IRQ_RX_DONE) == 0) {
        return SX127X_ERR_NO_PACKET;
    }

    s = sx127x_write_reg(dev, SX127X_LORA_REG_IRQ_FLAGS, irq_flags);  /* write-1-to-clear */
    if (s != SX127X_OK) {
        return s;
    }

    if (irq_flags & SX127X_IRQ_PAYLOAD_CRC_ERROR) {
        return SX127X_ERR_CRC;
    }

    uint8_t length = 0;
    if (dev->config.lora.implicit_header) {
        s = sx127x_read_reg(dev, SX127X_LORA_REG_PAYLOAD_LENGTH, &length);
    } else {
        s = sx127x_read_reg(dev, SX127X_LORA_REG_RX_NB_BYTES, &length);
    }
    if (s != SX127X_OK) {
        return s;
    }

    uint8_t fifo_rx_current_addr = 0;
    s = sx127x_read_reg(dev, SX127X_LORA_REG_FIFO_RX_CURRENT_ADDR, &fifo_rx_current_addr);
    if (s != SX127X_OK) {
        return s;
    }

    s = sx127x_write_reg(dev, SX127X_LORA_REG_FIFO_ADDR_PTR, fifo_rx_current_addr);
    if (s != SX127X_OK) {
        return s;
    }

    for (uint8_t i = 0; i < length; i++) {
        s = sx127x_read_reg(dev, SX127X_REG_FIFO, &packet->data[i]);
        if (s != SX127X_OK) {
            return s;
        }
    }
    packet->length = length;

    int16_t rssi = 0;
    if (SX127x_GetPacketRSSI(dev, &rssi) == SX127X_OK) {
        packet->rssi = rssi;
    }

    int8_t snr_raw = 0;
    if (SX127x_GetPacketSNR(dev, &snr_raw) == SX127X_OK) {
        packet->snr = snr_raw;
    }

    return SX127X_OK;
}

void SX127x_HandleDIO0(SX127x_Dev *dev) {
    if (!dev) {
        return;
    }

    uint8_t irq_flags = 0;
    if (sx127x_read_reg(dev, SX127X_LORA_REG_IRQ_FLAGS, &irq_flags) != SX127X_OK) {
        return;
    }

    sx127x_write_reg(dev, SX127X_LORA_REG_IRQ_FLAGS, irq_flags);

    if (irq_flags & SX127X_IRQ_PAYLOAD_CRC_ERROR) {
        return;  /* drop silently, no callback */
    }

    if (irq_flags & SX127X_IRQ_RX_DONE) {
        if (dev->on_rx_done) {
            uint8_t buf[SX127X_MAX_PKT_LENGTH];
            SX127x_Packet packet = { .data = buf, .length = 0, .rssi = 0, .snr = 0 };

            uint8_t length = 0;
            if (dev->config.lora.implicit_header) {
                sx127x_read_reg(dev, SX127X_LORA_REG_PAYLOAD_LENGTH, &length);
            } else {
                sx127x_read_reg(dev, SX127X_LORA_REG_RX_NB_BYTES, &length);
            }

            uint8_t fifo_rx_current_addr = 0;
            sx127x_read_reg(dev, SX127X_LORA_REG_FIFO_RX_CURRENT_ADDR, &fifo_rx_current_addr);
            sx127x_write_reg(dev, SX127X_LORA_REG_FIFO_ADDR_PTR, fifo_rx_current_addr);

            for (uint16_t i = 0; i < length && i < SX127X_MAX_PKT_LENGTH; i++) {
                sx127x_read_reg(dev, SX127X_REG_FIFO, &buf[i]);
            }
            packet.length = length;

            int16_t rssi = 0;
            if (SX127x_GetPacketRSSI(dev, &rssi) == SX127X_OK) {
                packet.rssi = rssi;
            }

            int8_t snr_raw = 0;
            if (SX127x_GetPacketSNR(dev, &snr_raw) == SX127X_OK) {
                packet.snr = snr_raw;
            }

            dev->on_rx_done(dev, &packet);
        }
    } else if (irq_flags & SX127X_IRQ_TX_DONE) {
        if (dev->on_tx_done) {
            dev->on_tx_done(dev);
        }
    }
}

/* ================================================================
 *  MODE CONTROL
 * ================================================================ */

SX127x_Status SX127x_Sleep(SX127x_Dev *dev) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }
    SX127x_Status s = sx127x_write_reg(dev, SX127X_REG_OP_MODE, SX127X_OPMODE_LONG_RANGE_MODE | SX127X_MODE_SLEEP);
    if (s == SX127X_OK) {
        dev->mode = SX127X_MODE_SLEEP;
    }
    return s;
}

SX127x_Status SX127x_Standby(SX127x_Dev *dev) {
    if (!dev) {
        return SX127X_ERR_INVALID_ARG;
    }
    SX127x_Status s = sx127x_write_reg(dev, SX127X_REG_OP_MODE, SX127X_OPMODE_LONG_RANGE_MODE | SX127X_MODE_STANDBY);
    if (s == SX127X_OK) {
        dev->mode = SX127X_MODE_STANDBY;
    }
    return s;
}

/* ================================================================
 *  STATUS / DIAGNOSTICS
 * ================================================================ */

SX127x_Status SX127x_GetRSSI(SX127x_Dev *dev, int16_t *out_rssi) {
    if (!dev || !out_rssi) {
        return SX127X_ERR_INVALID_ARG;
    }

    uint8_t raw = 0;
    SX127x_Status s = sx127x_read_reg(dev, SX127X_LORA_REG_RSSI_VALUE, &raw);
    if (s != SX127X_OK) {
        return s;
    }

    int offset = (dev->config.lora.frequency_hz < SX127X_RF_MID_BAND_THRESHOLD_HZ) ? SX127X_RSSI_OFFSET_LF_PORT : SX127X_RSSI_OFFSET_HF_PORT;

    *out_rssi = (int16_t)((int)raw - offset);
    return SX127X_OK;
}

SX127x_Status SX127x_GetPacketRSSI(SX127x_Dev *dev, int16_t *out_rssi) {
    if (!dev || !out_rssi) {
        return SX127X_ERR_INVALID_ARG;
    }

    uint8_t raw = 0;
    SX127x_Status s = sx127x_read_reg(dev, SX127X_LORA_REG_PKT_RSSI_VALUE, &raw);
    if (s != SX127X_OK) {
        return s;
    }

    int offset = (dev->config.lora.frequency_hz < SX127X_RF_MID_BAND_THRESHOLD_HZ) ? SX127X_RSSI_OFFSET_LF_PORT : SX127X_RSSI_OFFSET_HF_PORT;

    *out_rssi = (int16_t)((int)raw - offset);
    return SX127X_OK;
}

SX127x_Status SX127x_GetPacketSNR(SX127x_Dev *dev, int8_t *out_snr_raw) {
    if (!dev || !out_snr_raw) {
        return SX127X_ERR_INVALID_ARG;
    }

    uint8_t raw = 0;
    SX127x_Status s = sx127x_read_reg(dev, SX127X_LORA_REG_PKT_SNR_VALUE, &raw);
    if (s != SX127X_OK) {
        return s;
    }

    *out_snr_raw = (int8_t)raw;
    return SX127X_OK;
}

SX127x_Status SX127x_Random(SX127x_Dev *dev, uint8_t *out_value) {
    if (!dev || !out_value) {
        return SX127X_ERR_INVALID_ARG;
    }
    return sx127x_read_reg(dev, SX127X_LORA_REG_RSSI_WIDEBAND, out_value);
}

void SX127x_DumpRegisters(SX127x_Dev *dev, void (*print_fn)(uint8_t addr, uint8_t value)) {
    if (!dev || !print_fn) {
        return;
    }

    for (uint16_t addr = 0; addr < 0x80; addr++) {
        uint8_t value = 0;
        if (sx127x_read_reg(dev, (uint8_t)addr, &value) == SX127X_OK) {
            print_fn((uint8_t)addr, value);
        }
    }
}

const char *SX127x_StatusStr(SX127x_Status status) {
    switch (status) {
        case SX127X_OK:               return "OK";
        case SX127X_ERR_INVALID_ARG:  return "ERR_INVALID_ARG";
        case SX127X_ERR_SPI:          return "ERR_SPI";
        case SX127X_ERR_NO_DEVICE:    return "ERR_NO_DEVICE";
        case SX127X_ERR_INVALID_MODE: return "ERR_INVALID_MODE";
        case SX127X_ERR_CRC:          return "ERR_CRC";
        case SX127X_ERR_NO_PACKET:    return "ERR_NO_PACKET";
        default:                      return "(unknown)";
    }
}