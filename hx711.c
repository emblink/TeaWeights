#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "hx711.h"

Hx711Handle *handle = NULL;
static bool inited = false;
static bool isPowerDown = false;

Hx711Status hx711Init(Hx711Handle *hndl)
{
    inited = false;
    if (hndl != NULL) {
        if (hndl->readCb && hndl->writeCb && hndl->delayCb) {
            handle = hndl;
            inited = true;
        }
    }
    return inited == true ? Hx711StatusOk : Hx711StatusInitErr;
}

Hx711Status hx711GetStatus(void)
{
    if (!inited)
        return Hx711StatusInitErr;
    else if (isPowerDown)
        return Hx711StatusPowerErr;

    return handle->readCb(handle->dataPin) == false ? Hx711StatusReady : Hx711StatusBusy;
}

Hx711Status hx711ReadChannel(Hx711Channel channel, int32_t *data)
{
    if (!inited)
        return Hx711StatusInitErr;
    else if (channel < Hx711ChannelA128 || channel > Hx711ChannelA64 || data == NULL)
        return Hx711StatusParamErr;
    else if (isPowerDown)
        return Hx711StatusPowerErr;

    *data = 0;
    for (uint32_t i = 0; i < 24; i++) {
        *data <<= 1;
    	handle->writeCb(handle->sclkPin, true);
        handle->delayCb(1);
        if (handle->readCb(handle->dataPin)) {
            (*data)++;
        }
        handle->writeCb(handle->sclkPin, false);
        handle->delayCb(1);
    }

    for (uint32_t i = 0; i < channel - 24; i++) {
        handle->writeCb(handle->sclkPin, true);
        handle->delayCb(1);
        handle->writeCb(handle->sclkPin, false);
        handle->delayCb(1);
    }
    
    *data ^= 0x800000;
    return *data > 0 ? Hx711StatusOk : Hx711StatusPowerErr;
}

/* When PD_SCK pin changes from low to high
and stays at high for longer than 60µs, HX711
enters power down mode (Fig.3). */
Hx711Status hx711PowerDown(void)
{
    if (!inited) 
        return Hx711StatusInitErr;
    else if (isPowerDown)
        return Hx711StatusPowerErr;

    handle->writeCb(handle->sclkPin, true);
    handle->delayCb(65);
    isPowerDown = true;
    return Hx711StatusOk;
}

/* After a reset or power-down event, input
selection is default to Channel A with a gain of 128. */
Hx711Status hx711PowerUp(void)
{
    if (!inited) 
        return Hx711StatusInitErr;
    else if (!isPowerDown)
        return Hx711StatusPowerErr;

    handle->writeCb(handle->sclkPin, false);
    isPowerDown = false;
    return Hx711StatusOk;
}