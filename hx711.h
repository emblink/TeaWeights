#ifndef __HX_711_H
#define __HX_711_H

#include "stdint.h"
#include "stdbool.h"

typedef enum Hx711Status {
    Hx711StatusOk,
    Hx711StatusBusy,
    Hx711StatusReady,
    Hx711StatusInitErr,
    Hx711StatusParamErr,
    Hx711StatusPowerErr,
    Hx711StatusCount
} Hx711Status;

typedef enum Hx711Channel {
    Hx711ChannelA128 = 25,
    Hx711ChannelB32 = 26,
    Hx711ChannelA64 = 27,
    Hx711ChannelCount
} Hx711Channel;

/* These callbacks params can be changed according to used HAL */
typedef int32_t (*pinReadCallback)(int32_t pin);
typedef int32_t (*pinWriteCallback)(int32_t pin, uint32_t state);
typedef void (*delayUsCb)(uint32_t us);

typedef struct Hx711Handle {
    uint32_t dataPin;
    uint32_t sclkPin;
    pinReadCallback readCb;
    pinWriteCallback writeCb;
    delayUsCb delayCb;
} Hx711Handle;

Hx711Status hx711Init(Hx711Handle *handle);
Hx711Status hx711GetStatus(void);
Hx711Status hx711ReadChannel(Hx711Channel channel, int32_t *data);
Hx711Status hx711PowerDown(void);
Hx711Status hx711PowerUp(void);
#endif // __HX_711_H