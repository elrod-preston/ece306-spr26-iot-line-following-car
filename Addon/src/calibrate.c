//===========================================================================
// File Name : calibrate.c
//
// Description:
// Simple calibration state module using detect.c raw read system.
//   - Does NOT read ADC directly
//   - Uses Detect_Task()
//   - Uses detect_left_raw and detect_right_raw
//   - Captures white on first SW1 press
//   - Captures black on second SW1 press
//   - Finishes on third SW1 press
//
// Notes:
//   - No motor logic
//   - No movement logic
//   - detect_request should be raised every 50ms by interrupt.c
//===========================================================================

#include <msp430.h>

#include "Core\\lib\\display.h"

#include "Addon\\lib\\calibrate.h"
#include "Addon\\lib\\detect.h"

extern volatile unsigned char sw1_pressed;

//---------------------------------------------------------------------------
// Exposed calibration globals
//---------------------------------------------------------------------------

volatile unsigned int calibrate_white_left  = 0u;
volatile unsigned int calibrate_white_right = 0u;
volatile unsigned int calibrate_black_left  = 0u;
volatile unsigned int calibrate_black_right = 0u;

volatile unsigned char calibrate_have_white = 0u;
volatile unsigned char calibrate_have_black = 0u;
volatile unsigned char calibrate_done       = 0u;
volatile unsigned int calibrate_state       = CAL_ST_WHITE;

//---------------------------------------------------------------------------
// Local helper: build "L: 1234   " style row
//---------------------------------------------------------------------------

static void Build_Row(char label, unsigned int value, char *row)
{
  unsigned int temp;
  unsigned int digits;
  unsigned int divisor;
  unsigned int i;

  row[0] = label;
  row[1] = ':';
  row[2] = ' ';

  temp = value;
  digits = 1u;

  while (temp >= 10u)
  {
    temp /= 10u;
    digits++;
  }

  divisor = 1u;

  for (i = 1u; i < digits; i++)
  {
    divisor *= 10u;
  }

  temp = value;
  i = 3u;

  while ((divisor > 0u) && (i < 10u))
  {
    row[i] = (char)('0' + (temp / divisor));
    temp %= divisor;
    divisor /= 10u;
    i++;
  }

  while (i < 10u)
  {
    row[i] = ' ';
    i++;
  }

  row[10] = '\0';
}

//---------------------------------------------------------------------------
// Local helper: update display
//---------------------------------------------------------------------------

static void Calibrate_Display(unsigned int L, unsigned int R)
{
  char row1[11];
  char row2[11];

  Build_Row('L', L, row1);
  Build_Row('R', R, row2);

  Display_Text(0, "CALIBRATE ");
  Display_Text(1, row1);
  Display_Text(2, row2);

  if (calibrate_state == CAL_ST_WHITE)
  {
    Display_Text(3, "WHITE->SW1");
  }
  else if (calibrate_state == CAL_ST_BLACK)
  {
    Display_Text(3, "BLACK->SW1");
  }
  else if (calibrate_state == CAL_ST_READY)
  {
    Display_Text(3, "SW1 START ");
  }
  else
  {
    Display_Text(3, "DONE      ");
  }

  UpdateDisplay();
}

//---------------------------------------------------------------------------
// Initialize calibration state
//---------------------------------------------------------------------------

void Calibrate_Init(void)
{
  sw1_pressed = 0u;

  calibrate_white_left  = 0u;
  calibrate_white_right = 0u;
  calibrate_black_left  = 0u;
  calibrate_black_right = 0u;

  calibrate_have_white = 0u;
  calibrate_have_black = 0u;
  calibrate_done       = 0u;
  calibrate_state      = CAL_ST_WHITE;

  Detect_Init();

  // Force one immediate read request.
  // After this, interrupt.c can raise detect_request every 50ms.
  detect_request = 1u;
  Detect_Task();

  Calibrate_Display(detect_left_raw, detect_right_raw);
}

//---------------------------------------------------------------------------
// Run one calibration tick
//---------------------------------------------------------------------------

void Calibrate_Task(void)
{
  unsigned int L;
  unsigned int R;

  L = detect_left_raw;
  R = detect_right_raw;

  if (sw1_pressed)
  {
    sw1_pressed = 0u;

    if (calibrate_state == CAL_ST_WHITE)
    {
      calibrate_white_left  = L;
      calibrate_white_right = R;

      calibrate_have_white = 1u;
      calibrate_state      = CAL_ST_BLACK;
    }
    else if (calibrate_state == CAL_ST_BLACK)
    {
      calibrate_black_left  = L;
      calibrate_black_right = R;

      calibrate_have_black = 1u;
      calibrate_state      = CAL_ST_READY;
    }
    else if (calibrate_state == CAL_ST_READY)
    {
      calibrate_state = CAL_ST_DONE;
      calibrate_done  = 1u;
    }
  }

  Calibrate_Display(L, R);
}

//---------------------------------------------------------------------------
// Done flag
//---------------------------------------------------------------------------

unsigned char Calibrate_IsDone(void)
{
  return calibrate_done;
}
