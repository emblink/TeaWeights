#include "hx711.h"
#define NULL ((void *) 0)

Hx711Handle hx711Handle = {0};
unsigned char isPowerDown = 0;

Hx711Err hx711Init(Hx711Handle *handle)
{
    if (handle == NULL || handle->pinReadCb == NULL  || handle->pinWriteCb == NULL
        || handle->delayUsCb == NULL || handle->initChannel >= Hx711ChannelCount)
        return Hx711ErrInit;
    
    hx711Handle.pinReadCb = handle->pinReadCb;
    hx711Handle.pinWriteCb = handle->pinWriteCb;
    hx711Handle.delayUsCb = handle->delayUsCb;
    hx711Handle.initChannel = handle->initChannel;
    // init channel for next measurements
    hx711Handle.pinWriteCb(Hx711PinClock, Hx711PinStateLow);
    signed long pinState;
    while (hx711ReadChannel(hx711Handle.initChannel, &pinState) != Hx711ErrOk) {}
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
    // hx711Handle.delayUsCb(1);
    Hx711PinState pinState;
    hx711Handle.pinReadCb(Hx711PinData, &pinState);
    unsigned char bit = pinState == Hx711PinStateHigh ? 1U : 0;
    hx711Handle.pinWriteCb(Hx711PinClock, Hx711PinStateLow);
    // hx711Handle.delayUsCb(1);
    return bit;
}

static unsigned char hx711ReadByte(void)
{
    unsigned char byte = 0;
    /* According to datasheet order of reading is MSB to LSB */
    for (int bit = 0; bit < 8; bit++) {
        byte |= hx711ReadBit() << (7 - bit);
    }
    return byte;
}

Hx711Err hx711ReadChannel(Hx711Channel nextChannel, signed long *value)
{
    if (nextChannel < Hx711ChannelA128 || nextChannel > Hx711ChannelA64 || value == NULL)
        return Hx711ErrParam;
    
    if (isPowerDown)
        return Hx711ErrPower;

    Hx711Err err = hx711GetStatus();
    if (err != Hx711ErrOk)
        return err;

    // Pulse the clock pin 24 times to read the data.
    ((unsigned char *) value)[1] = hx711ReadByte();
    ((unsigned char *) value)[2] = hx711ReadByte();
    ((unsigned char *) value)[3] = hx711ReadByte();

    for (unsigned char i = 0; i < nextChannel; i++) {
        hx711Handle.pinWriteCb(Hx711PinClock, Hx711PinStateHigh);
        // hx711Handle.delayUsCb(1);
        hx711Handle.pinWriteCb(Hx711PinClock, Hx711PinStateLow);
        // hx711Handle.delayUsCb(1);
    }

    /* if value is negative, fill first byte with 0xFF in order to
    indicate int32_t two's complement negative number */
    if (((unsigned char *) value)[1] & 0x80) {
        ((unsigned char *) value)[0] = 0xFF;
    } else {
        ((unsigned char *) value)[0] = 0x00;
    }

    return Hx711ErrOk;
}

/* When PD_SCK pin changes from low to high
and stays at high for longer than 60us, HX711
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