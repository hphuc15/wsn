#include "sx127x_lora.h"
#include "sx127x_priv.h"

/**
 * @brief Set frequency_hz
 */
SX127x_Status SX127x_SetFrequency(SX127x_Dev *dev, uint32_t frequency_hz) {
    if (!dev){
        return SX127X_ERR_INVALID_ARGS;
    }

    uint32_t frf = SX127X_CALC_FRF(frequency_hz);
    SX127x_Status status;

    if ((status = sx127x_write_reg(dev, SX127X_REGFRFMSB, SX127X_FRFMSB(frf))) != SX127X_OK){
        return status;
    }
    if ((status = sx127x_write_reg(dev, SX127X_REGFRFMID, SX127X_FRFMID(frf))) != SX127X_OK){
        return status;
    }
    if ((status = sx127x_write_reg(dev, SX127X_REGFRFLSB, SX127X_FRFLSB(frf))) != SX127X_OK){
        return status;
    }

    return SX127X_OK;
}

/**
 * @brief Set bandwidth
 */
SX127x_Status SX127x_SetBandwidth(SX127x_Dev *dev, SX127x_LoRa_BW bw) {
    if (!dev){
        return SX127X_ERR_INVALID_ARGS;
    }
    if (dev->modem != SX127X_MODEM_LORA){
        return SX127X_ERR_INVALID_MODE;
    }

    SX127x_Status status;
    uint8_t reg;

    status = sx127x_read_reg(dev, SX127X_LORA_REGMODEMCONFIG1, &reg);
    if (status != SX127X_OK){
        return status;
    }

    reg = (reg & ~0xF0u) | ((uint8_t)(bw) << 4);
    return sx127x_write_reg(dev, SX127X_LORA_REGMODEMCONFIG1, reg);
}

/**
 * @brief Set spreading_factor
 */
SX127x_Status SX127x_SetSpreadingFactor(SX127x_Dev *dev, SX127x_LoRa_SF sf) {
    if (!dev){
        return SX127X_ERR_INVALID_ARGS;
    }
    if (dev->modem != SX127X_MODEM_LORA){
        return SX127X_ERR_INVALID_MODE;
    }
    if (sf < SX127X_LORA_SF_6 || sf > SX127X_LORA_SF_12){
        return SX127X_ERR_INVALID_ARGS;
    }

    SX127x_Status status;
    uint8_t reg;

    status = sx127x_read_reg(dev, SX127X_LORA_REGMODEMCONFIG2, &reg);
    if (status != SX127X_OK){
        return status;
    }

    reg = (reg & ~0xF0u) | ((uint8_t)(sf) << 4);
    return sx127x_write_reg(dev, SX127X_LORA_REGMODEMCONFIG2, reg);
}

/**
 * @brief Set coding_rate
 */
SX127x_Status SX127x_SetCodingRate(SX127x_Dev *dev, SX127x_LoRa_CR cr) {
    if (!dev){
        return SX127X_ERR_INVALID_ARGS;
    }
    if (dev->modem != SX127X_MODEM_LORA){
        return SX127X_ERR_INVALID_MODE;
    }

    SX127x_Status status;
    uint8_t reg;

    status = sx127x_read_reg(dev, SX127X_LORA_REGMODEMCONFIG1, &reg);
    if (status != SX127X_OK){
        return status;
    }

    reg = (reg & ~0x0Eu) | ((uint8_t)(cr) << 1);
    return sx127x_write_reg(dev, SX127X_LORA_REGMODEMCONFIG1, reg);
}

SX127x_Status SX127x_SetPreambleLength(SX127x_Dev *dev, uint16_t length) {
    if (!dev){
        return SX127X_ERR_INVALID_ARGS;
    }
    if (dev->modem != SX127X_MODEM_LORA){
        return SX127X_ERR_INVALID_MODE;
    }

    SX127x_Status status;
    if ((status = sx127x_write_reg(dev, SX127X_LORA_REGPREAMBLEMSB, (uint8_t)(length >> 8)))   != SX127X_OK){
        return status;
    }
    if ((status = sx127x_write_reg(dev, SX127X_LORA_REGPREAMBLELSB, (uint8_t)(length & 0xFFu))) != SX127X_OK){
        return status;
    }
    return SX127X_OK;
}

SX127x_Status SX127x_SetTxPower(SX127x_Dev *dev, uint8_t power_dbm) {
    if (!dev){
        return SX127X_ERR_INVALID_ARGS;
    }

    /* PA_BOOST (bit7=1): Pout = 2 + OutputPower, range 2–17 dBm */
    if (power_dbm < 2){
        power_dbm = 2;
    }
    if (power_dbm > 17){
        power_dbm = 17;
    }

    return sx127x_write_reg(dev, SX127X_REGPACONFIG, 0x80u | (uint8_t)(power_dbm - 2));
}

/**
 * @brief Set crc_enable
 */
SX127x_Status SX127x_SetCRC(SX127x_Dev *dev, bool enable) {
    if (!dev){
        return SX127X_ERR_INVALID_ARGS;
    }
    if (dev->modem != SX127X_MODEM_LORA){
        return SX127X_ERR_INVALID_MODE;
    }

    SX127x_Status status;
    uint8_t reg;

    status = sx127x_read_reg(dev, SX127X_LORA_REGMODEMCONFIG2, &reg);
    if (status != SX127X_OK){
        return status;
    }

    reg = enable ? (reg | 0x04u) : (reg & ~0x04u);
    return sx127x_write_reg(dev, SX127X_LORA_REGMODEMCONFIG2, reg);
}

/**
 * @brief Set sync_word
 */
SX127x_Status SX127x_SetImplicitHeader(SX127x_Dev *dev, bool enable) {
    if (!dev){
        return SX127X_ERR_INVALID_ARGS;
    }
    if (dev->modem != SX127X_MODEM_LORA){
        return SX127X_ERR_INVALID_MODE;
    }

    SX127x_Status status;
    uint8_t reg;

    status = sx127x_read_reg(dev, SX127X_LORA_REGMODEMCONFIG1, &reg);
    if (status != SX127X_OK){
        return status;
    }

    reg = enable ? (reg | 0x01u) : (reg & ~0x01u);
    return sx127x_write_reg(dev, SX127X_LORA_REGMODEMCONFIG1, reg);
}


SX127x_Status SX127x_SetSyncWord(SX127x_Dev *dev, uint8_t sync_word) {
    if (!dev){
        return SX127X_ERR_INVALID_ARGS;
    }
    if (dev->modem != SX127X_MODEM_LORA){
        return SX127X_ERR_INVALID_MODE;
    }

    return sx127x_write_reg(dev, SX127X_LORA_REGSYNCWORD, sync_word);
}

SX127x_Status SX127x_SetConfig(SX127x_Dev *dev, SX127x_Config *config) {
    if (!dev || !config){
        return SX127X_ERR_INVALID_ARGS;
    }
    if (dev->modem != SX127X_MODEM_LORA){
        return SX127X_ERR_INVALID_MODE;
    }

    SX127x_LoRa_Config *lora = &config->lora;
    SX127x_Status status;

    if ((status = SX127x_SetFrequency(dev, lora->frequency_hz)) != SX127X_OK){
        return status;
    }
    if ((status = SX127x_SetBandwidth(dev, lora->bandwidth))!= SX127X_OK){
        return status;
    }
    if ((status = SX127x_SetSpreadingFactor(dev, lora->spreading_factor))!= SX127X_OK){
        return status;
    }
    if ((status = SX127x_SetCodingRate(dev, lora->coding_rate))!= SX127X_OK){
        return status;
    }
    if ((status = SX127x_SetPreambleLength(dev, lora->preamble_length))!= SX127X_OK){
        return status;
    }
    if ((status = SX127x_SetTxPower(dev, lora->tx_power_dbm))!= SX127X_OK){
        return status;
    }
    if ((status = SX127x_SetCRC(dev, lora->crc_enable))!= SX127X_OK){
        return status;
    }
    if ((status = SX127x_SetImplicitHeader(dev, lora->implicit_header))!= SX127X_OK){
        return status;
    }

    if ((status = SX127x_SetSyncWord(dev, lora->sync_word)) != SX127X_OK){
        return status;
    }
    
    return SX127X_OK;
}