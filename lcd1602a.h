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
    LcdPinReadWrite = 0U,
    LcdPinRegisterSelect,
    LcdPinEnable,
    LcdPinDB0,
    LcdPinDB1,
    LcdPinDB2,
    LcdPinDB3,
    LcdPinDB4,
    LcdPinDB5,
    LcdPinDB6,
    LcdPinDB7,
    LcdPinCount
} LcdPin;

typedef enum {
    LcdPinDirectionInput,
    LcdPinDirectionOutput,
    LcdPinDirectionCount
} LcdPinDirection;

typedef enum {
    LcdMode4Bit,
    LcdMode8Bit,
    LcdModeCount
} LcdMode;

typedef enum {
    LcdMoveDirectionRight,
    LcdMoveDirectionLeft,
    LcdMoveDirectionCount
} LcdMoveDirection;

typedef LcdErr (* LcdPinWriteCallback)(LcdPin pin, LcdPinState state);
typedef LcdErr (* LcdPinReadCallback)(LcdPin pin, LcdPinState *state);
typedef LcdErr (* LcdPinConfigCallback)(LcdPin pin, LcdPinDirection);
typedef void (* LcdDelayUsCallback)(unsigned short us);

typedef struct {
    LcdMode mode;
    LcdPinWriteCallback pinWriteCb;
    LcdPinReadCallback pinReadCb;
    LcdPinConfigCallback pinConfigCb;
    LcdDelayUsCallback delayCb;
} LcdHandle;

LcdErr lcdInit(LcdHandle *handle);
LcdErr lcdEnable(void);
LcdErr lcdDisable(void);
LcdErr lcdClearScreen(void);
LcdErr lcdPringChar(unsigned char symbol, unsigned int row, unsigned int position);
LcdErr lcdPrint(unsigned char text[], unsigned int len);
LcdErr lcdCursorReturnHome(void);
LcdErr lcdCursorSetPosition(unsigned int row, unsigned int position);
LcdErr lcdCursorBlinkOn(void);
LcdErr lcdCursorBlinkOff(void);
LcdErr lcdSetMovingDirection(LcdMoveDirection dir);
LcdErr lcdShiftDisplay(LcdMoveDirection dir);
