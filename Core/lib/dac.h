//===========================================================================
// File Name : dac.h
//
// Description:
// Simple DAC configuration for MSP430.
//
// Notes:
//   - Uses SAC3 DAC / OA buffer mode
//   - P3.5 is DAC control pin
//   - P2.5 enables DAC power
//   - Call Init_DAC() once during setup
//   - Call DAC_TimerOverflow_ISR() from TimerB0 overflow case
//===========================================================================

#ifndef DAC_H
#define DAC_H

#include <msp430.h>

//---------------------------------------------------------------------------
// DAC settings
//---------------------------------------------------------------------------
#define DAC_Begin   (2725u)   // ~2.0V start
#define DAC_Limit   (1500)    // ~6.08V limit
#define DAC_Adjust  (1505)    // ~6.00V final adjust

//---------------------------------------------------------------------------
// LED macros
//---------------------------------------------------------------------------
// Replace with your project LED macros if already defined elsewhere
#define RED_LED_ON   (P1OUT |= BIT0)
#define RED_LED_OFF  (P1OUT &= ~BIT0)

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------
extern volatile unsigned int DAC_data;

//---------------------------------------------------------------------------
// Function Prototypes
//---------------------------------------------------------------------------

// Configure DAC-related pins and initialize SAC3 DAC
void Init_DAC(void);

// Handles the TimerB0 overflow DAC ramp logic
void DAC_TimerOverflow_ISR(void);

#endif
