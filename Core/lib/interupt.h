//==============================================================================
// File Name : interupt.h
//
// Description:
//  - Header for switch interrupts, backlight pulse feedback, and 1ms system tick
//
// Author: Preston Elrod
// Date: Feb 2026
//==============================================================================

#ifndef INTERUPT_H
#define INTERUPT_H

#include "msp430.h"

#define SWITCH_DEBOUNCE_MS     (50u)
#define BACKLIGHT_PULSE_MS     (40u)
#define DISPLAY_UPDATE_MS      (200u)

// legacy globals kept for compatibility
extern volatile unsigned int Time_Sequence;
extern volatile char one_time;

// switch event flags
extern volatile unsigned char sw1_pressed;
extern volatile unsigned char sw2_pressed;

// 1ms free-running tick counter
extern volatile unsigned long system_ticks_ms;

// backlight pulse countdown
extern volatile unsigned int backlight_pulse_ms;

// simple timing helpers
unsigned long Get_System_Ticks(void);
unsigned char HasElapsed(unsigned long start, unsigned long interval);

// backlight helpers
void Backlight_On(void);
void Backlight_Off(void);
void Backlight_Pulse(unsigned int ms);

#endif
