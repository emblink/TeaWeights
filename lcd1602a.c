#include "lcd1602a.h"

#define NULL (void *) 0

// #ifdef (LCD_DEBUG && LCD_DEBUG > 0)
//     #define LCD_ASSERT(err) assert(!(err != LcdErrOk))
// #else
    #define LCD_ASSERT(err) return err
// #endif // LCD_DEBUG

typedef enum {
    LcdBitPositionDB0 = 0,
    LcdBitPositionDB1,
    LcdBitPositionDB2,
    LcdBitPositionDB3,
    LcdBitPositionDB4,
    LcdBitPositionDB5,
    LcdBitPositionDB6,
    LcdBitPositionDB7,
    LcdBitPositionCount
} LcdBitPosition;

/* Clear display */
#define LCD_CLEAR_DISPLAY (1 << LcdBitPositionDB0)

/* Return home */
#define LCD_RETURN_HOME (1 << LcdBitPositionDB1)

/* Entry mode set defines */
#define LCD_ENTRY_MODE (1 << LcdBitPositionDB2)

#define LCD_ENTRY_MODE_CURSOR_DIRECTION_BIT (LcdBitPositionDB1)
#define LCD_ENTRY_MODE_CURSOR_DIRECTION_BIT_MASK ~(1 << LCD_ENTRY_MODE_CURSOR_DIRECTION_BIT)
#define LCD_ENTRY_MODE_CURSOR_DIRECTION_RIGHT (1 << LCD_ENTRY_MODE_CURSOR_DIRECTION_BIT)
#define LCD_ENTRY_MODE_CURSOR_DIRECTION_LEFT (0)

// Display shift depends on I/D bit, and shift direction is oposite to cursor shift.
// If display shift enabled, I/D bit needs to be reconfigured also.
#define LCD_ENTRY_MODE_DISPLAY_SHIFT_BIT (LcdBitPositionDB0)
#define LCD_ENTRY_MODE_DISPLAY_SHIFT_BIT_MASK ~(1 << LCD_ENTRY_MODE_DISPLAY_SHIFT_BIT)
#define LCD_ENTRY_MODE_DISPLAY_SHIFT_ENABLE (1 << LCD_ENTRY_MODE_DISPLAY_SHIFT_BIT)
#define LCD_ENTRY_MODE_DISPLAY_SHIFT_DISABLE (0)

/* Display on/off defines */
#define LCD_DISPLAY_ON_OFF (1 << LcdBitPositionDB3)

#define LCD_DISPLAY_ON_OFF_BIT (LcdBitPositionDB2)
#define LCD_DISPLAY_ON_OFF_BIT_MASK ~(1 << LCD_DISPLAY_ON_OFF_BIT)
#define LCD_DISPLAY_ON_OFF_ENABLE (1 << LCD_DISPLAY_ON_OFF_BIT)
#define LCD_DISPLAY_ON_OFF_DISABLE (0)

#define LCD_DISPLAY_ON_OFF_CURSOR_BIT (LcdBitPositionDB1)
#define LCD_DISPLAY_ON_OFF_CURSOR_BIT_MASK ~(1 << LCD_DISPLAY_ON_OFF_CURSOR_BIT)
#define LCD_DISPLAY_ON_OFF_CURSOR_ENABLE (1 << LCD_DISPLAY_ON_OFF_CURSOR_BIT)
#define LCD_DISPLAY_ON_OFF_CURSOR_DISABLE (0)

#define LCD_DISPLAY_ON_OFF_BLINK_BIT (LcdBitPositionDB0)
#define LCD_DISPLAY_ON_OFF_BLINK_BIT_MASK ~(1 << LCD_DISPLAY_ON_OFF_BLINK_BIT)
#define LCD_DISPLAY_ON_OFF_BLINK_ENABLE (1 << LCD_DISPLAY_ON_OFF_BLINK_BIT)
#define LCD_DISPLAY_ON_OFF_BLINK_DISABLE (0)

/* Cursor or display shift defines */
#define LCD_CURSOR_OR_DISPLAY_SHIFT (1 << LcdBitPositionDB4)

#define LCD_CURSOR_OR_DISPLAY_SHIFT_DISPLAY_BIT (LcdBitPositionDB3)
#define LCD_CURSOR_OR_DISPLAY_SHIFT_DISPLAY_BIT_MASK ~(1 << LCD_CURSOR_OR_DISPLAY_SHIFT_DISPLAY_BIT)
#define LCD_CURSOR_OR_DISPLAY_SHIFT_DISPLAY_ENABLE (1 << LCD_CURSOR_OR_DISPLAY_SHIFT_DISPLAY_BIT)
#define LCD_CURSOR_OR_DISPLAY_SHIFT_DISPLAY_DISABLE (0)

#define LCD_CURSOR_OR_DISPLAY_SHIFT_DIRECTION_BIT (LcdBitPositionDB2)
#define LCD_CURSOR_OR_DISPLAY_SHIFT_DIRECTION_BIT_MASK ~(1 << LCD_CURSOR_OR_DISPLAY_SHIFT_DIRECTION_BIT)
#define LCD_CURSOR_OR_DISPLAY_SHIFT_DIRECTION_RIGHT (1 << LCD_CURSOR_OR_DISPLAY_SHIFT_DIRECTION_BIT)
#define LCD_CURSOR_OR_DISPLAY_SHIFT_DIRECTION_LEFT (0)

/* Function Set defines */
#define LCD_FUNCTION_SET (1 << LcdBitPositionDB5)

#define LCD_FUNCTION_SET_BUS_MODE_BIT (LcdBitPositionDB4)
#define LCD_FUNCTION_SET_BUS_MODE_BIT_MASK ~(1 << LCD_FUNCTION_SET_BUS_MODE_BIT)
#define LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE (1 << LCD_FUNCTION_SET_BUS_MODE_BIT)
#define LCD_FUNCTION_SET_BUS_MODE_4_BIT_MODE (0)

#define LCD_FUNCTION_SET_LINE_NUMBER_BIT (LcdBitPositionDB3)
#define LCD_FUNCTION_SET_LINE_NUMBER_BIT_MASK ~(1 << LCD_FUNCTION_SET_LINE_NUMBER_BIT)
#define LCD_FUNCTION_SET_LINE_NUMBER_2_LINE_MODE (1 << LCD_FUNCTION_SET_LINE_NUMBER_BIT)
#define LCD_FUNCTION_SET_LINE_NUMBER_1_LINE_MODE (0)

#define LCD_FUNCTION_SET_FONT_TYPE_BIT (LcdBitPositionDB2)
#define LCD_FUNCTION_SET_FONT_TYPE_BIT_MASK ~(1 << LCD_FUNCTION_SET_FONT_TYPE_BIT)
#define LCD_FUNCTION_SET_FONT_TYPE_5x11 (1 << LCD_FUNCTION_SET_FONT_TYPE_BIT)
#define LCD_FUNCTION_SET_FONT_TYPE_5x8 (0)

/* Set CGRAM address */
#define LCD_SET_CGRAM_ADDRESS (1 << LcdBitPositionDB6)

/* Set DDRAM address */
#define LCD_SET_DDRAM_ADDRESS (1 << LcdBitPositionDB7)

static LcdHandle lcdHandle;
static void lcdSend(unsigned char data);
static void lcdSendData(unsigned char data);
static void lcdSendInstruction(unsigned char code);
static unsigned char lcdRead(void);
static unsigned char lcdReadData(void);
static unsigned char lcdReadInstruction(void);

static void lcdSend(unsigned char data)
{
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateLow);
    for (LcdPin pin = LcdPinDB0; pin <= LcdPinDB7; pin++) {
        lcdHandle.pinWriteCb(pin, (data & 1) ? LcdPinStateHigh : LcdPinStateLow);
        data >>= 1;
    }
    lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateHigh);
    lcdHandle.delayUsCb(1);
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

static void lcdSendInstruction(unsigned char code)
{
    // Register Select: L:Instruction Input
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateLow);
    lcdSend(code);
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateHigh);
}

static unsigned char lcdRead(void)
{
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateHigh);
    unsigned char data = 0;
    for (LcdPin pin = LcdPinDB0; pin <= LcdPinDB7; pin++) {
        lcdHandle.pinConfigCb(pin, LcdPinDirectionInput);
        LcdPinState pinState = LcdPinStateLow;
        lcdHandle.pinReadCb(pin, &pinState);
        data = (pinState == LcdPinStateHigh ? 1 : 0) << pin;
        lcdHandle.pinConfigCb(pin, LcdPinDirectionOutput);
    }
    lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateHigh);
    lcdHandle.delayUsCb(1);
    lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateLow);
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateLow);
    return data;
}

static unsigned char lcdReadData(void)
{
    // Register Select: H:Data Input
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateHigh);
    unsigned char data = lcdRead();
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateLow);
    return data;
}

static unsigned char lcdReadInstruction(void)
{
    // Register Select: H:Data Input
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateLow);
    unsigned char data = lcdRead();
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateHigh);
    return data;
}

LcdErr lcdInit(LcdHandle *handle)
{
    if (handle == NULL || handle->mode >= LcdInterfaceCount || handle->pinWriteCb == NULL ||
        handle->pinReadCb == NULL || handle->pinConfigCb == NULL ||
        handle->delayUsCb == NULL) {
        LCD_ASSERT(LcdErrParam);
    }

    lcdHandle.mode = handle->mode;
    lcdHandle.pinWriteCb = handle->pinWriteCb;
    lcdHandle.pinReadCb = handle->pinReadCb;
    lcdHandle.pinConfigCb = handle->pinConfigCb;
    lcdHandle.delayUsCb = handle->delayUsCb;

    for (LcdPin pin = LcdPinDB0; pin < LcdPinCount; pin++) {
        lcdHandle.pinConfigCb(pin, LcdPinDirectionOutput);
    }

    lcdSendInstruction(LCD_FUNCTION_SET | LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE);
    lcdHandle.delayUsCb(4500);
    lcdSendInstruction(LCD_FUNCTION_SET | LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE);
    lcdHandle.delayUsCb(150);
    lcdSendInstruction(LCD_FUNCTION_SET | LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdSendInstruction(LCD_FUNCTION_SET | LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE | LCD_FUNCTION_SET_LINE_NUMBER_2_LINE_MODE | LCD_FUNCTION_SET_FONT_TYPE_5x8);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdSendInstruction(LCD_DISPLAY_ON_OFF | LCD_DISPLAY_ON_OFF_ENABLE | LCD_DISPLAY_ON_OFF_CURSOR_ENABLE | LCD_DISPLAY_ON_OFF_BLINK_ENABLE);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdSendInstruction(LCD_CLEAR_DISPLAY);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    // lcdSendInstruction(LCD_ENTRY_MODE | LCD_ENTRY_MODE_DISPLAY_SHIFT_DISABLE | LCD_ENTRY_MODE_CURSOR_DIRECTION_RIGHT);
    lcdSendInstruction(LCD_ENTRY_MODE | LCD_ENTRY_MODE_DISPLAY_SHIFT_DISABLE | LCD_ENTRY_MODE_CURSOR_DIRECTION_LEFT);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }

    return LcdErrOk;
}

LcdErr lcdClearScreen(void)
{
    lcdSendInstruction(LCD_CLEAR_DISPLAY);
    return LcdErrOk;
}

LcdErr lcdCursorReturnHome(void)
{
    lcdSendInstruction(LCD_RETURN_HOME);
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
    lcdSendInstruction(LCD_DISPLAY_ON_OFF | LCD_DISPLAY_ON_OFF_ENABLE);
    return LcdErrOk;
}

LcdErr lcdTurnOff(void)
{
    lcdSendInstruction(LCD_DISPLAY_ON_OFF | LCD_DISPLAY_ON_OFF_DISABLE);
    return LcdErrOk;
}

LcdErr lcdCursorOn(void)
{
    lcdSendInstruction(LCD_DISPLAY_ON_OFF | LCD_DISPLAY_ON_OFF_ENABLE | LCD_DISPLAY_ON_OFF_CURSOR_ENABLE);
    return LcdErrOk;
}

LcdErr lcdCursorOff(void)
{
    lcdSendInstruction(LCD_DISPLAY_ON_OFF | LCD_DISPLAY_ON_OFF_ENABLE | LCD_DISPLAY_ON_OFF_CURSOR_DISABLE);
    return LcdErrOk;
}

LcdErr lcdCursorBlinkOn(void)
{
    lcdSendInstruction(LCD_DISPLAY_ON_OFF | LCD_DISPLAY_ON_OFF_ENABLE | LCD_DISPLAY_ON_OFF_BLINK_ENABLE);
    return LcdErrOk;
}

LcdErr lcdCursorBlinkOff(void)
{
    lcdSendInstruction(LCD_DISPLAY_ON_OFF | LCD_DISPLAY_ON_OFF_BLINK_DISABLE);
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
    // lcdSendInstruction(linesNumber == 2 ?  : );
    return LcdErrOk;
}

LcdErr lcdFontTypeSet(LcdFontType font)
{

    return LcdErrOk;
}

LcdErr lcdCGRAMAddrSet(unsigned char addr)
{
    lcdSendInstruction(LCD_SET_CGRAM_ADDRESS | (addr & 0x3F));
    return LcdErrOk;
}

LcdErr lcdDDRAMAddrSet(unsigned char addr)
{
    lcdSendInstruction(LCD_SET_DDRAM_ADDRESS | (addr & 0x7F));
    return LcdErrOk;
}

LcdErr lcdCheckBusyFlag(void)
{
    unsigned char data = lcdReadInstruction();
    if (data & (1 << LcdPinDB7) == 1) {
        return LcdErrBusy;
    }
    return LcdErrOk;
}

LcdErr lcdPringChar(unsigned char symbol)
{
    lcdSendData(symbol);
    return LcdErrOk;
}

LcdErr lcdPrint(unsigned char text[], unsigned char len)
{
    for (unsigned char i = 0; i < len; i++) {
        lcdCheckBusyFlag();
        lcdPringChar(text[i]);
    }
    return LcdErrOk;
}

LcdErr lcdCursorPositionSet(unsigned char line, unsigned char position)
{
    if (position >= LCD_MAX_SYMBOLS_IN_ROW || line >= LCD_MAX_ROWS)
        return LcdErrParam;
    // one line mode : 00H - 0x4F
    // two line mode : first line - 0x00 - 0x27, second line 0x40 - 0x67
    unsigned char addr = position + (line == 1 ? 0x40 : 0x00);
    lcdDDRAMAddrSet(addr);
    return LcdErrOk;
}

