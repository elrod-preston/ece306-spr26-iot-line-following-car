//===========================================================================
// File Name : detect.c
//
// Description:
//   Simple two-sensor raw ADC detection.
//   - No calibration
//   - No filtering
//   - No emitter subtraction
//   - No thresholds
//   - No sensor flipping
//   - Only raw left/right ADC values
//   - Only reads when detect_request is high
//   - Output values are averaged from three readings
//===========================================================================

#include <msp430.h>

#include "Core\\lib\\ports.h"
#include "Core\\lib\\adc.h"
#include "Addon\\lib\\detect.h"

extern volatile unsigned long system_ticks_ms;

#define DELAY (0u)


//---------------------------------------------------------------------------
// Public globals
//---------------------------------------------------------------------------

volatile unsigned int detect_left_raw  = 0u;
volatile unsigned int detect_right_raw = 0u;

volatile unsigned char detect_request = 0u;

//---------------------------------------------------------------------------
// Local globals
//---------------------------------------------------------------------------

static unsigned long start_time = 0u;
static unsigned char detect_waiting = 0u;

//---------------------------------------------------------------------------
// Initialize detection module
//---------------------------------------------------------------------------

void Detect_Init(void)
{
  detect_left_raw  = 0u;
  detect_right_raw = 0u;

  P2OUT &= ~IR_LED;              // Drive P2.2 low, IR LED off

  start_time = 0u;
  detect_waiting = 0u;
  detect_request = 0u;
}

//---------------------------------------------------------------------------
// Run one requested raw sensor update
//---------------------------------------------------------------------------

void Detect_Task(void)
{
  unsigned long left_sum;
  unsigned long right_sum;
  unsigned long now;

  // No request pending
  if (!detect_request)
  {
    P2OUT |= IR_LED;            // IR LED off
    detect_waiting = 0u;
    return;
  }

  now = system_ticks_ms;

  // First task cycle after request:
  // turn on IR LED and start the delay timer
  if (!detect_waiting)
  {
    P2OUT |= IR_LED;             // IR LED on
    start_time = now;
    detect_waiting = 1u;
    return;
  }

  // Wait until IR LED has been on long enough.
  // Subtraction handles timer rollover safely.
  if ((unsigned long)(now - start_time) < DELAY)
  {
    return;
  }


  detect_left_raw  = ADC_ReadChannel(ADC_CH_LEFT);
  detect_right_raw = ADC_ReadChannel(ADC_CH_RIGHT);

  // Finish request
  detect_request = 0u;
  detect_waiting = 0u;

  P2OUT |= IR_LED;              // IR LED off after reading
}
