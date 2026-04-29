//==============================================================================
// File Name : motor.c
//
// Description:
//   Simple polled motor controller.
//
// Behavior:
//   - Other code calls Motor_SetWheels()
//   - Motor_Task() applies the requested state/speed to the PWM hardware
//   - Runtime minimum PWM is applied when a motor is active
//   - Runtime one-sided offset is applied
//
// Notes:
//   - No PID here
//   - No line-follow logic here
//   - No extra movement wrappers here
//==============================================================================

#include <msp430.h>
#include "Core\\lib\\motor.h"

//==============================================================================
// HARDWARE PWM REGISTER MAPPING
//==============================================================================

#define PWM_PERIOD                 (TB3CCR0)

#define RIGHT_FORWARD_SPEED        (TB3CCR2)
#define LEFT_FORWARD_SPEED         (TB3CCR3)
#define RIGHT_REVERSE_SPEED        (TB3CCR4)
#define LEFT_REVERSE_SPEED         (TB3CCR5)

// WHEEL_PERIOD = 5000, so 1 percent = 50 timer counts.
#define DUTY_PER_PERCENT           (WHEEL_PERIOD / 100u)

//==============================================================================
// PUBLIC GLOBALS
//==============================================================================

volatile signed char motor_left_state = motor_off;
volatile signed char motor_right_state = motor_off;

volatile unsigned int motor_left_speed = MOTOR_MIN_PWM;
volatile unsigned int motor_right_speed = MOTOR_MIN_PWM;

volatile unsigned int motor_min_pwm = MOTOR_MIN_PWM;
volatile signed int motor_pwm_offset = MOTOR_PWM_OFFSET;

//==============================================================================
// Motor_SetWheels
//==============================================================================

void Motor_SetWheels(signed char left_state,
                     unsigned int left_speed,
                     signed char right_state,
                     unsigned int right_speed)
{
  motor_left_state = left_state;
  motor_left_speed = left_speed;

  motor_right_state = right_state;
  motor_right_speed = right_speed;
}

//==============================================================================
// Motor_Stop
//==============================================================================

void Motor_Stop(void)
{
  Motor_SetWheels(motor_off, 0u,
                  motor_off, 0u);
}

//==============================================================================
// Motor_Init
//==============================================================================

void Motor_Init(void)
{
  TB3CTL = TBSSEL__SMCLK;
  TB3CTL |= MC__UP;
  TB3CTL |= TBCLR;

  PWM_PERIOD = WHEEL_PERIOD;

  TB3CCTL2 = OUTMOD_7;
  TB3CCTL3 = OUTMOD_7;
  TB3CCTL4 = OUTMOD_7;
  TB3CCTL5 = OUTMOD_7;

  motor_min_pwm = MOTOR_MIN_PWM;
  motor_pwm_offset = MOTOR_PWM_OFFSET;

  Motor_Stop();

  LEFT_FORWARD_SPEED = 0u;
  LEFT_REVERSE_SPEED = 0u;
  RIGHT_FORWARD_SPEED = 0u;
  RIGHT_REVERSE_SPEED = 0u;
}

//==============================================================================
// Motor_Task
//==============================================================================

void Motor_Task(void)
{
  signed int left_speed;
  signed int right_speed;

  unsigned int left_duty;
  unsigned int right_duty;

  //--------------------------------------------------------------------------
  // Start with requested speeds.
  //--------------------------------------------------------------------------

  left_speed = (signed int)motor_left_speed;
  right_speed = (signed int)motor_right_speed;

  //--------------------------------------------------------------------------
  // If motor is off, force speed to zero.
  //--------------------------------------------------------------------------

  if(motor_left_state == motor_off)
  {
    left_speed = 0;
  }

  if(motor_right_state == motor_off)
  {
    right_speed = 0;
  }

  //--------------------------------------------------------------------------
  // Apply minimum PWM only when motor is active.
  //--------------------------------------------------------------------------

  if((motor_left_state != motor_off) &&
     (left_speed < (signed int)motor_min_pwm))
  {
    left_speed = (signed int)motor_min_pwm;
  }

  if((motor_right_state != motor_off) &&
     (right_speed < (signed int)motor_min_pwm))
  {
    right_speed = (signed int)motor_min_pwm;
  }

  //--------------------------------------------------------------------------
  // Apply one-sided offset.
  //
  // negative = reduce LEFT motor
  // positive = reduce RIGHT motor
  //--------------------------------------------------------------------------

  if((motor_pwm_offset < 0) &&
     (motor_left_state != motor_off))
  {
    left_speed += motor_pwm_offset;
  }

  if((motor_pwm_offset > 0) &&
     (motor_right_state != motor_off))
  {
    right_speed -= motor_pwm_offset;
  }

  //--------------------------------------------------------------------------
  // Prevent invalid hardware duty.
  //--------------------------------------------------------------------------

  if(left_speed < 0)
  {
    left_speed = 0;
  }

  if(right_speed < 0)
  {
    right_speed = 0;
  }

  if(left_speed > 100)
  {
    left_speed = 100;
  }

  if(right_speed > 100)
  {
    right_speed = 100;
  }

  //--------------------------------------------------------------------------
  // Convert percent to timer duty.
  //
  // duty = percent * (WHEEL_PERIOD / 100)
  //--------------------------------------------------------------------------

  left_duty = (unsigned int)left_speed * DUTY_PER_PERCENT;
  right_duty = (unsigned int)right_speed * DUTY_PER_PERCENT;

  //--------------------------------------------------------------------------
  // Apply left motor hardware.
  //--------------------------------------------------------------------------

  if(motor_left_state == motor_forward)
  {
    LEFT_FORWARD_SPEED = left_duty;
    LEFT_REVERSE_SPEED = 0u;
  }
  else if(motor_left_state == motor_reverse)
  {
    LEFT_FORWARD_SPEED = 0u;
    LEFT_REVERSE_SPEED = left_duty;
  }
  else
  {
    LEFT_FORWARD_SPEED = 0u;
    LEFT_REVERSE_SPEED = 0u;
  }

  //--------------------------------------------------------------------------
  // Apply right motor hardware.
  //--------------------------------------------------------------------------

  if(motor_right_state == motor_forward)
  {
    RIGHT_FORWARD_SPEED = right_duty;
    RIGHT_REVERSE_SPEED = 0u;
  }
  else if(motor_right_state == motor_reverse)
  {
    RIGHT_FORWARD_SPEED = 0u;
    RIGHT_REVERSE_SPEED = right_duty;
  }
  else
  {
    RIGHT_FORWARD_SPEED = 0u;
    RIGHT_REVERSE_SPEED = 0u;
  }
}
