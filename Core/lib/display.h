#ifndef DISPLAY_H
#define DISPLAY_H


void Blank_LCD(void);

//---------------------------------------------------------------------------
// Reset_LCD
//
// Purpose:
//   Performs a hardware reset of the LCD module.
//
// When to use:
//   Call this during startup if the LCD needs a clean hardware reset before
//   normal operation begins.
//
// Notes for prompt usage:
//   Use this when you want to reinitialize the physical LCD hardware, not
//   just clear or change displayed text.
//---------------------------------------------------------------------------
void Reset_LCD(void);

//---------------------------------------------------------------------------
// Display_Process
//
// Purpose:
//   Handles pending LCD updates and pushes changed display data to the screen.
//
// When to use:
//   Call this repeatedly inside the main loop so the LCD can refresh whenever
//   update_display and display_changed are set.
//
// Notes for prompt usage:
//   Use this as the main background display service function in the program
//   loop. This should not normally be called from an interrupt.
//---------------------------------------------------------------------------
void Display_Process(void);

//---------------------------------------------------------------------------
// Init_Display_Conditions
//
// Purpose:
//   Initializes the display buffers, row pointers, and startup text/state.
//
// When to use:
//   Call once during system startup before using Display_Text() or
//   Display_Process().
//
// Notes for prompt usage:
//   Use this when setting up the LCD system at boot. This is the place to
//   define default startup messages such as initialization status text.
//---------------------------------------------------------------------------
void Init_Display_Conditions(void);

//---------------------------------------------------------------------------
// Display_Text
//
// Purpose:
//   Writes plain text to one display row.
//
// Parameters:
//   row  - LCD row index (typically 0 to 3)
//   text - Text to place on that row
//
// When to use:
//   Call whenever you want to change the contents of a single LCD line.
//
// Notes for prompt usage:
//   Use this for simple row-based text output. Best for labels, status
//   messages, and short debug text. Text is expected to fit the display width.
//---------------------------------------------------------------------------
void Display_Text(unsigned int row, char *text);

//---------------------------------------------------------------------------
// UpdateDisplay
//
// Purpose:
//   Requests that the LCD refresh on the next Display_Process() call.
//
// When to use:
//   Call this after changing display content if your design uses a manual
//   update request model.
//
// Notes for prompt usage:
//   Use this after one or more Display_Text() calls when you want the new
//   buffer contents to appear on the screen.
//---------------------------------------------------------------------------
void UpdateDisplay(void);

//---------------------------------------------------------------------------
// ChangeBacklight
//
// Purpose:
//   Turns the LCD backlight on or off.
//
// Parameters:
//   mode - Nonzero = on, zero = off
//
// When to use:
//   Call when you want to control LCD visibility or provide visual feedback.
//
// Notes for prompt usage:
//   Use this for basic backlight control during startup, alerts, button
//   feedback, or power-saving behavior.
//---------------------------------------------------------------------------
void ChangeBacklight(char mode);

#endif
