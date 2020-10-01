#include "lcd1602a.h"

#define NULL (void *) 0

// #ifdef (LCD_DEBUG && LCD_DEBUG > 0)
//     #define LCD_ASSERT(err) assert(!(err != LcdErrOk))
// #else
    #define LCD_ASSERT(err) return err
// #endif // LCD_DEBUG

typedef enum {
    LcdBitDB0 = 0,
    LcdBitDB1,
    LcdBitDB2,
    LcdBitDB3,
    LcdBitDB4,
    LcdBitDB5,
    LcdBitDB6,
    LcdBitDB7,
    LcdBitCount
} LcdBit;

typedef enum {
    LcdInstructionClearDisplay,
    LcdInstructionReturnHome,
    LcdInstructionDisplayShiftRightOnReadWrite,
    LcdInstructionDisplayShiftLeftOnReadWrite,
    LcdInstructionDisplayShiftOnReadWriteDisable,
    LcdInstructionDisplayOn,
    LcdInstructionDisplayOff,
    LcdInstructionCursorOn,
    LcdInstructionCursorOff,
    LcdInstructionCursorBlinkOn,
    LcdInstructionCursorBlinkOff,
    LcdInstructionCursorShiftLeft,
    LcdInstructionCursorShiftRight,
    LcdInstructionDisplayShiftLeft,
    LcdInstructionDisplayShiftRight,
    LcdInstructionInterfaceSet4Bit,
    LcdInstructionInterfaceSet8Bit,
    LcdInstructionOneLineModeSet,
    LcdInstructionTwoLineModeSet,
    LcdInstructionOneLineModeFontType5x8,
    LcdInstructionOneLineModeFontType5x11,
    LcdInstructionTwoLineModeFontType5x8,
    LcdInstructionSetCGRAMAddr,
    LcdInstructionSetDDRAMAddr,
    LcdInstructionCount
} LcdInstruction;

static unsigned char instructions[LcdInstructionCount] = {
    [LcdInstructionClearDisplay] = (1 << LcdBitDB0),
    [LcdInstructionReturnHome] = (1 << LcdBitDB1),
    [LcdInstructionDisplayShiftRightOnReadWrite] = (1 << LcdBitDB2) | (1 << LcdBitDB1) | (1 << LcdBitDB0),
    [LcdInstructionDisplayShiftLeftOnReadWrite] = (1 << LcdBitDB2) | (1 << LcdBitDB0),
    [LcdInstructionDisplayShiftOnReadWriteDisable] = (1 << LcdBitDB2),
    [LcdInstructionDisplayOn] = (1 << LcdBitDB3) | (1 << LcdPinDB2),
    [LcdInstructionDisplayOff] = (1 << LcdBitDB3),
    [LcdInstructionCursorOn] = (1 << LcdBitDB3) | (1 << LcdBitDB1),
    [LcdInstructionCursorOff] = (1 << LcdBitDB3),
    [LcdInstructionCursorBlinkOn] = (1 << LcdBitDB3) | (1 << LcdBitDB0),
    [LcdInstructionCursorBlinkOff] = (1 << LcdBitDB3),
    [LcdInstructionCursorShiftLeft] = (1 << LcdBitDB4),
    [LcdInstructionCursorShiftRight] = (1 << LcdBitDB4) | (1 << LcdBitDB2),
    [LcdInstructionDisplayShiftLeft] = (1 << LcdBitDB4) | (1 << LcdBitDB3),
    [LcdInstructionDisplayShiftRight] = (1 << LcdBitDB4) | (1 << LcdBitDB4) | (1 << LcdBitDB2),
    [LcdInstructionInterfaceSet4Bit] = (1 << LcdBitDB5),
    [LcdInstructionInterfaceSet8Bit] = (1 << LcdBitDB5) | (1 << LcdBitDB4),
    [LcdInstructionOneLineModeSet] = (1 << LcdBitDB5),
    [LcdInstructionTwoLineModeSet] = (1 << LcdBitDB5) | (1 << LcdBitDB3),
    [LcdInstructionOneLineModeFontType5x8] = (1 << LcdBitDB5),
    [LcdInstructionOneLineModeFontType5x11] = (1 << LcdBitDB5) | (1 << LcdBitDB2),
    [LcdInstructionTwoLineModeFontType5x8] = (1 << LcdBitDB5) | (1 << LcdBitDB3),
    [LcdInstructionSetCGRAMAddr] = (1 << LcdBitDB6),
    [LcdInstructionSetDDRAMAddr] = (1 << LcdBitDB7),
};

static LcdHandle lcdHandle;
static void lcdSend(unsigned char data);
static void lcdSendData(unsigned char data);
static void lcdSendInstruction(LcdInstruction code);
static unsigned char lcdRead(void);
static void lcdReadData(unsigned char *data, unsigned int len);

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

static void lcdSendData(unsigned char data)
{
    // Register Select: H:Data Input
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateHigh);
    lcdSend(data);
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateLow);
}

static void lcdSendInstruction(LcdInstruction code)
{
    // Register Select: L:Instruction Input
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateLow);
    lcdSend(instructions[code]);
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateHigh);
}

static unsigned char lcdRead(void)
{
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateHigh);
    lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateHigh);
    unsigned char data = 0;
    for (LcdPin pin = LcdPinDB0; pin <= LcdPinDB7; pin++) {
        lcdHandle.pinConfigCb(pin, LcdPinDirectionInput);
        LcdPinState pinState = LcdPinStateLow;
        lcdHandle.pinReadCb(pin, &pinState);
        data = (pinState == LcdPinStateHigh ? 1 : 0) << pin;
        lcdHandle.pinConfigCb(pin, LcdPinDirectionOutput);
    }
    lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateLow);
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateLow);
    return data;
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

LcdErr lcdInit(LcdHandle *handle)
{
    if (handle == NULL || handle->mode >= LcdInterfaceCount || handle->pinWriteCb == NULL ||
        handle->pinReadCb == NULL || handle->pinConfigCb == NULL ||
        handle->delayCb == NULL) {
        LCD_ASSERT(LcdErrParam);
    }

    lcdHandle.mode = handle->mode;
    lcdHandle.pinWriteCb = handle->pinWriteCb;
    lcdHandle.pinReadCb = handle->pinReadCb;
    lcdHandle.pinConfigCb = handle->pinConfigCb;
    lcdHandle.delayCb = handle->delayCb;

    for (LcdPin pin = LcdPinDB0; pin < LcdPinCount; pin++) {
        lcdHandle.pinConfigCb(pin, LcdPinDirectionOutput);
    }

    lcdSendInstruction(LcdInstructionInterfaceSet8Bit);
    lcdHandle.delayCb(4100);
    lcdSendInstruction(LcdInstructionInterfaceSet8Bit);
    lcdHandle.delayCb(100);
    lcdSendInstruction(LcdInstructionInterfaceSet8Bit);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdSendInstruction(LcdInstructionTwoLineModeSet);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdSendInstruction(LcdInstructionDisplayOn);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdSendInstruction(LcdInstructionClearDisplay);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdSendInstruction(LcdInstructionClearDisplay);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdSendInstruction(LcdInstructionDisplayShiftRightOnReadWrite);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }

    return LcdErrOk;
}

LcdErr lcdClearScreen(void)
{
    lcdSendInstruction(LcdInstructionClearDisplay);
    return LcdErrOk;
}

LcdErr lcdCursorReturnHome(void)
{
    lcdSendInstruction(LcdInstructionReturnHome);
    return LcdErrOk;
}

LcdErr lcdCursorShiftSet(LcdMoveDirection dir)
{

    return LcdErrOk;
}

LcdErr lcdShiftOnReadWriteEnable(LcdMoveDirection dir)
{

    return LcdErrOk;
}

LcdErr lcdShiftOnWriteDisable(void)
{

    return LcdErrOk;
}

LcdErr lcdTurnOn(void)
{
    lcdSendInstruction(LcdInstructionDisplayOn);
    return LcdErrOk;
}

LcdErr lcdTurnOff(void)
{
    lcdSendInstruction(LcdInstructionDisplayOff);
    return LcdErrOk;
}

LcdErr lcdCursorOn(void)
{
    lcdSendInstruction(LcdInstructionCursorOn);
    return LcdErrOk;
}

LcdErr lcdCursorOff(void)
{

    return LcdErrOk;
}

LcdErr lcdCursorBlinkOn(void)
{
    lcdSendInstruction(LcdInstructionCursorOn);
    return LcdErrOk;
}

LcdErr lcdCursorBlinkOff(void)
{

    return LcdErrOk;
}

LcdErr lcdCursorShift(LcdMoveDirection dir)
{

    return LcdErrOk;
}

LcdErr lcdShift(LcdMoveDirection dir)
{

    return LcdErrOk;
}

LcdErr lcdInterfaceSet(LcdInterface mode)
{

    return LcdErrOk;
}

LcdErr lcdLineNumberSet(unsigned char linesNumber)
{
    lcdSendInstruction(linesNumber == 2 ? LcdInstructionTwoLineModeSet : LcdInstructionOneLineModeSet);
    return LcdErrOk;
}

LcdErr lcdFontTypeSet(LcdFontType font)
{

    return LcdErrOk;
}

LcdErr lcdCGRAMAddrSet(unsigned char addr)
{

    return LcdErrOk;
}

LcdErr lcdDDRAMAddrSet(unsigned char addr)
{

    return LcdErrOk;
}

LcdErr lcdCheckBusyFlag(void)
{
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateLow);
    unsigned char data = lcdRead();
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateHigh);
    if (data & (1 << LcdPinDB7) == 1) {
        return LcdErrBusy;
    }
    return LcdErrOk;
}

LcdErr lcdPringChar(unsigned char symbol, unsigned int row, unsigned int position)
{

    return LcdErrOk;
}

LcdErr lcdPrint(unsigned char text[], unsigned int len)
{

    return LcdErrOk;
}

LcdErr lcdCursorPositionSet(unsigned int row, unsigned int position)
{

    return LcdErrOk;
}

