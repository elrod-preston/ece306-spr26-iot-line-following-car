//===========================================================================
// File Name : display.c
//
// Description:
// Simple LCD display control for 4 rows x 10 chars.
//
// Notes:
//  - Each row is always exactly 10 visible chars + null terminator
//  - Call Display_Process() in the main loop
//  - Call Display_Text(row, text) to change one line
//  - Call UpdateDisplay() to request LCD refresh
//===========================================================================

#include <Core\lib\macros.h>
#include "Core\lib\functions.h"
#include "msp430.h"
#include "Core\lib\LCD.h"
#include "Core\lib\ports.h"
#include "Core\lib\display.h"

extern char display_line[4][11];
extern char *display[4];
extern volatile unsigned char display_changed;
extern volatile unsigned char update_display;




//-----------------------------------------------------------------------------
// Push display buffer to LCD when requested
//-----------------------------------------------------------------------------
void Display_Process(void)
{
  if(update_display)
  {
    update_display = 0;

    if(display_changed)
    {
      display_changed = 0;
      Display_Update(0,0,0,0);
    }
  }
}

//-----------------------------------------------------------------------------
// Initialize display buffers and LCD row pointers
//-----------------------------------------------------------------------------
void Init_Display_Conditions(void)
{
  unsigned int row;
  unsigned int col;

  for(row = 0; row < 4; row++)
  {
    for(col = 0; col < 10; col++)
    {
      display_line[row][col] = ' ';
    }
    display_line[row][10] = 0;
  }

  display[0] = display_line[0];
  display[1] = display_line[1];
  display[2] = display_line[2];
  display[3] = display_line[3];

  Display_Text(1, "Init Core");
  Display_Text(2, "Success");

  UpdateDisplay();
}

//-----------------------------------------------------------------------------
// Update one row with plain text
// row = 0 to 3
//-----------------------------------------------------------------------------
void Display_Text(unsigned int row, char *text)
{
  unsigned int i;

  if(row >= 4)
  {
    return;
  }

  for(i = 0; i < 10; i++)
  {
    display_line[row][i] = ' ';
  }

  i = 0;
  while((i < 10) && (text[i] != 0))
  {
    display_line[row][i] = text[i];
    i++;
  }

  display_line[row][10] = 0;
  display_changed = 1;
}

//-----------------------------------------------------------------------------
// Blank entire LCD buffer
// Fills all 4 rows with spaces and requests refresh
//-----------------------------------------------------------------------------
void Blank_LCD(void)
{
  unsigned int row;
  unsigned int col;

  for(row = 0; row < 4; row++)
  {
    for(col = 0; col < 10; col++)
    {
      display_line[row][col] = ' ';
    }
    display_line[row][10] = 0;
  }

  display_changed = 1;
  UpdateDisplay();
}

//-----------------------------------------------------------------------------
// Request LCD refresh
//-----------------------------------------------------------------------------
void UpdateDisplay(void)
{
  update_display = 1;
}

//-----------------------------------------------------------------------------
// Backlight control
//-----------------------------------------------------------------------------
void ChangeBacklight(char mode)
{
  if(mode)
  {
    P6OUT |= LCD_BACKLITE;
  }
  else
  {
    P6OUT &= ~LCD_BACKLITE;
  }
}
