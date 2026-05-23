#ifndef SX127X_LORA_H
#define SX127X_LORA_H

#include "sx127x_defs.h"
#include "sx127x.h"

/* Lora Configuration */
SX127x_Status SX127x_SetConfig(SX127x_Dev *dev, SX127x_Config *config);
SX127x_Status SX127x_SetFrequency(SX127x_Dev *dev, uint32_t freq_hz);
SX127x_Status SX127x_SetBandwidth(SX127x_Dev *dev, SX127x_LoRa_BW bw);
SX127x_Status SX127x_SetSpreadingFactor(SX127x_Dev *dev, SX127x_LoRa_SF sf);
SX127x_Status SX127x_SetCodingRate(SX127x_Dev *dev, SX127x_LoRa_CR cr);
SX127x_Status SX127x_SetTxPower(SX127x_Dev *dev, uint8_t power_dbm);
SX127x_Status SX127x_SetPreambleLength(SX127x_Dev *dev, uint16_t length);
SX127x_Status SX127x_SetCRC(SX127x_Dev *dev, bool enable);
SX127x_Status SX127x_SetImplicitHeader(SX127x_Dev *dev, bool enable);

#endif /* SX127X_LORA_H */