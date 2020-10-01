#include "lcd1602a.h"

#define NULL (void *) 0

typedef enum {
    LcdCodeClearDisplay,
    LcdCodeReturnHome,
    LcdCodeEntryModeSet,
    LcdCodeDisplayOn,
    LcdCodeDisplayOff,
    LcdCodeCursorOn,
    LcdCodeCursorOff,
    LcdCodeCursorBlinkOn,
    LcdCodeCursorBlinkOff,
    LcdCodeCursorOrDisplayShift,
    LcdCodeFunctionSet,
    LcdCodeSetCGRAMAddr,
    LcdCodeSetDDRAMAddr,
    LcdCodeReadBusyFlagAndAddr,
    LcdCodeWriteRAMData,
    LcdCodeReadRAMData,
    LcdCodeCount
} LcdCode;

static void lcdSendData(unsigned char data);
static void lcdSendInstruction(unsigned char data);
static void lcdSendInstructionCode(LcdCode code);
static LcdErr lcdCheckBusyFlag();

static unsigned char LcdInstruction[LcdCodeCount] = {
    [LcdCodeClearDisplay] = 1 << LcdPinDB0,
    [LcdCodeReturnHome] = 1 << LcdPinDB1,
    [LcdCodeEntryModeSet]= 1 << LcdPinDB2,
    [LcdCodeDisplayOn]= (1 << LcdPinDB3) | (1 << LcdPinDB2),
    [LcdCodeDisplayOff]= 1 << LcdPinDB3,
    [LcdCodeCursorOn] = (1 << LcdPinDB3) | (1 << LcdPinDB1),
    [LcdCodeCursorOff] = (1 << LcdPinDB3),
    [LcdCodeCursorBlinkOn] = (1 << LcdPinDB3) | (1 << LcdPinDB0),
    [LcdCodeCursorBlinkOff] = (1 << LcdPinDB3),
    [LcdCodeCursorOrDisplayShift] = 1 << LcdPinDB4,
    [LcdCodeFunctionSet] = 1 << LcdPinDB5,
    [LcdCodeSetCGRAMAddr] = 1 << LcdPinDB6,
    [LcdCodeSetDDRAMAddr] = 1 << LcdPinDB7,
    // [LcdCodeReadBusyFlagAndAddr] = 1 << LcdPinReadWrite,
    // [LcdCodeWriteRAMData] = 1 << LcdPinRegisterSelect,
    // [LcdCodeReadRAMData] = (1 << LcdPinReadWrite) | (1 << LcdPinRegisterSelect)
};

LcdHandle lcdHandle;

static void lcdSend(unsigned char data)
{
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateLow);
    lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateHigh);
    for (LcdPin pin = LcdPinDB0; pin <= LcdPinDB7; pin++) {
        lcdHandle.pinWriteCb(pin, data & 1 ? LcdPinStateHigh : LcdPinStateLow);
        data >>= 1;
    }
    lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateLow);
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateHigh);
}

unsigned char lcdRead(void)
{
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateHigh);
    lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateHigh);
    unsigned char data = 0;
    for (LcdPin pin = LcdPinDB0; pin <= LcdPinDB7; pin++) {
        lcdHandle.pinConfigCb(pin, LcdPinDirectionInput);
        LcdPinState pinState = LcdPinStateLow;
        lcdHandle.pinReadCb(pin, &pinState);
        data = (data << 1) | (pinState == LcdPinStateHigh ? 1 : 0);
        lcdHandle.pinConfigCb(pin, LcdPinDirectionOutput);
    }
    lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateLow);
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateLow);
    return data;
}

static void lcdSendInstructionCode(LcdCode code)
{
    // Register Select: L:Instruction Input
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateLow);
    lcdSend(LcdInstruction[code]);
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateHigh);
}

static void lcdSendData(unsigned char data)
{
    // Register Select: H:Data Input
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateHigh);
    lcdSend(data);
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateLow);
}

static void lcdReadData(unsigned char *data, unsigned int len)
{
    for (int i = 0; i < len; i++) {
        // Register Select: H:Data Input
        lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateHigh);
        data[i] = lcdRead();
        lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateLow);
    }
}

static LcdErr lcdCheckBusyFlag()
{
    // lcdHandle.pinCb(LcdPinRegisterSelect, LcdPinStateHigh);
    // lcdHandle.pinCb(LcdPinReadWrite, LcdPinStateLow);
    // lcdHandle.pinCb(LcdPinEnable, LcdPinStateHigh);
    // for (LcdPin pin = LcdPinDB0; pin <= LcdPinDB7; pin++) {
    //     pinCb(pin, data & 1 ? LcdPinStateHigh : LcdPinStateLow);
    //     data >>= 1;
    // }
    // lcdHandle.pinCb(LcdPinEnable, LcdPinStateLow);
    // lcdHandle.pinCb(LcdPinReadWrite, LcdPinStateHigh);
}

LcdErr lcdInit(LcdHandle *handle)
{
    if (handle == NULL || handle->mode >= LcdModeCount || handle->pinWriteCb ||
        handle->pinReadCb == NULL || handle->pinConfigCb == NULL ||
        handle->delayCb == NULL) {
        return LcdErrParam;
    }

    lcdHandle.mode = handle->mode;
    lcdHandle.pinWriteCb = handle->pinWriteCb;
    lcdHandle.pinReadCb = handle->pinReadCb;
    lcdHandle.pinConfigCb = handle->pinConfigCb;
    lcdHandle.delayCb = handle->delayCb;
    return LcdErrOk;
}

LcdErr lcdEnable(void)
{
    lcdSendInstructionCode(LcdCodeDisplayOn);
    return LcdErrOk;
}

LcdErr lcdDisable(void)
{
    lcdSendInstructionCode(LcdCodeDisplayOff);
    return LcdErrOk;
}

LcdErr lcdClearScreen(void)
{
    lcdSendInstructionCode(LcdCodeClearDisplay);
    return LcdErrOk;
}

LcdErr lcdPringChar(unsigned char symbol, unsigned int row, unsigned int position)
{
    lcdSendData(symbol);
    return LcdErrOk;
}

LcdErr lcdPrint(unsigned char text[], unsigned int len)
{
    return LcdErrOk;
}

LcdErr lcdCursorReturnHome(void)
{
    lcdSendInstructionCode(LcdCodeReturnHome);
    return LcdErrOk;
}

LcdErr lcdCursorSetPosition(unsigned int row, unsigned int position)
{
    return LcdErrOk;
}

LcdErr lcdCursorBlinkOn(void)
{
    return LcdErrOk;
}

LcdErr lcdCursorBlinkOff(void)
{
    return LcdErrOk;
}

LcdErr lcdSetMovingDirection(LcdMoveDirection dir)
{
    return LcdErrOk;
}

LcdErr lcdShiftDisplay(LcdMoveDirection dir)
{
    return LcdErrOk;
}
