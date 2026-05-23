#ifndef SX127X_H
#define SX127X_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "sx127x_defs.h"
#include "sx127x_lora.h"

/* PUBLIC APIs */

SX127x_Status SX127x_Init(SX127x_Dev *dev, SX127x_Config *config);
SX127x_Status SX127x_Reset(SX127x_Dev *dev, SX127x_ResetMechanism reset);
SX127x_Status SX127x_SetModem(SX127x_Dev *dev, SX127x_Modem modem);
SX127x_Status SX127x_SetMode(SX127x_Dev *dev, SX127x_OpMode mode);
SX127x_Status SX127x_GetMode(SX127x_Dev *dev, SX127x_OpMode *out);



/* FSK/OOK Configuration */

/* Transfer */

/**
 * @brief TX 
 */
SX127x_Status SX127x_Send(SX127x_Dev *dev, SX127x_Payload *payload);

/**
 * @brief RX
 */
SX127x_Status SX127x_StartReceive(SX127x_Dev *dev, bool continuous);
SX127x_Status SX127x_Recv(SX127x_Dev *dev, SX127x_Payload *out);

void SX127x_OnDIO0(SX127x_Dev *dev);

/**
 * @brief PACKET INFO (đọc sau khi nhận)
 */
SX127x_Status SX127x_GetPacketRSSI(SX127x_Dev *dev, int16_t *out_rssi);
SX127x_Status SX127x_GetPacketSNR (SX127x_Dev *dev, int8_t  *out_snr);

/**
 * @brief MISC
 */
bool SX127x_IsTransmitting(SX127x_Dev *dev);



void DEBUG_(SX127x_Dev *dev, uint8_t reg, uint8_t *out);


#endif /* SX127X_H */


