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

typedef struct {
    unsigned char LcdEntryMode;
    unsigned char LcdDisplayOnOffMode;
    unsigned char LcdCursorOrDisplayShiftMode;
    unsigned char LcdFunctionMode;
} LcdConfig;

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

/* Busy flag bit */
#define LCD_BUSY_FLAG_BIT (1 << LcdBitPositionDB7)

/* Address counter mask */
#define LCD_ADDRESS_COUNTER_MASK ((1 << LcdBitPositionDB6) | (1 << LcdBitPositionDB5) |\
        (1 << LcdBitPositionDB4) | (1 << LcdBitPositionDB3) | (1 << LcdBitPositionDB2) |\
        (1 << LcdBitPositionDB1) | (1 << LcdBitPositionDB0))

static LcdHandle lcdHandle;
static LcdConfig lcdConfig = {0};
static void enablePulse(void);
static void send4Bits(unsigned char data);
static void lcdSend(unsigned char data);
static void lcdSendData(unsigned char data);
static void lcdSendInstruction(unsigned char code);
unsigned char read4Bits(void);
static unsigned char lcdRead(void);
static unsigned char lcdReadData(void);
static unsigned char lcdReadInstruction(void);

static void enablePulse(void)
{
    lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateHigh);
    lcdHandle.delayUsCb(1);
    lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateLow);
}

static void send4Bits(unsigned char data)
{
    for (LcdPin pin = LcdPinDB4; pin <= LcdPinDB7; pin++) {
        lcdHandle.pinWriteCb(pin, (data & 1) ? LcdPinStateHigh : LcdPinStateLow);
        data >>= 1;
    }
    enablePulse();
}

static void lcdSend(unsigned char data)
{
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateLow);
    if (lcdConfig.LcdFunctionMode & LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE) {
        for (LcdPin pin = LcdPinDB0; pin <= LcdPinDB7; pin++) {
            lcdHandle.pinWriteCb(pin, (data & 1) ? LcdPinStateHigh : LcdPinStateLow);
            data >>= 1;
        }
        enablePulse();
    } else {
        send4Bits((data & 0xF0) >> 4);
        send4Bits(data & 0x0F);
    }
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

unsigned char read4Bits(void)
{
    for (LcdPin pin = LcdPinDB4; pin <= LcdPinDB7; pin++) {
        lcdHandle.pinConfigCb(pin, LcdPinDirectionInput);
    }

    lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateHigh);
    lcdHandle.delayUsCb(20);

    unsigned char data = 0;
    unsigned char shift = 0;
    for (LcdPin pin = LcdPinDB4; pin <= LcdPinDB7; pin++, shift++) {
        LcdPinState pinState = LcdPinStateLow;
        lcdHandle.pinReadCb(pin, &pinState);
        data |= (pinState == LcdPinStateHigh ? 1 : 0) << shift;
        lcdHandle.pinConfigCb(pin, LcdPinDirectionOutput);
    }

    lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateLow);
    return data;
}

static unsigned char lcdRead(void)
{
    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateHigh);
    
    unsigned char data = 0;
    if (lcdConfig.LcdFunctionMode & LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE) {
        for (LcdPin pin = LcdPinDB0; pin <= LcdPinDB7; pin++) {
            lcdHandle.pinConfigCb(pin, LcdPinDirectionInput);
        }
        lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateHigh);
        lcdHandle.delayUsCb(20);

        unsigned char shift = 0;
        for (LcdPin pin = LcdPinDB0; pin <= LcdPinDB7; pin++, shift++) {
            LcdPinState pinState = LcdPinStateLow;
            lcdHandle.pinReadCb(pin, &pinState);
            data |= (pinState == LcdPinStateHigh ? 1 : 0) << shift;
            lcdHandle.pinConfigCb(pin, LcdPinDirectionOutput);
        }

        lcdHandle.pinWriteCb(LcdPinEnable, LcdPinStateLow);
    } else {
        unsigned char highBist = read4Bits();
        unsigned char lowBist = read4Bits();
        data = (highBist << 4) | lowBist;
    }

    lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateLow);
    return data;
}

static unsigned char lcdReadData(void)
{
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateHigh);
    unsigned char data = lcdRead();
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateLow);
    return data;
}

static unsigned char lcdReadInstruction(void)
{
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateLow);
    unsigned char data = lcdRead();
    lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateHigh);
    return data;
}

LcdErr lcdInit(LcdHandle *handle, LcdInterface mode, LcdFontType font, LcdLineMode lineMode)
{
    if (handle == NULL || mode >= LcdInterfaceCount  || font >= LcdFontTypeCount ||
        lineMode >= LcdLineModeCount || handle->pinWriteCb == NULL || handle->pinReadCb == NULL ||
        handle->pinConfigCb == NULL || handle->delayUsCb == NULL) {
        LCD_ASSERT(LcdErrParam);
    }

    lcdConfig.LcdEntryMode = LCD_ENTRY_MODE;
    lcdConfig.LcdDisplayOnOffMode = LCD_DISPLAY_ON_OFF;
    lcdConfig.LcdCursorOrDisplayShiftMode = LCD_CURSOR_OR_DISPLAY_SHIFT;
    lcdConfig.LcdFunctionMode = LCD_FUNCTION_SET;

    if (mode == LcdInterface4Bit) {
        lcdConfig.LcdFunctionMode |= LCD_FUNCTION_SET_BUS_MODE_4_BIT_MODE;
    } else {
        lcdConfig.LcdFunctionMode |= LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE;
    }

    if (font == LcdFontType5x8) {
        lcdConfig.LcdFunctionMode |= LCD_FUNCTION_SET_FONT_TYPE_5x8;
    } else {
        lcdConfig.LcdFunctionMode |= LCD_FUNCTION_SET_FONT_TYPE_5x11;
    }

    if (lineMode == LcdOneLineMode) {
        lcdConfig.LcdFunctionMode |= LCD_FUNCTION_SET_LINE_NUMBER_1_LINE_MODE;
    } else {
        lcdConfig.LcdFunctionMode |= LCD_FUNCTION_SET_LINE_NUMBER_2_LINE_MODE;
    }

    lcdHandle.pinWriteCb = handle->pinWriteCb;
    lcdHandle.pinReadCb = handle->pinReadCb;
    lcdHandle.pinConfigCb = handle->pinConfigCb;
    lcdHandle.delayUsCb = handle->delayUsCb;

    for (LcdPin pin = mode == LcdInterface8Bit ? LcdPinDB0 : LcdPinDB4; pin < LcdPinCount; pin++) {
        lcdHandle.pinConfigCb(pin, LcdPinDirectionOutput);
    }

    if (mode == LcdInterface8Bit) {
        lcdSendInstruction(LCD_FUNCTION_SET | LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE);
        lcdHandle.delayUsCb(4500);
        lcdSendInstruction(LCD_FUNCTION_SET | LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE);
        lcdHandle.delayUsCb(150);
        lcdSendInstruction(LCD_FUNCTION_SET | LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE);
    } else {
        lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateLow);
        lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateLow);
        send4Bits((LCD_FUNCTION_SET | LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE) >> 4);
        lcdHandle.delayUsCb(4500);
        send4Bits((LCD_FUNCTION_SET | LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE) >> 4);
        lcdHandle.delayUsCb(150);
        send4Bits((LCD_FUNCTION_SET | LCD_FUNCTION_SET_BUS_MODE_8_BIT_MODE) >> 4);
        send4Bits((LCD_FUNCTION_SET | LCD_FUNCTION_SET_BUS_MODE_4_BIT_MODE) >> 4);
        lcdHandle.pinWriteCb(LcdPinRegisterSelect, LcdPinStateHigh);
        lcdHandle.pinWriteCb(LcdPinReadWrite, LcdPinStateHigh);
    }
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdSendInstruction(lcdConfig.LcdFunctionMode);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdTurnOff();
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdClearScreen();
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdShiftDirectionSet(LcdDirectionRight);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdDisplayShiftDisable();
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    return LcdErrOk;
}

LcdErr lcdClearScreen(void)
{
    lcdConfig.LcdEntryMode |= LCD_ENTRY_MODE_CURSOR_DIRECTION_RIGHT;
    lcdSendInstruction(LCD_CLEAR_DISPLAY);
    return LcdErrOk;
}

LcdErr lcdCursorReturnHome(void)
{
    lcdSendInstruction(LCD_RETURN_HOME);
    return LcdErrOk;
}

LcdErr lcdShiftDirectionSet(LcdDirection dir)
{
    lcdConfig.LcdEntryMode &= LCD_ENTRY_MODE_CURSOR_DIRECTION_BIT_MASK;
    if (lcdConfig.LcdEntryMode & LCD_ENTRY_MODE_DISPLAY_SHIFT_BIT == LCD_ENTRY_MODE_DISPLAY_SHIFT_ENABLE) {
        lcdConfig.LcdEntryMode |= dir == LcdDirectionRight ?
        LCD_ENTRY_MODE_CURSOR_DIRECTION_LEFT : LCD_ENTRY_MODE_CURSOR_DIRECTION_RIGHT;
    } else {
        lcdConfig.LcdEntryMode |= dir == LcdDirectionRight ?
        LCD_ENTRY_MODE_CURSOR_DIRECTION_RIGHT : LCD_ENTRY_MODE_CURSOR_DIRECTION_LEFT;
    }
    lcdSendInstruction(lcdConfig.LcdEntryMode);
    return LcdErrOk;
}

LcdErr lcdDisplayShiftEnable(void)
{
    lcdConfig.LcdEntryMode &= LCD_ENTRY_MODE_DISPLAY_SHIFT_BIT_MASK;
    lcdConfig.LcdEntryMode |= LCD_ENTRY_MODE_DISPLAY_SHIFT_ENABLE;
    lcdSendInstruction(lcdConfig.LcdEntryMode);
    return LcdErrOk;
}

LcdErr lcdDisplayShiftDisable(void)
{
    lcdConfig.LcdEntryMode &= LCD_ENTRY_MODE_DISPLAY_SHIFT_BIT_MASK;
    lcdConfig.LcdEntryMode |= LCD_ENTRY_MODE_DISPLAY_SHIFT_DISABLE;
    lcdSendInstruction(lcdConfig.LcdEntryMode);
    return LcdErrOk;
}

LcdErr lcdTurnOn(void)
{
    lcdConfig.LcdDisplayOnOffMode &= LCD_DISPLAY_ON_OFF_BIT_MASK;
    lcdConfig.LcdDisplayOnOffMode |= LCD_DISPLAY_ON_OFF_ENABLE;
    lcdSendInstruction(lcdConfig.LcdDisplayOnOffMode);
    return LcdErrOk;
}

LcdErr lcdTurnOff(void)
{
    lcdConfig.LcdDisplayOnOffMode &= LCD_DISPLAY_ON_OFF_BIT_MASK;
    lcdConfig.LcdDisplayOnOffMode |= LCD_DISPLAY_ON_OFF_DISABLE;
    lcdSendInstruction(lcdConfig.LcdDisplayOnOffMode);
    return LcdErrOk;
}

LcdErr lcdCursorOn(void)
{
    lcdConfig.LcdDisplayOnOffMode &= LCD_DISPLAY_ON_OFF_CURSOR_BIT_MASK;
    lcdConfig.LcdDisplayOnOffMode |= LCD_DISPLAY_ON_OFF_CURSOR_ENABLE;
    lcdSendInstruction(lcdConfig.LcdDisplayOnOffMode);
    return LcdErrOk;
}

LcdErr lcdCursorOff(void)
{
    lcdConfig.LcdDisplayOnOffMode &= LCD_DISPLAY_ON_OFF_CURSOR_BIT_MASK;
    lcdConfig.LcdDisplayOnOffMode |= LCD_DISPLAY_ON_OFF_CURSOR_DISABLE;
    lcdSendInstruction(lcdConfig.LcdDisplayOnOffMode);
    return LcdErrOk;
}

LcdErr lcdCursorBlinkOn(void)
{
    lcdConfig.LcdDisplayOnOffMode &= LCD_DISPLAY_ON_OFF_BLINK_BIT_MASK;
    lcdConfig.LcdDisplayOnOffMode |= LCD_DISPLAY_ON_OFF_BLINK_ENABLE;
    lcdSendInstruction(lcdConfig.LcdDisplayOnOffMode);
    return LcdErrOk;
}

LcdErr lcdCursorBlinkOff(void)
{
    lcdConfig.LcdDisplayOnOffMode &= LCD_DISPLAY_ON_OFF_BLINK_BIT_MASK;
    lcdConfig.LcdDisplayOnOffMode |= LCD_DISPLAY_ON_OFF_BLINK_DISABLE;
    lcdSendInstruction(lcdConfig.LcdDisplayOnOffMode);
    return LcdErrOk;
}

/* Cursor or display shift shifts the cursor position or display to the right or left without writing or reading
display data (Table 7). This function is used to correct or search the display. */
LcdErr lcdCursorShift(LcdDirection dir)
{
    lcdConfig.LcdCursorOrDisplayShiftMode &= LCD_CURSOR_OR_DISPLAY_SHIFT_DISPLAY_BIT_MASK;
    lcdConfig.LcdCursorOrDisplayShiftMode |= LCD_CURSOR_OR_DISPLAY_SHIFT_DISPLAY_DISABLE;
    lcdConfig.LcdCursorOrDisplayShiftMode &= LCD_CURSOR_OR_DISPLAY_SHIFT_DIRECTION_BIT_MASK;
    lcdConfig.LcdCursorOrDisplayShiftMode |= dir == LcdDirectionRight ?
    LCD_CURSOR_OR_DISPLAY_SHIFT_DIRECTION_RIGHT : LCD_CURSOR_OR_DISPLAY_SHIFT_DIRECTION_LEFT;
    lcdSendInstruction(lcdConfig.LcdCursorOrDisplayShiftMode);
    return LcdErrOk;
}

LcdErr lcdDisplayShift(LcdDirection dir)
{
    lcdConfig.LcdCursorOrDisplayShiftMode &= LCD_CURSOR_OR_DISPLAY_SHIFT_DISPLAY_BIT_MASK;
    lcdConfig.LcdCursorOrDisplayShiftMode |= LCD_CURSOR_OR_DISPLAY_SHIFT_DISPLAY_ENABLE;
    lcdConfig.LcdCursorOrDisplayShiftMode &= LCD_CURSOR_OR_DISPLAY_SHIFT_DIRECTION_BIT_MASK;
    lcdConfig.LcdCursorOrDisplayShiftMode |= dir == LcdDirectionRight ?
    LCD_CURSOR_OR_DISPLAY_SHIFT_DIRECTION_RIGHT : LCD_CURSOR_OR_DISPLAY_SHIFT_DIRECTION_LEFT;
    lcdSendInstruction(lcdConfig.LcdCursorOrDisplayShiftMode);
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
    if ((data & LCD_BUSY_FLAG_BIT) == 0) {
        return LcdErrOk;
    }
    return LcdErrBusy;
}

LcdErr lcdReadAddressCounter(unsigned char *addressCounter)
{
    unsigned char data = lcdReadInstruction();
    if (data & LCD_BUSY_FLAG_BIT) {
        return LcdErrBusy;
    }
    *addressCounter = data & LCD_ADDRESS_COUNTER_MASK;
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
    // TODO: check line mode
    unsigned char offset = line == 1 ? 0x40 : 0x00;
    lcdDDRAMAddrSet(position + offset);
    return LcdErrOk;
}

