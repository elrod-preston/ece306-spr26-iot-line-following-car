//==============================================================================
// File Name : motor_demo.h
//
// Description:
//   Motor demo and stress test.
//
// Notes:
//   - This demo directly changes the public motor globals from motor.h
//   - Motor_Task() must still be called from main
//   - Display_Process() must still be called from main
//==============================================================================

#ifndef MOTOR_DEMO_H
#define MOTOR_DEMO_H

//==============================================================================
// CONFIGURATION AREA
//==============================================================================

//------------------------------------------------------------------------------
// Basic demo settings
//------------------------------------------------------------------------------
#define MOTOR_DEMO_BOTH_SPEED          (40u)
#define MOTOR_DEMO_BOTH_HOLD_MS        (1500ul)

#define MOTOR_DEMO_RAMP_START_SPEED    (MOTOR_MIN_PWM)
#define MOTOR_DEMO_RAMP_STEP           (10u)
#define MOTOR_DEMO_RAMP_STEP_MS        (300ul)

#define MOTOR_DEMO_LEFT_OFF_HOLD_MS    (800ul)

//------------------------------------------------------------------------------
// Stress test settings
//------------------------------------------------------------------------------

// Fast sweep step time
#define MOTOR_DEMO_STRESS_SWEEP_MS     (20ul)

// Fast pulse step time
#define MOTOR_DEMO_STRESS_PULSE_MS     (20ul)

// Fast flip step time
#define MOTOR_DEMO_STRESS_FLIP_MS      (80ul)

// How long each stress section runs
#define MOTOR_DEMO_STRESS_SWEEP_TIME   (3000ul)
#define MOTOR_DEMO_STRESS_PULSE_TIME   (3000ul)
#define MOTOR_DEMO_STRESS_FLIP_TIME    (3000ul)

// Sweep limits
#define MOTOR_DEMO_STRESS_MIN_SPEED    (25u)
#define MOTOR_DEMO_STRESS_MAX_SPEED    (100u)
#define MOTOR_DEMO_STRESS_STEP         (10u)

// Pulse values for strong correction-style changes
#define MOTOR_DEMO_STRESS_LOW_SPEED    (25u)
#define MOTOR_DEMO_STRESS_HIGH_SPEED   (100u)

//------------------------------------------------------------------------------
// Public functions
//------------------------------------------------------------------------------
void MotorDemo_Init(void);
void MotorDemo_Task(void);

#endif
