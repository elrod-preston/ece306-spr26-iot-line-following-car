//==============================================================================
// File Name : interrupt.c
//
// Description:
//  - TB0 CCR0 @ 1ms: system heartbeat tick
//  - TB0 overflow: DAC settling / ramp handling
//  - SW1 (P4.1) PORT4 ISR: sets sw1_pressed flag, starts SW1 debounce
//  - SW2 (P2.3) PORT2 ISR: sets sw2_pressed flag, starts SW2 debounce
//  - Any switch press gives one short backlight pulse
//  - Display update flag is raised every 200ms
//
// IMPORTANT:
//  - Do NOT call Display_Update() / SPI / LCD updates inside ISRs.
//  - Backlight is GPIO only, not PWM.
//==============================================================================

#include "msp430.h"
#include "Core\lib\interupt.h"
#include "Core\lib\display.h"
#include "Core\lib\ports.h"
#include "Core\lib\timers.h"
#include "Core\lib\dac.h"
#include "Addon\lib\detect.h"

extern volatile unsigned char update_display;
extern volatile unsigned char display_changed;

volatile unsigned int Time_Sequence = 0;
volatile char one_time = 0;

volatile unsigned char sw1_pressed = 0;
volatile unsigned char sw2_pressed = 0;

volatile unsigned long system_ticks_ms = 0;
volatile unsigned int backlight_pulse_ms = 0;

static volatile unsigned int sw1_db_ms = 0;
static volatile unsigned int sw2_db_ms = 0;

#define TB0CCR0_INTERVAL       (125u)     // 1ms @ SMCLK=8MHz, ID__8, TBIDEX__8
#define DISPLAY_DIVIDER_MS     (200u)
#define DETECT_DIVIDER_MS      (8u)

//==============================================================================
// PORT4 ISR (SW1)
//==============================================================================

#pragma vector=PORT4_VECTOR
__interrupt void switchP4_interrupt(void)
{
  if (P4IFG & SW1) {
    P4IFG &= ~SW1;

    if (sw1_db_ms == 0u) {
      sw1_pressed = 1u;
      sw1_db_ms = SWITCH_DEBOUNCE_MS;
      P4IE &= ~SW1;

      P6OUT |= LCD_BACKLITE;
      backlight_pulse_ms = BACKLIGHT_PULSE_MS;
    }
  }
}

//==============================================================================
// PORT2 ISR (SW2)
//==============================================================================

#pragma vector=PORT2_VECTOR
__interrupt void switchP2_interrupt(void)
{
  if (P2IFG & SW2) {
    P2IFG &= ~SW2;

    if (sw2_db_ms == 0u) {
      sw2_pressed = 1u;
      sw2_db_ms = SWITCH_DEBOUNCE_MS;
      P2IE &= ~SW2;

      P6OUT |= LCD_BACKLITE;
      backlight_pulse_ms = BACKLIGHT_PULSE_MS;
    }
  }
}

//==============================================================================
// TIMER0_B0_VECTOR (CCR0) - 1ms system tick
//==============================================================================

#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void)
{
  static unsigned int display_ms = 0u;
  static unsigned int detect_ms = 0u;

  system_ticks_ms++;
  Time_Sequence++;
  one_time = 1;

  // schedule next CCR0 event
  TB0CCR0 += TB0CCR0_INTERVAL;

  //--------------------------------------------------------------------------
  // SW1 debounce
  //--------------------------------------------------------------------------
  if (sw1_db_ms > 0u) {
    sw1_db_ms--;
    if (sw1_db_ms == 0u) {
      P4IFG &= ~SW1;
      P4IE  |= SW1;
    }
  }

  //--------------------------------------------------------------------------
  // SW2 debounce
  //--------------------------------------------------------------------------
  if (sw2_db_ms > 0u) {
    sw2_db_ms--;
    if (sw2_db_ms == 0u) {
      P2IFG &= ~SW2;
      P2IE  |= SW2;
    }
  }

  //--------------------------------------------------------------------------
  // Backlight pulse timing
  //--------------------------------------------------------------------------
  if (backlight_pulse_ms > 0u) {
    backlight_pulse_ms--;
    if (backlight_pulse_ms == 0u) {
      P6OUT &= ~LCD_BACKLITE;
    }
  }

  //--------------------------------------------------------------------------
  // Raise raw detect request every 50ms
  //--------------------------------------------------------------------------
  detect_ms++;
  if (detect_ms >= DETECT_DIVIDER_MS) {
    detect_ms = 0u;
    detect_request = 1u;
  }

  //--------------------------------------------------------------------------
  // Raise display refresh request every 200ms
  //--------------------------------------------------------------------------
  display_ms++;
  if (display_ms >= DISPLAY_DIVIDER_MS) {
    display_ms = 0u;
    update_display = 1;
    display_changed = 1;
  }
}

//==============================================================================
// TIMER0_B1_VECTOR
//
// Handles all TimerB0 sources except CCR0.
// We only use overflow here for DAC stepping.
//==============================================================================

#pragma vector = TIMER0_B1_VECTOR
__interrupt void Timer0_B1_ISR(void)
{
  switch(__even_in_range(TB0IV,14))
  {
    case 0:   // no interrupt
      break;

    case 2:   // CCR1
      break;

    case 4:   // CCR2
      break;

    case 6:   // CCR3
      break;

    case 8:   // CCR4
      break;

    case 10:  // CCR5
      break;

    case 12:  // CCR6
      break;

    case 14:  // overflow
      DAC_TimerOverflow_ISR();
      break;

    default:
      break;
  }
}
