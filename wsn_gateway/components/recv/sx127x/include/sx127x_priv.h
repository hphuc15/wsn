#ifndef SX127X_PRIV_H
#define SX127X_PRIV_H

#include "sx127x_defs.h"

SX127x_Status sx127x_read_reg(SX127x_Dev *dev, uint8_t reg, uint8_t *out);
SX127x_Status sx127x_write_reg(SX127x_Dev *dev, uint8_t reg, uint8_t data);
SX127x_Status sx127x_write_fifo(SX127x_Dev *dev, uint8_t *data, uint8_t len);
SX127x_Status sx127x_read_fifo(SX127x_Dev *dev, uint8_t *out, uint8_t len);

#endif SX127X_PRIV_H