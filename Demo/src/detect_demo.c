/*
///===========================================================================
// File Name : Demo\src\detect_demo.c
//
// Description:
//   Simple detector demo.
//   - Calls Detect_Task() repeatedly
//   - Displays logical left/right raw values
//   - Displays logical left/right normalized levels
//   - Displays on-line flags and signed line position
//
// Notes:
//   - Uses the new display API:
//       Display_Text()
//       UpdateDisplay()
//   - Assumes detect.c / calibrate.c are already set up
//   - Assumes 4-line 10-char LCD
//   - Uses integer formatting only
//===========================================================================

#include <msp430.h>

#include "Core\\lib\\display.h"
#include "Addon\\lib\\detect.h"
#include "Demo\\lib\\detect_demo.h"

//---------------------------------------------------------------------------
// Local display buffers
//---------------------------------------------------------------------------

static char line0[11];
static char line1[11];
static char line2[11];
static char line3[11];

//---------------------------------------------------------------------------
// Local helpers
//---------------------------------------------------------------------------

static void PutString10(char *dst, const char *src)
{
  unsigned int i = 0u;

  while((i < 10u) && (src[i] != 0))
  {
    dst[i] = src[i];
    i++;
  }

  while(i < 10u)
  {
    dst[i] = ' ';
    i++;
  }

  dst[10] = 0;
}

static void PutChar(char *s, unsigned int index, char c)
{
  if(index < 10u)
  {
    s[index] = c;
  }
}

static void PutU4(char *s, unsigned int index, unsigned int value)
{
  if(value > 9999u)
  {
    value = 9999u;
  }

  s[index + 0u] = (char)('0' + ((value / 1000u) % 10u));
  s[index + 1u] = (char)('0' + ((value / 100u)  % 10u));
  s[index + 2u] = (char)('0' + ((value / 10u)   % 10u));
  s[index + 3u] = (char)('0' + ( value          % 10u));
}

static void PutS4(char *s, unsigned int index, int value)
{
  unsigned int mag;

  if(value < 0)
  {
    s[index] = '-';
    mag = (unsigned int)(-value);
  }
  else
  {
    s[index] = '+';
    mag = (unsigned int)value;
  }

  if(mag > 999u)
  {
    mag = 999u;
  }

  s[index + 1u] = (char)('0' + ((mag / 100u) % 10u));
  s[index + 2u] = (char)('0' + ((mag / 10u)  % 10u));
  s[index + 3u] = (char)('0' + ( mag         % 10u));
}

static void RefreshDetectDisplay(void)
{
  // Row 0: title
  PutString10(line0, "DETECT");

  // Row 1: raw values
  // "L1234R5678"
  PutString10(line1, "L0000R0000");
  PutU4(line1, 1u, detect_left_raw);
  PutU4(line1, 6u, detect_right_raw);

  // Row 2: normalized levels
  // "A1234B5678"
  PutString10(line2, "A0000B0000");
  PutU4(line2, 1u, detect_left_level);
  PutU4(line2, 6u, detect_right_level);

  // Row 3: flags + position
  // "L1R0+123  "
  PutString10(line3, "L0R0+000");
  PutChar(line3, 1u, detect_left_on_line  ? '1' : '0');
  PutChar(line3, 3u, detect_right_on_line ? '1' : '0');
  PutS4(line3, 4u, detect_line_position);

  Display_Text(0u, line0);
  Display_Text(1u, line1);
  Display_Text(2u, line2);
  Display_Text(3u, line3);
  UpdateDisplay();
}

//---------------------------------------------------------------------------
// Public functions
//---------------------------------------------------------------------------

void DetectDemo_Init(void)
{
  PutString10(line0, "DETECT");
  PutString10(line1, " ");
  PutString10(line2, " ");
  PutString10(line3, " ");

  Display_Text(0u, line0);
  Display_Text(1u, line1);
  Display_Text(2u, line2);
  Display_Text(3u, line3);
  UpdateDisplay();

  ChangeBacklight(1);
}

void DetectDemo_Task(void)
{
  Detect_Task();
  RefreshDetectDisplay();
}
*/
