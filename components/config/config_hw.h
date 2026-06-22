#ifndef CONFIG_HW_H
#define CONFIG_HW_H

#include "sx127x.h"
#include "driver/gpio.h"

/* LoRa pin */
#define CFG_WSN_GATEWAY_LORA_IO_NSS             GPIO_NUM_15
#define CFG_WSN_GATEWAY_LORA_IO_NRESET          GPIO_NUM_14
#define CFG_WSN_GATEWAY_LORA_IO_DIO0            GPIO_NUM_26
#define CFG_WSN_GATEWAY_LORA_IO_MOSI            GPIO_NUM_23
#define CFG_WSN_GATEWAY_LORA_IO_MISO            GPIO_NUM_19
#define CFG_WSN_GATEWAY_LORA_IO_SCK             GPIO_NUM_18
/* LoRa physical properties */
#define CFG_WSN_GATEWAY_LORA_BANDWIDTH          SX127X_LORA_BW_125K
#define CFG_WSN_GATEWAY_LORA_SF                 SX127X_LORA_SF7
#define CFG_WSN_GATEWAY_LORA_PREAMBLE_LENGTH    10
#define CFG_WSN_GATEWAY_LORA_CODING_RATE        SX127X_LORA_CR_4_5
#define CFG_WSN_GATEWAY_LORA_TX_POWER_DBM       20
#define CFG_WSN_GATEWAY_LORA_FREQ_HZ            433000000UL
#define CFG_WSN_GATEWAY_LORA_IMPLICIT_HEADER    0
#define CFG_WSN_GATEWAY_LORA_ENABLE_CRC         1
#define CFG_WSN_GATEWAY_LORA_SYNC_WORD          0x12
/* Gateway config button and status led pin */
#define CFG_WSN_GATEWAY_GPIO_WAKE_BTN           GPIO_NUM_33
#define CFG_WSN_GATEWAY_GPIO_STATUS_LED         GPIO_NUM_4

#endif /* CONFIG_HW_H */