#ifndef STATUS_H
#define STATUS_H

extern volatile WSN_Gateway_Status wsn_gateway_status;

typedef enum {
    WSN_GATEWAY_ONLINE = 0,
    WSN_GATEWAY_CONFIG,
    WSN_GATEWAY_OFFLINE
} WSN_Gateway_Status;

#endif /* STATUS_H */