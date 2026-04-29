//===========================================================================
// File Name : utils.c
//
// Description: Generic helper functions (simple / old compiler friendly)
//
// Author: Preston Elrod
// Date: Feb 2026
//===========================================================================

#include "Core\lib\utils.h"

#define TOSTR_WIDTH        (10)
#define FLOATSTR_MAX_WIDTH (16)

// internal static buffers
static char g_toStringBuf[TOSTR_WIDTH + 1];
static char g_floatStringBuf[FLOATSTR_MAX_WIDTH + 1];

// Make buffer = 10 chars wide, right-aligned decimal number, padded with spaces
// Returns pointer to internal static buffer (overwritten every call)
char* ToString(unsigned v)
{
  int i;
  unsigned temp;
  int digits;

  for(i = 0; i < TOSTR_WIDTH; i++) g_toStringBuf[i] = ' ';
  g_toStringBuf[TOSTR_WIDTH] = 0;

  if(v == 0u)
  {
    g_toStringBuf[TOSTR_WIDTH - 1] = '0';
    return g_toStringBuf;
  }

  temp = v;
  digits = 0;
  while(temp > 0u)
  {
    temp /= 10u;
    digits++;
  }

  if(digits > TOSTR_WIDTH)
  {
    for(i = 0; i < TOSTR_WIDTH; i++) g_toStringBuf[i] = '*';
    g_toStringBuf[TOSTR_WIDTH] = 0;
    return g_toStringBuf;
  }

  i = TOSTR_WIDTH - 1;
  while((v > 0u) && (i >= 0))
  {
    g_toStringBuf[i] = (char)('0' + (v % 10u));
    v /= 10u;
    i--;
  }

  return g_toStringBuf;
}

//---------------------------------------------------------------------------
// FloatToString
//
// Purpose:
//   Convert float to right-aligned text with rounding.
//
// Inputs:
//   v         = float value
//   width     = total output width
//   decimals  = digits after decimal point
//
// Behavior:
//   - right aligned
//   - padded with spaces on left
//   - rounded to requested decimals
//   - returns "*" fill on overflow
//   - uses static internal buffer
//
// Examples:
//   FloatToString(0.0f,    10, 3) -> "     0.000"
//   FloatToString(-0.347f, 10, 3) -> "    -0.347"
//   FloatToString(12.34f,   8, 2) -> "   12.34"
//---------------------------------------------------------------------------
char* FloatToString(float v, unsigned width, unsigned decimals)
{
  int i;
  int pos;
  int digit_count;
  int is_negative;
  unsigned long scale;
  unsigned long rounding;
  unsigned long scaled;
  unsigned long int_part;
  unsigned long frac_part;
  unsigned long temp;
  unsigned long div;

  if(width == 0u)
  {
    g_floatStringBuf[0] = 0;
    return g_floatStringBuf;
  }

  if(width > FLOATSTR_MAX_WIDTH)
  {
    width = FLOATSTR_MAX_WIDTH;
  }

  // limit decimals so scale math stays reasonable
  if(decimals > 6u)
  {
    decimals = 6u;
  }

  // fill output with spaces
  for(i = 0; i < (int)width; i++)
  {
    g_floatStringBuf[i] = ' ';
  }
  g_floatStringBuf[width] = 0;

  // sign
  is_negative = 0;
  if(v < 0.0f)
  {
    is_negative = 1;
    v = -v;
  }

  // scale = 10^decimals
  scale = 1u;
  for(i = 0; i < (int)decimals; i++)
  {
    scale *= 10u;
  }

  // rounded integer representation
  // example: 1.2345 with decimals=3 => 1235
  rounding = scale / 2u;
  scaled = (unsigned long)(v * (float)scale + 0.5f);

  int_part = scaled / scale;
  frac_part = scaled % scale;

  // count integer digits
  if(int_part == 0u)
  {
    digit_count = 1;
  }
  else
  {
    temp = int_part;
    digit_count = 0;
    while(temp > 0u)
    {
      temp /= 10u;
      digit_count++;
    }
  }

  // required characters
  // sign + integer digits + optional '.' + decimals
  {
    unsigned required;

    required = (unsigned)digit_count;
    if(is_negative)
    {
      required++;
    }
    if(decimals > 0u)
    {
      required += 1u + decimals;
    }

    if(required > width)
    {
      for(i = 0; i < (int)width; i++)
      {
        g_floatStringBuf[i] = '*';
      }
      g_floatStringBuf[width] = 0;
      return g_floatStringBuf;
    }
  }

  // build from right to left
  pos = (int)width - 1;

  // fractional digits
  if(decimals > 0u)
  {
    div = 1u;
    for(i = 1; i < (int)decimals; i++)
    {
      div *= 10u;
    }

    for(i = 0; i < (int)decimals; i++)
    {
      g_floatStringBuf[pos] = (char)('0' + ((frac_part / div) % 10u));
      pos--;

      if(div > 1u)
      {
        div /= 10u;
      }
    }

    g_floatStringBuf[pos] = '.';
    pos--;
  }

  // integer digits
  if(int_part == 0u)
  {
    g_floatStringBuf[pos] = '0';
    pos--;
  }
  else
  {
    while((int_part > 0u) && (pos >= 0))
    {
      g_floatStringBuf[pos] = (char)('0' + (int_part % 10u));
      int_part /= 10u;
      pos--;
    }
  }

  // sign
  if(is_negative && (pos >= 0))
  {
    g_floatStringBuf[pos] = '-';
    pos--;
  }

  return g_floatStringBuf;
}
