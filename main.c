//===========================================================================
// File Name : main.c
//
// Description:
//   Simplified main loop.
//
// State order:
//   1 = Init Core
//   2 = DAC Ramp
//   3 = IOT Setup
//   4 = Calibrate Sensor
//   5 = Command Coordinator
//
// Notes:
//   - main.c does NOT process IOT commands.
//   - command_coordinator.c owns command parsing and command execution.
//   - IOT_Task() only manages Wi-Fi, auth, and latest command receive.
//   - Detect_Task() runs during calibration and command coordinator stage.
//===========================================================================

#include <msp430.h>

#include "Core\\core.h"
#include "Addon\\lib\\calibrate.h"
#include "Addon\\lib\\detect.h"
#include "Addon\\lib\\command.h"
#include "Addon\\lib\\led.h"


extern volatile unsigned char sw1_pressed;
extern volatile unsigned char sw2_pressed;

//---------------------------------------------------------------------------
// States
//---------------------------------------------------------------------------

#define STATE_INIT_CORE             (1u)
#define STATE_DAC_RAMP              (2u)
#define STATE_IOT_SETUP             (3u)
#define STATE_CALIBRATE_SENSOR      (4u)
#define STATE_COMMAND_COORDINATOR   (5u)

//---------------------------------------------------------------------------
// State globals
//---------------------------------------------------------------------------

static unsigned char state          = STATE_INIT_CORE;
static unsigned char previous_state = 0u;
static unsigned char state_entry    = 0u;

//===========================================================================
// Main
//===========================================================================

void main(void)
{
  WDTCTL = WDTPW | WDTHOLD;

  while (1)
  {
    //-----------------------------------------------------------------------
    // Detect state entry
    //-----------------------------------------------------------------------

    if (state != previous_state)
    {
      previous_state = state;
      state_entry = 1u;
    }
    else
    {
      state_entry = 0u;
    }

    //-----------------------------------------------------------------------
    // Main state machine
    //-----------------------------------------------------------------------

    switch (state)
    {
      //---------------------------------------------------------------------
      // State 1: Init Core
      //---------------------------------------------------------------------

      case STATE_INIT_CORE:
        if (state_entry)
        {
          unsigned int bat_mv;
          char bat_line[11];

          Init_All();
          Motor_Init();

          sw1_pressed = 0u;
          sw2_pressed = 0u;

          Blank_LCD();

          bat_mv = ADC_ReadMilliVolts((unsigned)ADC_CH_BAT);

          // Format: BAT 12.34V
          bat_line[0]  = 'B';
          bat_line[1]  = 'A';
          bat_line[2]  = 'T';
          bat_line[3]  = ' ';
          bat_line[4]  = (char)(((bat_mv / 10000u) % 10u) + '0');
          bat_line[5]  = (char)(((bat_mv / 1000u)  % 10u) + '0');
          bat_line[6]  = '.';
          bat_line[7]  = (char)(((bat_mv / 100u)   % 10u) + '0');
          bat_line[8]  = (char)(((bat_mv / 10u)    % 10u) + '0');
          bat_line[9]  = 'V';
          bat_line[10] = 0;

          Display_Text(0, bat_line);
          Display_Text(1, "PRESS SW");
          Display_Process();
        }

        Display_Process();

        if ((sw1_pressed != 0u) || (sw2_pressed != 0u))
        {
          sw1_pressed = 0u;
          sw2_pressed = 0u;
          state++;
        }
        break;

      //---------------------------------------------------------------------
      // State 2: DAC Ramp
      //---------------------------------------------------------------------

      case STATE_DAC_RAMP:
        if (state_entry)
        {
          Blank_LCD();
          Init_DAC();
        }

        Display_Process();

        if ((TB0CTL & TBIE) == 0u)
        {
          state++;
        }
        break;

      //---------------------------------------------------------------------
      // State 3: IOT Setup
      //---------------------------------------------------------------------

      case STATE_IOT_SETUP:
        if (state_entry)
        {
          Blank_LCD();
          IOT_Init();
        }

        IOT_Task();
        Display_Process();

        if (IOT_Authorized() != 0u)
        {
          state++;
        }
        break;

      //---------------------------------------------------------------------
      // State 4: Calibrate Sensor
      //---------------------------------------------------------------------

      case STATE_CALIBRATE_SENSOR:
        if (state_entry)
        {
          Blank_LCD();
          Detect_Init();
          Calibrate_Init();
        }

        IOT_Task();
        Detect_Task();
        Calibrate_Task();
        Display_Process();

        if (Calibrate_IsDone() != 0u)
        {
          state++;
        }
        break;

      //---------------------------------------------------------------------
      // State 5: Command Coordinator
      //---------------------------------------------------------------------

      case STATE_COMMAND_COORDINATOR:
        if (state_entry)
        {
          Blank_LCD();

          Detect_Init();
          CommandCoordinator_Init();
          Motor_Task();
        }

        IOT_Task();
        Detect_Task();
        CommandCoordinator_Task();
        Motor_Task();
        Display_Process();
        //Strobe_Task();
        break;

      //---------------------------------------------------------------------
      // Default recovery
      //---------------------------------------------------------------------

      default:
        state = STATE_INIT_CORE;
        break;
    }
  }
}
