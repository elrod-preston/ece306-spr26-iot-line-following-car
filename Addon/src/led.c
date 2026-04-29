#include "Core\lib\macros.h"
#include "Core\lib\ports.h"
#include "msp430.h"

//===========================================================================
// External timebase
//===========================================================================

extern volatile unsigned long system_ticks_ms;

//===========================================================================
// Strobe settings
//===========================================================================

#define STROBE_RATE_MS   (80u)

//===========================================================================
// Strobe_Task
//
// Call repeatedly from main loop.
// Fast alternating red / green strobe.
//===========================================================================

void Strobe_Task(void)
{
  static unsigned long last_tick = 0u;
  static unsigned char on_green = 0u;

  if ((system_ticks_ms - last_tick) < STROBE_RATE_MS) {
    return;
  }

  last_tick = system_ticks_ms;

  if (on_green) {
    P6OUT |= GRN_LED;
    P1OUT &= ~RED_LED;
    on_green = 0u;
  } else {
    P1OUT |= RED_LED;
    P6OUT &= ~GRN_LED;
    on_green = 1u;
  }
}
