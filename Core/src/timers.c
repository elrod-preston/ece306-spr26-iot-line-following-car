//===========================================================================
// File Name : timers.c
//
// TB0 = general time base / debounce / display tick
// TB3 = PWM for backlight + motor speed control
//===========================================================================

#include "Core\\lib\\timers.h"
#include "Core\\lib\\macros.h"
#include "Core\\lib\\motor.h"
#include "msp430.h"

//===========================================================================
// TB0: continuous mode timing base
//===========================================================================

void Init_Timer_B0(void)
{
  TB0CTL  = RESET_REGISTER;
  TB0EX0  = RESET_REGISTER;

  TB0CTL |= TBSSEL__SMCLK;       // SMCLK source
  TB0CTL |= MC__CONTINUOUS;      // Continuous mode
  TB0CTL |= ID__8;               // /8 divider
  TB0EX0 |= TBIDEX__8;           // /8 expansion => total /64
  TB0CTL |= TBCLR;               // Clear timer

  TB0CCR0  = TB0R + TB0CCR0_INTERVAL;
  TB0CCTL0 = CCIE;

  TB0CCR1  = TB0R + TB0CCR1_INTERVAL;
  TB0CCTL1 = 0;

  TB0CTL &= ~TBIE;
  TB0CTL &= ~TBIFG;
}

//===========================================================================
// TB3: PWM generator for LCD + motor speed
//
// P6.0 TB3.1 LCD_BACKLITE
// P6.1 TB3.2 RIGHT_FORWARD
// P6.2 TB3.3 LEFT_FORWARD
// P6.3 TB3.4 RIGHT_REVERSE
// P6.4 TB3.5 LEFT_REVERSE
//===========================================================================

void Init_Timer_B3(void)
{
  TB3CTL = 0;
  TB3EX0 = 0;

  TB3CTL |= TBSSEL__SMCLK;       // SMCLK
  TB3CTL |= ID__1;               // /1
  TB3EX0 |= TBIDEX__1;           // /1
  TB3CTL |= TBCLR;               // Clear timer

  TB3CCR0 = WHEEL_PERIOD;        // PWM period

  /* RIGHT FORWARD : TB3.2 */
  TB3CCTL2 = OUTMOD_7;
  TB3CCR2  = WHEEL_OFF;

  /* LEFT FORWARD : TB3.3 */
  TB3CCTL3 = OUTMOD_7;
  TB3CCR3  = WHEEL_OFF;

  /* RIGHT REVERSE : TB3.4 */
  TB3CCTL4 = OUTMOD_7;
  TB3CCR4  = WHEEL_OFF;

  /* LEFT REVERSE : TB3.5 */
  TB3CCTL5 = OUTMOD_7;
  TB3CCR5  = WHEEL_OFF;

  TB3CTL |= MC__UP;              // Start in up mode
}
