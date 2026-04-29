//===========================================================================
// File Name : init.c
//
// Description: This file inits all components on the board
//
// Author: Preston Elrod
// Date: Feb 2026
// Compiler: Built with Code Composer Version: CCS12.4.0.00007_win64
//===========================================================================

#include  "Core\core.h"

void Init_All(void) {

  Init_Ports();                 // GPIO + peripheral pin mux first
  Init_Clocks();                // ACLK/SMCLK/MCLK stable
 // Reset_LCD();
  Init_ADC();
  Init_Timer_B0();              // Now start 200ms update_display timer
  Init_Timer_B3();
  Motor_Init();
  Init_Serial_UCA0(BAUD_115200);
  Init_Serial_UCA1(BAUD_115200);
  Init_LCD();                   // LCD + SPI must be ready BEFORE any display updates
  Init_Display_Conditions();    // Initialize display_line[] contents + display_changed
  enable_interrupts();          // Enable port interrupts (SW1/SW2 etc.)
  __enable_interrupt();    // GIE last
  //end of system inits, user code inits:
  //Calibrate_Init(); // attempt to load calibration data from the stored fram, if invalid or user requests calibration, calibration seq
}
