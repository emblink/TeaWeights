#include "lcd1602a.h"
#include "hx711.h"
#include "stm8s_clk.h"
#include "stm8s_tim1.h"
#include "stm8s_tim2.h"
#include "stm8s_gpio.h"

#define NULL (void *) 0

 // PD1 - is swim by default.
static struct {
	GPIO_TypeDef *portBase;
	GPIO_Pin_TypeDef pin;
} LcdPinMap[] = {
    // [LcdPinDB0] = {.portBase = GPIOC, .pin = GPIO_PIN_6},
	// [LcdPinDB1] = {.portBase = GPIOB, .pin = GPIO_PIN_4},
	// [LcdPinDB2] = {.portBase = GPIOD, .pin = GPIO_PIN_2},
	// [LcdPinDB3] = {.portBase = GPIOD, .pin = GPIO_PIN_3},                                                              
	[LcdPinDB4] = {.portBase = GPIOC, .pin = GPIO_PIN_4},
	[LcdPinDB5] = {.portBase = GPIOC, .pin = GPIO_PIN_5},
	[LcdPinDB6] = {.portBase = GPIOC, .pin = GPIO_PIN_6},
	[LcdPinDB7] = {.portBase = GPIOC, .pin = GPIO_PIN_7},
    [LcdPinReadWrite] = {.portBase = GPIOA, .pin = GPIO_PIN_2},
	[LcdPinRegisterSelect] = {.portBase = GPIOA, .pin = GPIO_PIN_1},
	[LcdPinEnable] = {.portBase = GPIOA, .pin = GPIO_PIN_3},
};

static struct {
	GPIO_TypeDef *portBase;
	GPIO_Pin_TypeDef pin;
} hx711PinMap[] = {
    [Hx711PinClock] = {.portBase = GPIOB, .pin = GPIO_PIN_5},                                                  
	[Hx711PinData] = {.portBase = GPIOB, .pin = GPIO_PIN_4},
};

static volatile uint16_t timeTick = 0;
static volatile uint8_t usDelayPassed = 0;

static LcdErr lcdPinWriteCallback(LcdPin pin, LcdPinState state)
{
	if (pin >= LcdPinCount || state >= LcdPinStateCount)
		return LcdErrParam;

    if (state == LcdPinStateHigh) {
        GPIO_WriteHigh(LcdPinMap[pin].portBase, LcdPinMap[pin].pin);
    } else {
        GPIO_WriteLow(LcdPinMap[pin].portBase, LcdPinMap[pin].pin);
    }
    return LcdErrOk;
}

static LcdErr lcdPinReadCallback(LcdPin pin, LcdPinState *state)
{
	if (pin >= LcdPinCount || state == NULL)
		return LcdErrParam;

    if (GPIO_ReadInputPin(LcdPinMap[pin].portBase, LcdPinMap[pin].pin)) {
        *state = LcdPinStateHigh;
    } else {
        *state = LcdPinStateLow;
    }
    return LcdErrOk;
}

static LcdErr lcdPinConfigCallback(LcdPin pin, LcdPinDirection dir)
{
	if (pin >= LcdPinCount || dir >= LcdPinDirectionCount)
		return LcdErrParam;

    if (dir == LcdPinDirectionInput) {
        GPIO_Init(LcdPinMap[pin].portBase, LcdPinMap[pin].pin, GPIO_MODE_IN_FL_NO_IT);
    } else {
        GPIO_Init(LcdPinMap[pin].portBase, LcdPinMap[pin].pin, GPIO_MODE_OUT_PP_LOW_SLOW);
    }
    return LcdErrOk;
}

static Hx711Err hx711PinWriteCallback(Hx711Pin pin, Hx711PinState state)
{
	if (pin >= Hx711PinCount || state >= Hx711PinStateCount)
		return Hx711ErrParam;

    if (state == Hx711PinStateHigh) {
        GPIO_WriteHigh(hx711PinMap[pin].portBase, hx711PinMap[pin].pin);
    } else {
        GPIO_WriteLow(hx711PinMap[pin].portBase, hx711PinMap[pin].pin);
    }
    return Hx711ErrOk;
}

static Hx711Err hx711PinReadCallback(Hx711Pin pin, Hx711PinState *state)
{
	if (pin >= Hx711PinCount || state == NULL)
		return Hx711ErrParam;

    if (GPIO_ReadInputPin(hx711PinMap[pin].portBase, hx711PinMap[pin].pin)) {
        *state = Hx711PinStateHigh;
    } else {
        *state = Hx711PinStateLow;
    }
    return Hx711ErrOk;
}

static uint16_t getTimeTick(void)
{
    disableInterrupts();
    uint16_t tick = timeTick;
    enableInterrupts();
    return tick;
}

static void delayUs(uint16_t us)
{
    usDelayPassed = 0;
    TIM2_SetCounter(0);
	TIM2_SetAutoreload(us);
    TIM2_Cmd(ENABLE);
    while(usDelayPassed == 0) { }
}

static void delayMs(uint16_t ms)
{
    uint16_t timestamp = getTimeTick();
    while (getTimeTick() - timestamp < ms) {

    }
}

int main( void )
{
    /* Configure system clock */
	CLK_HSICmd(ENABLE);
	while (!CLK->ICKR & (1 << 1)); // wait untill clock became stable
	CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1); // 16MHz SYS
	CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1); // 16MHz CPU

    /* Enable clocks for peripherals */
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, ENABLE);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);

    /* Timer 1 Init, ms timer*/
	TIM1_DeInit();
	TIM1_PrescalerConfig(16 - 1, TIM1_PSCRELOADMODE_IMMEDIATE); // fCK_CNT = fCK_PSC/(PSCR[15:0]+1)
	TIM1_SetAutoreload(1000); // frequency = 16 000 000 / 16 / 1000 = 1000 Hz
    TIM1_ClearITPendingBit(TIM1_IT_UPDATE);
	TIM1_ITConfig(TIM1_IT_UPDATE, ENABLE);
	TIM1_CounterModeConfig(TIM1_COUNTERMODE_UP);

    /* Timer 2 Init, us timer*/
    TIM2_DeInit();
	TIM2_UpdateRequestConfig(TIM2_UPDATESOURCE_REGULAR);
	TIM2_PrescalerConfig(TIM2_PRESCALER_16, TIM2_PSCRELOADMODE_IMMEDIATE); // frequency = 16M / 16 / = 1Mhz
	TIM2_SetAutoreload(0);
	TIM2_SelectOnePulseMode(TIM2_OPMODE_SINGLE);
    TIM2_ClearITPendingBit(TIM2_IT_UPDATE);
	TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);

    enableInterrupts();
	
    /* Start application timer */
	TIM1_Cmd(ENABLE);

    /* Onboard led init */
    GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_SLOW);
    
    Hx711Handle hx711Handle = {
        .pinWriteCb = hx711PinWriteCallback,
        .pinReadCb = hx711PinReadCallback,
        .delayUsCb = delayUs
    };
    hx711Init(&hx711Handle);

	LcdHandle lcdHandle = {
		.pinWriteCb = lcdPinWriteCallback,
		.pinReadCb = lcdPinReadCallback,
		.pinConfigCb = lcdPinConfigCallback,
		.delayUsCb = delayUs
	};

    /* Wait for more than 40 ms after VCC rises to 2.7 V */
    delayMs(50);
    lcdInit(&lcdHandle, LcdInterface4Bit, LcdFontType5x8, LcdTwoLineMode);
    lcdTurnOn();
    delayUs(100);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdCursorOn();
    delayUs(100);
    while(lcdCheckBusyFlag() == LcdErrBusy) { }
    lcdClearScreen();
    while(lcdCheckBusyFlag() == LcdErrBusy) { }

    unsigned char message[] = "Prr... Prr..... ^_^";

    while (1) {
        static unsigned char line = 0;
        line = (line + 1) % 2;
        lcdCursorPositionSet(line, 1);
        while(lcdCheckBusyFlag() == LcdErrBusy) { }
        for (unsigned int i = 0; i < sizeof(message) - 1; i++) {
            lcdPringChar(message[i]);
            GPIO_WriteReverse(GPIOB, GPIO_PIN_5);
            delayMs(150); 
        }
        // for (unsigned char i = 0; i < 5 - 1; i++) {
        //     lcdDisplayShift(LcdDirectionLeft);
        //     delayMs(200);
        //     lcdDisplayShift(LcdDirectionRight);
        //     delayMs(200);
        // }
        lcdClearScreen();
        GPIO_WriteReverse(GPIOB, GPIO_PIN_5);
        delayMs(500);
    }
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif // USE_FULL_ASSERT

/* Application Timer Interrupt Handler */
/**
  * @brief Timer1 Update/Overflow/Trigger/Break Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM1_UPD_OVF_TRG_BRK_IRQHandler, 11)
{
	TIM1_ClearITPendingBit(TIM1_IT_UPDATE);
	timeTick++;
}

INTERRUPT_HANDLER(TIM2_UPD_OVF_BRK_IRQHandler, 13)
{
	TIM2_ClearITPendingBit(TIM2_IT_UPDATE);
    usDelayPassed = 1;
}