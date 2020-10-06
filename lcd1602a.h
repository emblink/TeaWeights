#define LCD_MAX_ROWS (2U)
#define LCD_MAX_SYMBOLS_IN_ROW (16U)

typedef enum {
    LcdErrOk,
    LcdErrBusy,
    LcdErrInit,
    LcdErrMode,
    LcdErrParam,
} LcdErr;

typedef enum {
    LcdPinStateLow,
    LcdPinStateHigh,
    LcdPinStateCount
} LcdPinState;

typedef enum {
    LcdPinDB0 = 0,
    LcdPinDB1,
    LcdPinDB2,
    LcdPinDB3,
    LcdPinDB4,
    LcdPinDB5,
    LcdPinDB6,
    LcdPinDB7,
    LcdPinReadWrite,
    LcdPinRegisterSelect,
    LcdPinEnable,
    LcdPinCount
} LcdPin;

typedef enum {
    LcdPinDirectionInput,
    LcdPinDirectionOutput,
    LcdPinDirectionCount
} LcdPinDirection;

typedef enum {
    LcdInterface4Bit,
    LcdInterface8Bit,
    LcdInterfaceCount
} LcdInterface;

typedef enum {
    LcdFontType5x8,
    LcdFontType5x11,
    LcdFontTypeCount
} LcdFontType;

typedef enum {
    LcdOneLineMode,
    LcdTwoLineMode,
    LcdLineModeCount
} LcdLineMode;

typedef enum {
    LcdDirectionLeft,
    LcdDirectionRight,
    LcdDirectionCount
} LcdDirection;

typedef LcdErr (* LcdPinWriteCallback)(LcdPin pin, LcdPinState state);
typedef LcdErr (* LcdPinReadCallback)(LcdPin pin, LcdPinState *state);
typedef LcdErr (* LcdPinConfigCallback)(LcdPin pin, LcdPinDirection);
typedef void (* LcdDelayUsCallback)(unsigned short us);

typedef struct {
    LcdPinWriteCallback pinWriteCb;
    LcdPinReadCallback pinReadCb;
    LcdPinConfigCallback pinConfigCb;
    LcdDelayUsCallback delayUsCb;
} LcdHandle;

LcdErr lcdInit(LcdHandle *handle, LcdInterface mode, LcdFontType font, LcdLineMode lineMode);
LcdErr lcdClearScreen(void);
LcdErr lcdCursorReturnHome(void);
LcdErr lcdShiftDirectionSet(LcdDirection dir);
LcdErr lcdDisplayShiftEnable(void);
LcdErr lcdDisplayShiftDisable(void);
LcdErr lcdTurnOn(void);
LcdErr lcdTurnOff(void);
LcdErr lcdCursorOn(void);
LcdErr lcdCursorOff(void);
LcdErr lcdCursorBlinkOn(void);
LcdErr lcdCursorBlinkOff(void);
LcdErr lcdCursorShift(LcdDirection dir);
LcdErr lcdDisplayShift(LcdDirection dir);
LcdErr lcdCGRAMAddrSet(unsigned char addr);
LcdErr lcdDDRAMAddrSet(unsigned char addr);
LcdErr lcdCheckBusyFlag(void);
LcdErr lcdReadAddressCounter(unsigned char *addressCounter);
LcdErr lcdPringChar(unsigned char symbol);
LcdErr lcdPrint(unsigned char text[], unsigned char len);
LcdErr lcdCursorPositionSet(unsigned char line, unsigned char position);

