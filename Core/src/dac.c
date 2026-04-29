//===========================================================================
// File Name : dac.c
//
// Description:
// Simple DAC configuration for MSP430.
//
// Display format during ramp:
//   DAC RAMP
//   xxxx
//
// Notes:
//   - Configures P3.5 for DAC function
//   - Enables DAC power on P2.5
//   - Uses SAC3 in DAC + OA buffer mode
//   - TimerB0 overflow interrupt steps DAC output down until limit reached
//===========================================================================

#include "msp430.h"
#include "Core\lib\dac.h"
#include "Core\lib\ports.h"
#include "Core\lib\display.h"

//---------------------------------------------------------------------------
// Global DAC data
//---------------------------------------------------------------------------
volatile unsigned int DAC_data = DAC_Begin;

//---------------------------------------------------------------------------
// Local helpers
//---------------------------------------------------------------------------
static void DAC_DisplayValue(unsigned int value);

//---------------------------------------------------------------------------
// Init_DAC
//
// Purpose:
//   Initialize DAC hardware in simple mode.
//---------------------------------------------------------------------------
void Init_DAC(void)
{
  //-------------------------------------------------------------------------
  // Show startup value
  //-------------------------------------------------------------------------
  Display_Text(0, "DAC RAMP");
  DAC_DisplayValue(DAC_data);
  Display_Text(2, " ");
  Display_Text(3, " ");
  UpdateDisplay();

  //-------------------------------------------------------------------------
  // Configure Port 3 Pin 5 for DAC function
  //-------------------------------------------------------------------------
  P3SELC |= DAC_CNTL;

  //-------------------------------------------------------------------------
  // Enable DAC power on Port 2 Pin 5
  //-------------------------------------------------------------------------
  P2SEL0 &= ~DAC_ENB;
  P2SEL1 &= ~DAC_ENB;
  P2DIR  |= DAC_ENB;
  P2OUT  |= DAC_ENB;

  //-------------------------------------------------------------------------
  // Configure SAC3 DAC
  //-------------------------------------------------------------------------
  SAC3DAC = DACSREF_0;     // VCC as DAC reference
  SAC3DAC |= DACLSEL_0;    // DAC latch loads when DACDAT written

  SAC3OA = NMUXEN;         // Negative input MUX enable
  SAC3OA |= PMUXEN;        // Positive input MUX enable
  SAC3OA |= PSEL_1;        // 12-bit reference DAC source selected
  SAC3OA |= NSEL_1;        // Select negative pin input
  SAC3OA |= OAPM;          // Low speed / low power mode

  SAC3PGA = MSEL_1;        // OA buffer mode

  SAC3OA |= SACEN;         // Enable SAC
  SAC3OA |= OAEN;          // Enable OA

  //-------------------------------------------------------------------------
  // Set initial DAC value
  //-------------------------------------------------------------------------
  DAC_data = DAC_Begin;
  SAC3DAT = DAC_data;

  // show true starting value after init
  DAC_DisplayValue(DAC_data);
  UpdateDisplay();

  //-------------------------------------------------------------------------
  // Enable TimerB0 overflow interrupt
  //-------------------------------------------------------------------------
  TB0CTL |= TBIE;

  //-------------------------------------------------------------------------
  // Indicate ramp active
  //-------------------------------------------------------------------------
  RED_LED_ON;

  //-------------------------------------------------------------------------
  // Enable DAC
  //-------------------------------------------------------------------------
  SAC3DAC |= DACEN;
}

//---------------------------------------------------------------------------
// DAC_TimerOverflow_ISR
//
// Purpose:
//   Called when TimerB0 overflow occurs.
//   Steps DAC downward until DAC_Limit is reached.
//---------------------------------------------------------------------------
void DAC_TimerOverflow_ISR(void)
{
  if (DAC_data > 100u) {
    DAC_data = DAC_data - 250u;
  } else {
    DAC_data = 0u;
  }

  if (DAC_data <= DAC_Limit) {
    DAC_data = DAC_Adjust;
    TB0CTL &= ~TBIE;   // Disable TimerB0 overflow interrupt
    RED_LED_OFF;       // DAC voltage set
  }

  SAC3DAT = DAC_data;

  // update display every overflow
  Display_Text(0, "DAC RAMP");
  DAC_DisplayValue(DAC_data);
  UpdateDisplay();
}

//---------------------------------------------------------------------------
// DAC_DisplayValue
//
// Purpose:
//   Shows the DAC value on row 1, left aligned.
//   Example:
//     2725
//      875
//---------------------------------------------------------------------------
static void DAC_DisplayValue(unsigned int value)
{
  char line[11];
  unsigned int d1000;
  unsigned int d100;
  unsigned int d10;
  unsigned int d1;
  unsigned int i = 0u;

  d1000 = value / 1000u;
  value  = value % 1000u;
  d100  = value / 100u;
  value  = value % 100u;
  d10   = value / 10u;
  d1    = value % 10u;

  if (d1000 > 0u) {
    line[i++] = (char)('0' + d1000);
    line[i++] = (char)('0' + d100);
    line[i++] = (char)('0' + d10);
    line[i++] = (char)('0' + d1);
  } else if (d100 > 0u) {
    line[i++] = (char)('0' + d100);
    line[i++] = (char)('0' + d10);
    line[i++] = (char)('0' + d1);
  } else if (d10 > 0u) {
    line[i++] = (char)('0' + d10);
    line[i++] = (char)('0' + d1);
  } else {
    line[i++] = (char)('0' + d1);
  }

  while (i < 10u) {
    line[i++] = ' ';
  }
  line[10] = 0;

  Display_Text(1, line);
}
