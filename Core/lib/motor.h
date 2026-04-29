//==============================================================================
// File Name : motor.h
//
// Description:
//   Simple polled motor controller interface.
//
// Usage:
//   Motor_SetWheels(motor_forward, 40u, motor_forward, 40u); // forward
//   Motor_SetWheels(motor_reverse, 35u, motor_reverse, 35u); // reverse
//   Motor_SetWheels(motor_reverse, 50u, motor_forward, 50u); // tank left
//   Motor_SetWheels(motor_forward, 50u, motor_reverse, 50u); // tank right
//   Motor_Stop();                                            // stop
//==============================================================================

#ifndef MOTOR_H
#define MOTOR_H

//==============================================================================
// MOTOR STATE VALUES
//==============================================================================

#define motor_reverse              (-1)
#define motor_off                  (0)
#define motor_forward              (1)

//==============================================================================
// DEFAULT MOTOR SETTINGS
//==============================================================================

#define MOTOR_MIN_PWM              (25u)

// negative = reduce LEFT motor
// positive = reduce RIGHT motor
#define MOTOR_PWM_OFFSET           (-6)

#define WHEEL_PERIOD               (5000u)

//==============================================================================
// PUBLIC GLOBALS
//==============================================================================

extern volatile signed char motor_left_state;
extern volatile signed char motor_right_state;

extern volatile unsigned int motor_left_speed;
extern volatile unsigned int motor_right_speed;

extern volatile unsigned int motor_min_pwm;
extern volatile signed int motor_pwm_offset;

//==============================================================================
// PUBLIC FUNCTIONS
//==============================================================================

void Motor_Init(void);
void Motor_Task(void);

void Motor_SetWheels(signed char left_state,
                     unsigned int left_speed,
                     signed char right_state,
                     unsigned int right_speed);

void Motor_Stop(void);

#endif
