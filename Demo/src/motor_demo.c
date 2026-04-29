//==============================================================================
// File Name : motor_demo.c
//
// Description:
//   Motor demo plus stress test.
//
// Sequence:
//   1) Both motors forward
//   2) Both motors reverse
//   3) Left motor ramps up
//   4) Left motor off
//   5) Right motor ramps up
//   6) Stress: fast forward sweep
//   7) Stress: fast correction pulsing
//   8) Stress: fast direction flipping
//   9) Done
//
// Notes:
//   - Uses the public motor globals from motor.h
//   - Uses the new display API
//==============================================================================

#include <msp430.h>

#include "Core\\lib\\display.h"
#include "Core\\lib\\motor.h"
#include "Demo\\lib\\motor_demo.h"

extern volatile unsigned long system_ticks_ms;

//==============================================================================
// PRIVATE DEMO STATES
//==============================================================================

#define DEMO_STATE_BOTH_FORWARD        (0u)
#define DEMO_STATE_BOTH_REVERSE        (1u)
#define DEMO_STATE_LEFT_RAMP           (2u)
#define DEMO_STATE_LEFT_OFF            (3u)
#define DEMO_STATE_RIGHT_RAMP          (4u)
#define DEMO_STATE_STRESS_SWEEP        (5u)
#define DEMO_STATE_STRESS_PULSE        (6u)
#define DEMO_STATE_STRESS_FLIP         (7u)
#define DEMO_STATE_DONE                (8u)

//==============================================================================
// PRIVATE STATE
//==============================================================================

static unsigned char motor_demo_state;
static unsigned long motor_demo_state_time;
static unsigned long motor_demo_step_time;

static unsigned int stress_sweep_left_speed;
static unsigned int stress_sweep_right_speed;
static signed char stress_sweep_dir;

static unsigned char stress_pulse_phase;
static unsigned char stress_flip_phase;

//==============================================================================
// PRIVATE PROTOTYPES
//==============================================================================

static void MotorDemo_SetState(unsigned char new_state);
static void MotorDemo_UpdateDisplay(void);
static void BuildMotorLine(char *line, char motor_name, signed char state, unsigned int speed);
static void BuildTextLine(char *line, const char *text);
static char MotorStateToChar(signed char state);

//==============================================================================
// MotorDemo_Init
//==============================================================================

void MotorDemo_Init(void)
{
  Motor_Init();
  MotorDemo_SetState(DEMO_STATE_BOTH_FORWARD);
}

//==============================================================================
// MotorDemo_Task
//==============================================================================

void MotorDemo_Task(void)
{
  MotorDemo_UpdateDisplay();

  switch(motor_demo_state)
  {
    case DEMO_STATE_BOTH_FORWARD:
      if((system_ticks_ms - motor_demo_state_time) >= MOTOR_DEMO_BOTH_HOLD_MS)
      {
        MotorDemo_SetState(DEMO_STATE_BOTH_REVERSE);
      }
      break;

    case DEMO_STATE_BOTH_REVERSE:
      if((system_ticks_ms - motor_demo_state_time) >= MOTOR_DEMO_BOTH_HOLD_MS)
      {
        MotorDemo_SetState(DEMO_STATE_LEFT_RAMP);
      }
      break;

    case DEMO_STATE_LEFT_RAMP:
      if((system_ticks_ms - motor_demo_step_time) >= MOTOR_DEMO_RAMP_STEP_MS)
      {
        motor_demo_step_time = system_ticks_ms;

        if(motor_left_speed < 100u)
        {
          motor_left_speed += MOTOR_DEMO_RAMP_STEP;

          if(motor_left_speed > 100u)
          {
            motor_left_speed = 100u;
          }
        }
        else
        {
          MotorDemo_SetState(DEMO_STATE_LEFT_OFF);
        }
      }
      break;

    case DEMO_STATE_LEFT_OFF:
      if((system_ticks_ms - motor_demo_state_time) >= MOTOR_DEMO_LEFT_OFF_HOLD_MS)
      {
        MotorDemo_SetState(DEMO_STATE_RIGHT_RAMP);
      }
      break;

    case DEMO_STATE_RIGHT_RAMP:
      if((system_ticks_ms - motor_demo_step_time) >= MOTOR_DEMO_RAMP_STEP_MS)
      {
        motor_demo_step_time = system_ticks_ms;

        if(motor_right_speed < 100u)
        {
          motor_right_speed += MOTOR_DEMO_RAMP_STEP;

          if(motor_right_speed > 100u)
          {
            motor_right_speed = 100u;
          }
        }
        else
        {
          MotorDemo_SetState(DEMO_STATE_STRESS_SWEEP);
        }
      }
      break;

    case DEMO_STATE_STRESS_SWEEP:
      if((system_ticks_ms - motor_demo_step_time) >= MOTOR_DEMO_STRESS_SWEEP_MS)
      {
        motor_demo_step_time = system_ticks_ms;

        if(stress_sweep_dir > 0)
        {
          if(stress_sweep_left_speed < MOTOR_DEMO_STRESS_MAX_SPEED)
          {
            stress_sweep_left_speed += MOTOR_DEMO_STRESS_STEP;
            if(stress_sweep_left_speed > MOTOR_DEMO_STRESS_MAX_SPEED)
            {
              stress_sweep_left_speed = MOTOR_DEMO_STRESS_MAX_SPEED;
            }
          }

          if(stress_sweep_right_speed > MOTOR_DEMO_STRESS_MIN_SPEED)
          {
            if(stress_sweep_right_speed > MOTOR_DEMO_STRESS_STEP)
            {
              stress_sweep_right_speed -= MOTOR_DEMO_STRESS_STEP;
            }
            else
            {
              stress_sweep_right_speed = MOTOR_DEMO_STRESS_MIN_SPEED;
            }

            if(stress_sweep_right_speed < MOTOR_DEMO_STRESS_MIN_SPEED)
            {
              stress_sweep_right_speed = MOTOR_DEMO_STRESS_MIN_SPEED;
            }
          }

          if(stress_sweep_left_speed >= MOTOR_DEMO_STRESS_MAX_SPEED)
          {
            stress_sweep_dir = -1;
          }
        }
        else
        {
          if(stress_sweep_left_speed > MOTOR_DEMO_STRESS_MIN_SPEED)
          {
            if(stress_sweep_left_speed > MOTOR_DEMO_STRESS_STEP)
            {
              stress_sweep_left_speed -= MOTOR_DEMO_STRESS_STEP;
            }
            else
            {
              stress_sweep_left_speed = MOTOR_DEMO_STRESS_MIN_SPEED;
            }

            if(stress_sweep_left_speed < MOTOR_DEMO_STRESS_MIN_SPEED)
            {
              stress_sweep_left_speed = MOTOR_DEMO_STRESS_MIN_SPEED;
            }
          }

          if(stress_sweep_right_speed < MOTOR_DEMO_STRESS_MAX_SPEED)
          {
            stress_sweep_right_speed += MOTOR_DEMO_STRESS_STEP;
            if(stress_sweep_right_speed > MOTOR_DEMO_STRESS_MAX_SPEED)
            {
              stress_sweep_right_speed = MOTOR_DEMO_STRESS_MAX_SPEED;
            }
          }

          if(stress_sweep_right_speed >= MOTOR_DEMO_STRESS_MAX_SPEED)
          {
            stress_sweep_dir = 1;
          }
        }

        motor_left_state = motor_forward;
        motor_right_state = motor_forward;
        motor_left_speed = stress_sweep_left_speed;
        motor_right_speed = stress_sweep_right_speed;
      }

      if((system_ticks_ms - motor_demo_state_time) >= MOTOR_DEMO_STRESS_SWEEP_TIME)
      {
        MotorDemo_SetState(DEMO_STATE_STRESS_PULSE);
      }
      break;

    case DEMO_STATE_STRESS_PULSE:
      if((system_ticks_ms - motor_demo_step_time) >= MOTOR_DEMO_STRESS_PULSE_MS)
      {
        motor_demo_step_time = system_ticks_ms;

        if(stress_pulse_phase == 0u)
        {
          motor_left_state = motor_forward;
          motor_right_state = motor_forward;
          motor_left_speed = MOTOR_DEMO_STRESS_HIGH_SPEED;
          motor_right_speed = MOTOR_DEMO_STRESS_LOW_SPEED;
          stress_pulse_phase = 1u;
        }
        else
        {
          motor_left_state = motor_forward;
          motor_right_state = motor_forward;
          motor_left_speed = MOTOR_DEMO_STRESS_LOW_SPEED;
          motor_right_speed = MOTOR_DEMO_STRESS_HIGH_SPEED;
          stress_pulse_phase = 0u;
        }
      }

      if((system_ticks_ms - motor_demo_state_time) >= MOTOR_DEMO_STRESS_PULSE_TIME)
      {
        MotorDemo_SetState(DEMO_STATE_STRESS_FLIP);
      }
      break;

    case DEMO_STATE_STRESS_FLIP:
      if((system_ticks_ms - motor_demo_step_time) >= MOTOR_DEMO_STRESS_FLIP_MS)
      {
        motor_demo_step_time = system_ticks_ms;

        if(stress_flip_phase == 0u)
        {
          motor_left_state = motor_forward;
          motor_right_state = motor_forward;
          motor_left_speed = 70u;
          motor_right_speed = 70u;
          stress_flip_phase = 1u;
        }
        else
        {
          motor_left_state = motor_reverse;
          motor_right_state = motor_reverse;
          motor_left_speed = 70u;
          motor_right_speed = 70u;
          stress_flip_phase = 0u;
        }
      }

      if((system_ticks_ms - motor_demo_state_time) >= MOTOR_DEMO_STRESS_FLIP_TIME)
      {
        MotorDemo_SetState(DEMO_STATE_DONE);
      }
      break;

    case DEMO_STATE_DONE:
    default:
      break;
  }
}

//==============================================================================
// MotorDemo_SetState
//==============================================================================

static void MotorDemo_SetState(unsigned char new_state)
{
  motor_demo_state = new_state;
  motor_demo_state_time = system_ticks_ms;
  motor_demo_step_time = system_ticks_ms;

  switch(new_state)
  {
    case DEMO_STATE_BOTH_FORWARD:
      motor_left_state = motor_forward;
      motor_right_state = motor_forward;
      motor_left_speed = MOTOR_DEMO_BOTH_SPEED;
      motor_right_speed = MOTOR_DEMO_BOTH_SPEED;
      break;

    case DEMO_STATE_BOTH_REVERSE:
      motor_left_state = motor_reverse;
      motor_right_state = motor_reverse;
      motor_left_speed = MOTOR_DEMO_BOTH_SPEED;
      motor_right_speed = MOTOR_DEMO_BOTH_SPEED;
      break;

    case DEMO_STATE_LEFT_RAMP:
      motor_left_state = motor_forward;
      motor_right_state = motor_off;
      motor_left_speed = MOTOR_DEMO_RAMP_START_SPEED;
      motor_right_speed = MOTOR_MIN_PWM;
      break;

    case DEMO_STATE_LEFT_OFF:
      motor_left_state = motor_off;
      motor_right_state = motor_off;
      motor_left_speed = MOTOR_MIN_PWM;
      motor_right_speed = MOTOR_MIN_PWM;
      break;

    case DEMO_STATE_RIGHT_RAMP:
      motor_left_state = motor_off;
      motor_right_state = motor_forward;
      motor_left_speed = MOTOR_MIN_PWM;
      motor_right_speed = MOTOR_DEMO_RAMP_START_SPEED;
      break;

    case DEMO_STATE_STRESS_SWEEP:
      stress_sweep_left_speed = MOTOR_DEMO_STRESS_MIN_SPEED;
      stress_sweep_right_speed = MOTOR_DEMO_STRESS_MAX_SPEED;
      stress_sweep_dir = 1;

      motor_left_state = motor_forward;
      motor_right_state = motor_forward;
      motor_left_speed = stress_sweep_left_speed;
      motor_right_speed = stress_sweep_right_speed;
      break;

    case DEMO_STATE_STRESS_PULSE:
      stress_pulse_phase = 0u;

      motor_left_state = motor_forward;
      motor_right_state = motor_forward;
      motor_left_speed = MOTOR_DEMO_STRESS_HIGH_SPEED;
      motor_right_speed = MOTOR_DEMO_STRESS_LOW_SPEED;
      break;

    case DEMO_STATE_STRESS_FLIP:
      stress_flip_phase = 0u;

      motor_left_state = motor_forward;
      motor_right_state = motor_forward;
      motor_left_speed = 70u;
      motor_right_speed = 70u;
      break;

    case DEMO_STATE_DONE:
    default:
      motor_left_state = motor_off;
      motor_right_state = motor_off;
      motor_left_speed = MOTOR_MIN_PWM;
      motor_right_speed = MOTOR_MIN_PWM;
      break;
  }
}

//==============================================================================
// MotorDemo_UpdateDisplay
//==============================================================================

static void MotorDemo_UpdateDisplay(void)
{
  char line0[11];
  char line1[11];
  char line2[11];
  char line3[11];

  BuildMotorLine(line0, 'L', motor_left_state, motor_left_speed);
  BuildMotorLine(line1, 'R', motor_right_state, motor_right_speed);

  switch(motor_demo_state)
  {
    case DEMO_STATE_BOTH_FORWARD:
      BuildTextLine(line2, "BOTH FWD");
      break;

    case DEMO_STATE_BOTH_REVERSE:
      BuildTextLine(line2, "BOTH REV");
      break;

    case DEMO_STATE_LEFT_RAMP:
      BuildTextLine(line2, "LEFT RAMP");
      break;

    case DEMO_STATE_LEFT_OFF:
      BuildTextLine(line2, "LEFT OFF");
      break;

    case DEMO_STATE_RIGHT_RAMP:
      BuildTextLine(line2, "RGHT RAMP");
      break;

    case DEMO_STATE_STRESS_SWEEP:
      BuildTextLine(line2, "ST SWEEP");
      break;

    case DEMO_STATE_STRESS_PULSE:
      BuildTextLine(line2, "ST PULSE");
      break;

    case DEMO_STATE_STRESS_FLIP:
      BuildTextLine(line2, "ST FLIP");
      break;

    case DEMO_STATE_DONE:
    default:
      BuildTextLine(line2, "DONE");
      break;
  }

  BuildTextLine(line3, "EXT VARS");

  Display_Text(0, line0);
  Display_Text(1, line1);
  Display_Text(2, line2);
  Display_Text(3, line3);
  UpdateDisplay();
}

//==============================================================================
// BuildMotorLine
//==============================================================================

static void BuildMotorLine(char *line, char motor_name, signed char state, unsigned int speed)
{
  line[0] = motor_name;
  line[1] = ' ';
  line[2] = MotorStateToChar(state);
  line[3] = ' ';
  line[4] = (char)('0' + ((speed / 100u) % 10u));
  line[5] = (char)('0' + ((speed / 10u) % 10u));
  line[6] = (char)('0' + (speed % 10u));
  line[7] = ' ';
  line[8] = ' ';
  line[9] = ' ';
  line[10] = 0;
}

//==============================================================================
// BuildTextLine
//==============================================================================

static void BuildTextLine(char *line, const char *text)
{
  unsigned int i;

  for(i = 0u; i < 10u; i++)
  {
    if(text[i] != 0)
    {
      line[i] = text[i];
    }
    else
    {
      break;
    }
  }

  while(i < 10u)
  {
    line[i] = ' ';
    i++;
  }

  line[10] = 0;
}

//==============================================================================
// MotorStateToChar
//==============================================================================

static char MotorStateToChar(signed char state)
{
  if(state == motor_forward)
  {
    return 'F';
  }

  if(state == motor_reverse)
  {
    return 'R';
  }

  return '0';
}
