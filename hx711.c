#include "hx711.h"
#define NULL ((void *) 0)

Hx711Handle hx711Handle = {0};
unsigned char isPowerDown = 0;

Hx711Err hx711Init(Hx711Handle *handle)
{
    if (handle == NULL || handle->pinReadCb == NULL  || handle->pinWriteCb == NULL || handle->delayUsCb == NULL)
        return Hx711ErrInit;
    
    hx711Handle.pinReadCb = handle->pinReadCb;
    hx711Handle.pinWriteCb = handle->pinWriteCb;
    hx711Handle.delayUsCb = handle->delayUsCb;
    return Hx711ErrOk;
}

Hx711Err hx711GetStatus(void)
{
    if (isPowerDown)
        return Hx711ErrPower;

    Hx711PinState pinState;
    Hx711Err err = hx711Handle.pinReadCb(Hx711PinData, &pinState);
    if (err == Hx711ErrOk) {
        return pinState == Hx711PinStateLow ? Hx711ErrOk : Hx711ErrBusy;
    }
    return err;
}

static unsigned char hx711ReadBit(void)
{
    hx711Handle.pinWriteCb(Hx711PinClock, Hx711PinStateHigh);
    hx711Handle.delayUsCb(1);
    Hx711PinState pinState;
    hx711Handle.pinReadCb(Hx711PinData, &pinState);
    unsigned char bit = pinState == Hx711PinStateHigh ? 1 : 0;
    hx711Handle.pinWriteCb(Hx711PinClock, Hx711PinStateLow);
    hx711Handle.delayUsCb(1);
    return bit;
}

Hx711Err hx711ReadChannel(Hx711Channel channel, unsigned char data[3])
{
    if (channel < Hx711ChannelA128 || channel > Hx711ChannelA64 || data == NULL)
        return Hx711ErrParam;
    
    if (isPowerDown)
        return Hx711ErrPower;

    *data = 0;
    for (unsigned char i = 0; i < 3; i++) {
        for (unsigned char j = 0; j < 8; j++) {
            data[i] <<= 1;
            data[i] |= hx711ReadBit();
        }
    }

    for (unsigned char i = 0; i < channel - 24; i++) {
        hx711Handle.pinWriteCb(Hx711PinClock, Hx711PinStateHigh);
        hx711Handle.delayUsCb(1);
        hx711Handle.pinWriteCb(Hx711PinClock, Hx711PinStateLow);
        hx711Handle.delayUsCb(1);
    }
    
    *data ^= 0x800000;
    return *data > 0 ? Hx711ErrOk : Hx711ErrPower;
}

/* When PD_SCK pin changes from low to high
and stays at high for longer than 60�s, HX711
enters power down mode (Fig.3). */
Hx711Err hx711PowerDown(void)
{
    if (isPowerDown)
        return Hx711ErrPower;

    hx711Handle.pinWriteCb(Hx711PinClock, Hx711PinStateHigh);
    hx711Handle.delayUsCb(65);
    isPowerDown = 1;
    return Hx711ErrOk;
}

/* After a reset or power-down event, input
selection is default to Channel A with a gain of 128. */
Hx711Err hx711PowerUp(void)
{
    if (!isPowerDown)
        return Hx711ErrPower;

    hx711Handle.pinWriteCb(Hx711PinClock, Hx711PinStateLow);
    isPowerDown = 0;
    return Hx711ErrOk;
}