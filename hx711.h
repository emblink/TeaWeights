#ifndef __HX_711_H
#define __HX_711_H

typedef enum {
    Hx711ErrOk,
    Hx711ErrBusy,
    Hx711ErrInit,
    Hx711ErrParam,
    Hx711ErrPower,
    Hx711ErrCount
} Hx711Err;

typedef enum {
    Hx711ChannelA128 = 25,
    Hx711ChannelB32 = 26,
    Hx711ChannelA64 = 27,
    Hx711ChannelCount
} Hx711Channel;

typedef enum {
    Hx711PinData,
    Hx711PinClock,
    Hx711PinCount,
} Hx711Pin;

typedef enum {
    Hx711PinStateLow,
    Hx711PinStateHigh,
    Hx711PinStateCount
} Hx711PinState;

typedef Hx711Err (* Hx711PinWriteCallback)(Hx711Pin pin, Hx711PinState state);
typedef Hx711Err (* Hx711PinReadCallback)(Hx711Pin pin, Hx711PinState *state);
typedef void (* Hx711DelayUsCallback)(unsigned short us);

typedef struct Hx711Handle {
    Hx711PinWriteCallback pinWriteCb;
    Hx711PinReadCallback pinReadCb;
    Hx711DelayUsCallback delayUsCb;
} Hx711Handle;

Hx711Err hx711Init(Hx711Handle *handle);
Hx711Err hx711IsBusy(void);
Hx711Err hx711ReadChannel(Hx711Channel channel, unsigned char data[3]);
Hx711Err hx711PowerDown(void);
Hx711Err hx711PowerUp(void);
#endif // __HX_711_H