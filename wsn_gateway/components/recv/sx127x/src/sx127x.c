#include "sx127x.h"
#include "sx127x_priv.h"

SX127x_Status sx127x_read_reg(SX127x_Dev *dev, uint8_t reg, uint8_t *out){
    if(!dev || !out){
        return SX127X_ERR_INVALID_ARGS;
    }
    return (dev->spi_transfer(reg, NULL, out, 1) == 0) ? SX127X_OK : SX127X_ERR_SPI;
}

SX127x_Status sx127x_write_reg(SX127x_Dev *dev, uint8_t reg, uint8_t data){
    if(!dev){
        return SX127X_ERR_INVALID_ARGS;
    }
    return (dev->spi_transfer(reg | 0x80u, &data, NULL, 1) == 0) ? SX127X_OK : SX127X_ERR_SPI;
}

SX127x_Status sx127x_write_fifo(SX127x_Dev *dev, uint8_t *data, uint8_t len) {
    if (!dev || !data){
        return SX127X_ERR_INVALID_ARGS;
    }
    return (dev->spi_transfer(SX127X_REGFIFO | 0x80u, data, NULL, len) == 0) ? SX127X_OK : SX127X_ERR_SPI;
}

SX127x_Status sx127x_read_fifo(SX127x_Dev *dev, uint8_t *out, uint8_t len) {
    if (!dev || !out){
        return SX127X_ERR_INVALID_ARGS;
    }
    return (dev->spi_transfer(SX127X_REGFIFO, NULL, out, len) == 0) ? SX127X_OK : SX127X_ERR_SPI;
}

/* ================================================================
 *  INIT & RESET
 * ================================================================ */

SX127x_Status SX127x_Init(SX127x_Dev *dev, SX127x_Config *config) {
    if (!dev || !config){
        return SX127X_ERR_INVALID_ARGS;
    }
    if (!dev->spi_transfer) {
        return SX127X_ERR_INVALID_ARGS;
    }

    SX127x_Status status;
    uint8_t version;

    status = sx127x_read_reg(dev, SX127X_REGVERSION, &version);
    if (status != SX127X_OK){
        return status;
    }
    if (version != SX127X_VERSION_DEFAULT){
        return SX127X_ERR_NO_DEVICE;
    }

    status = SX127x_SetModem(dev, dev->modem);
    if (status != SX127X_OK){
        return status;
    }

    status = SX127x_SetConfig(dev, config);
    if (status != SX127X_OK){
        return status;
    }

    SX127x_OpMode standby = { .lora = SX127X_LORA_MODE_STANDBY };
    status = SX127x_SetMode(dev, standby);
    if (status != SX127X_OK){
        return status;
    }

    return SX127X_OK;
}

SX127x_Status SX127x_Reset(SX127x_Dev *dev, SX127x_ResetMechanism reset) {
    if (!dev) return SX127X_ERR_INVALID_ARGS;
    switch (reset) {
        case SX127X_RESET_POR:
            if (!dev->delay_ms){
                return SX127X_ERR_INVALID_ARGS;
            }
            dev->delay_ms(10);
            break;
        case SX127X_RESET_MANUAL:
            if (!dev->set_nreset || !dev->delay_us || !dev->delay_ms){
                return SX127X_ERR_INVALID_ARGS;                
            }
            dev->set_nreset(0);
            dev->delay_us(100);
            dev->set_nreset(-1);
            dev->delay_ms(5);
            break;
        default:
            return SX127X_ERR_INVALID_ARGS;
    }
    return SX127X_OK;
}

/* ================================================================
 *  MODEM & MODE
 * ================================================================ */

/**
 * @brief Set SX127x Modem
 */
SX127x_Status SX127x_SetModem(SX127x_Dev *dev, SX127x_Modem modem) {
    if (!dev){
        return SX127X_ERR_INVALID_ARGS;
    }

    SX127x_Status status;
    uint8_t reg;

    status = sx127x_read_reg(dev, SX127X_REGOPMODE, &reg);
    if (status != SX127X_OK){
        return status;
    }

    status = sx127x_write_reg(dev, SX127X_REGOPMODE, SX127X_REGOPMODE_SET_MODE(reg, 0x00u));
    if (status != SX127X_OK){
        return status;
    }

    reg = SX127X_REGOPMODE_SET_MODEM(reg, modem);
    status = sx127x_write_reg(dev, SX127X_REGOPMODE, reg);
    if (status != SX127X_OK){
        return status;
    }

    dev->modem = modem;
    return SX127X_OK;
}
/**
 * @brief Set SX127x Operation Mode, based on modem
 */
SX127x_Status SX127x_SetMode(SX127x_Dev *dev, SX127x_OpMode mode) {
    if (!dev){
        return SX127X_ERR_INVALID_ARGS;
    }

    uint8_t mode_val;
    switch (dev->modem) {
        case SX127X_MODEM_LORA:
            mode_val = (uint8_t)mode.lora;
            break;
        case SX127X_MODEM_FSK:
            /* Implement in future */
            // mode_val = (uint8_t)mode.fsk;
            return SX127X_ERR_INVALID_MODE;
        default:
            return SX127X_ERR_INVALID_MODE;
    }

    SX127x_Status status;
    uint8_t reg;

    status = sx127x_read_reg(dev, SX127X_REGOPMODE, &reg);
    if (status != SX127X_OK){
        return status;
    }

    status = sx127x_write_reg(dev, SX127X_REGOPMODE, SX127X_REGOPMODE_SET_MODE(reg, mode_val));
    if (status != SX127X_OK){
        return status;
    }

    dev->mode = mode;
    return SX127X_OK;
}

/**
 * @brief Get SX127x Operation Mode
 */
SX127x_Status SX127x_GetMode(SX127x_Dev *dev, SX127x_OpMode *out) {
    if (!dev || !out){
        return SX127X_ERR_INVALID_ARGS;
    }
    *out = dev->mode;
    return SX127X_OK;
}

/* ================================================================
 *  TX
 * ================================================================ */

SX127x_Status SX127x_Send(SX127x_Dev *dev, SX127x_Payload *payload) {
    if (!dev || !payload || !payload->data || payload->length == 0){
        return SX127X_ERR_INVALID_ARGS;        
    }

    SX127x_Status status;

    SX127x_OpMode standby = { .lora = SX127X_LORA_MODE_STANDBY };
    if ((status = SX127x_SetMode(dev, standby)) != SX127X_OK){
        return status;
    }

    if ((status = sx127x_write_reg(dev, SX127X_LORA_REGFIFOTXBASEADDR, 0x00u)) != SX127X_OK){
        return status;
    }
    if ((status = sx127x_write_reg(dev, SX127X_LORA_REGFIFOADDRPTR,    0x00u)) != SX127X_OK){
        return status;
    }
    if ((status = sx127x_write_fifo(dev, payload->data, payload->length))        != SX127X_OK){
        return status;
    }
    if ((status = sx127x_write_reg(dev, SX127X_LORA_REGPAYLOADLENGTH, payload->length)) != SX127X_OK){
        return status;
    }

    SX127x_OpMode tx = { .lora = SX127X_LORA_MODE_TX };
    return SX127x_SetMode(dev, tx);
}

/* ================================================================
 *  RX
 * ================================================================ */

SX127x_Status SX127x_StartReceive(SX127x_Dev *dev, bool continuous) {
    if (!dev){
        return SX127X_ERR_INVALID_ARGS;
    }

    SX127x_Status status;

    if ((status = sx127x_write_reg(dev, SX127X_LORA_REGFIFORXBASEADDR, 0x00u)) != SX127X_OK){
        return status;
    }
    if ((status = sx127x_write_reg(dev, SX127X_LORA_REGFIFOADDRPTR,    0x00u)) != SX127X_OK){
        return status;
    }

    SX127x_OpMode rx = { .lora = continuous ? SX127X_LORA_MODE_RXCONTINUOUS : SX127X_LORA_MODE_RXSINGLE };
    return SX127x_SetMode(dev, rx);
}

SX127x_Status SX127x_Recv(SX127x_Dev *dev, SX127x_Payload *out) {
    if (!dev || !out || !out->data){
        return SX127X_ERR_INVALID_ARGS;
    }

    SX127x_Status status;
    uint8_t irq;

    status = sx127x_read_reg(dev, SX127X_LORA_REGIRQFLAGS, &irq);
    if (status != SX127X_OK){
        return status;
    }

    /* Clear all IRQ flags */
    sx127x_write_reg(dev, SX127X_LORA_REGIRQFLAGS, 0xFFu);

    if (!(irq & SX127X_LORA_IRQ_RXDONE)){
        return SX127X_FAIL;
    }
    if (  irq & SX127X_LORA_IRQ_PAYLOADCRCERROR){
        return SX127X_ERR_CRC;
    }

    uint8_t nb_bytes, rx_addr;
    if ((status = sx127x_read_reg(dev, SX127X_LORA_REGRXNBBYTES, &nb_bytes)) != SX127X_OK){
        return status;
    }
    if ((status = sx127x_read_reg(dev, SX127X_LORA_REGFIFORXCURRENTADDR, &rx_addr)) != SX127X_OK){
        return status;
    }
    if ((status = sx127x_write_reg(dev, SX127X_LORA_REGFIFOADDRPTR, rx_addr)) != SX127X_OK){
        return status;
    }

    out->length = nb_bytes;
    return sx127x_read_fifo(dev, out->data, nb_bytes);
}

/* ================================================================
 *  IRQ — gọi trong GPIO interrupt handler của DIO0
 * ================================================================ */

void SX127x_OnDIO0(SX127x_Dev *dev) {
    if (!dev){
        return;
    }

    uint8_t irq;
    if (sx127x_read_reg(dev, SX127X_LORA_REGIRQFLAGS, &irq) != SX127X_OK){
        return;
    }
    sx127x_write_reg(dev, SX127X_LORA_REGIRQFLAGS, 0xFFu);

    if (irq & SX127X_LORA_IRQ_TXDONE) {
        SX127x_OpMode standby = { .lora = SX127X_LORA_MODE_STANDBY };
        SX127x_SetMode(dev, standby);
        if (dev->txdone_cb){
            dev->txdone_cb(dev);
        }
        return;
    }

    if (irq & SX127X_LORA_IRQ_RXDONE) {
        if (!dev->rxdone_cb){
            return;
        }

        int16_t rssi; int8_t snr;
        SX127x_GetPacketRSSI(dev, &rssi);
        SX127x_GetPacketSNR(dev, &snr);

        uint8_t nb_bytes, rx_addr;
        if (sx127x_read_reg(dev, SX127X_LORA_REGRXNBBYTES, &nb_bytes) != SX127X_OK){
            return;
        }
        if (sx127x_read_reg(dev, SX127X_LORA_REGFIFORXCURRENTADDR, &rx_addr) != SX127X_OK){
            return;
        }
        if (sx127x_write_reg(dev, SX127X_LORA_REGFIFOADDRPTR, rx_addr) != SX127X_OK){
            return;
        }

        uint8_t buf[256];
        if (sx127x_read_fifo(dev, buf, nb_bytes) != SX127X_OK){
            return;
        }

        if (!(irq & SX127X_LORA_IRQ_PAYLOADCRCERROR)) {
            dev->rxdone_cb(dev, buf, nb_bytes, rssi, snr);
        }
    }
}

/* ================================================================
 *  PACKET INFO
 * ================================================================ */

SX127x_Status SX127x_GetPacketRSSI(SX127x_Dev *dev, int16_t *out_rssi) {
    if (!dev || !out_rssi){
        return SX127X_ERR_INVALID_ARGS;
    }

    uint8_t reg;
    SX127x_Status status = sx127x_read_reg(dev, SX127X_LORA_REGPKTRSSIVALUE, &reg);
    if (status != SX127X_OK){
        return status;
    }

    /* HF port (>779 MHz): -157 + reg
     * LF port (≤779 MHz): -164 + reg  */
    *out_rssi = (int16_t)(-157 + reg);
    return SX127X_OK;
}

SX127x_Status SX127x_GetPacketSNR(SX127x_Dev *dev, int8_t *out_snr) {
    if (!dev || !out_snr){
        return SX127X_ERR_INVALID_ARGS;
    }

    uint8_t reg;
    SX127x_Status status = sx127x_read_reg(dev, SX127X_LORA_REGPKTSNRVALUE, &reg);
    if (status != SX127X_OK){
        return status;
    }

    /* SNR = RegPktSnrValue / 4  (đơn vị dB, có dấu) */
    *out_snr = (int8_t)((int8_t)reg >> 2);
    return SX127X_OK;
}

/* ================================================================
 *  MISC
 * ================================================================ */

bool SX127x_IsTransmitting(SX127x_Dev *dev) {
    if (!dev){
        return false;
    }
    uint8_t reg;
    if (sx127x_read_reg(dev, SX127X_REGOPMODE, &reg) != SX127X_OK){
        return false;
    }
    return (reg & SX127X_REGOPMODE_MODE_MASK) == (uint8_t)SX127X_LORA_MODE_TX;
}



/* Helper func */

/**
 * @brief Convert SX127x_Status to string.
 */
const char *SX127x_StatusToName(SX127x_Status code){
    switch(code){
        case SX127X_OK:
            return "SX127X_OK";
        case SX127X_FAIL:
            return "SX127X_FAIL";

        default:
            return NULL;
    }
}