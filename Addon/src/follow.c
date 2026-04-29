//===========================================================================
// File Name : follow.c
//
// Description:
//   Simple constant-speed line follower.
//
//   - No PID
//   - No variable PWM correction
//   - Only one base speed
//   - No display
//   - No ADC_ReadChannel()
//   - No Detect_Task()
//   - No detect_request
//   - No IR LED control
//   - Assumes detect and calibration values are already 10-bit
//   - Uses only Motor_SetWheels() and Motor_Stop()
//
// Behavior:
//   both sensors black  -> drive forward
//   left sensor black   -> turn left
//   right sensor black  -> turn right
//   both sensors white  -> tank search toward last known line side
//===========================================================================

#include <msp430.h>

#include "Core\lib\motor.h"

#include "Addon\lib\follow.h"
#include "Addon\lib\detect.h"
#include "Addon\lib\calibrate.h"

//===========================================================================
// External globals
//===========================================================================

extern volatile unsigned long system_ticks_ms;

//===========================================================================
// Tuning
//===========================================================================

#define FOLLOW_UPDATE_MS             (10u)

/*
 * Only speed used by this file.
 *
 * Keep this slow for tight circles.
 */
#define BASE_SPEED                   (30u)

/*
 * 10-bit calibrated signal threshold.
 *
 * Raise this if it thinks white is black.
 * Lower this if it loses the line too easily.
 */
#define BLACK_LEVEL                  (2)

/*
 * 0 = softer correction:
 *     one wheel forward, one wheel stopped
 *
 * 1 = sharper correction:
 *     one wheel forward, one wheel reverse
 *
 * For very tight circles, 1 works better.
 */
#define SHARP_SINGLE_SENSOR_TURN     (1u)

//===========================================================================
// Internal state
//===========================================================================

static unsigned long last_update_ms;

/*
 * -1 = last line was more on right sensor
 *  1 = last line was more on left sensor
 */
static signed char last_line_side;

//===========================================================================
// Sensor helper
//
// Assumes raw_adc, white_cal, and black_cal are already 10-bit.
//
// Return:
//   positive = black side of midpoint
//   zero     = near midpoint
//   negative = white side of midpoint
//===========================================================================

static signed int GetBlackSignal(unsigned int raw_adc,
                                 unsigned int white_cal,
                                 unsigned int black_cal)
{
    signed int raw;
    signed int white;
    signed int black;
    signed int mid;

    raw   = (signed int)raw_adc;
    white = (signed int)white_cal;
    black = (signed int)black_cal;

    mid = (white + black) >> 1;

    if (black > white)
    {
        return raw - mid;
    }

    return mid - raw;
}

//===========================================================================
// Motion helpers
//===========================================================================

static void DriveForward(void)
{
    Motor_SetWheels(motor_forward,
                    BASE_SPEED,
                    motor_forward,
                    BASE_SPEED);
}

static void TurnLeft(void)
{
#if SHARP_SINGLE_SENSOR_TURN
    Motor_SetWheels(motor_reverse,
                    BASE_SPEED,
                    motor_forward,
                    BASE_SPEED);
#else
    Motor_SetWheels(motor_off,
                    0u,
                    motor_forward,
                    BASE_SPEED);
#endif
}

static void TurnRight(void)
{
#if SHARP_SINGLE_SENSOR_TURN
    Motor_SetWheels(motor_forward,
                    BASE_SPEED,
                    motor_reverse,
                    BASE_SPEED);
#else
    Motor_SetWheels(motor_forward,
                    BASE_SPEED,
                    motor_off,
                    0u);
#endif
}

static void SearchForLine(void)
{
    if (last_line_side >= 0)
    {
        TurnLeft();
    }
    else
    {
        TurnRight();
    }
}

//===========================================================================
// Follow_Init
//===========================================================================

void Follow_Init(void)
{
    last_update_ms = system_ticks_ms;

    /*
     * Default search direction if the robot starts with both sensors white.
     */
    last_line_side = 1;

    DriveForward();
}

//===========================================================================
// Follow_Task
//===========================================================================

void Follow_Task(void)
{
    unsigned long now;

    signed int left_signal;
    signed int right_signal;

    unsigned char left_black;
    unsigned char right_black;

    now = system_ticks_ms;

    /*
     * Minimum action time.
     * This prevents rapid motor flipping.
     */
    if ((now - last_update_ms) < FOLLOW_UPDATE_MS)
    {
        return;
    }

    last_update_ms = now;

    left_signal = GetBlackSignal(detect_left_raw,
                                 calibrate_white_left,
                                 calibrate_black_left);

    right_signal = GetBlackSignal(detect_right_raw,
                                  calibrate_white_right,
                                  calibrate_black_right);

    left_black  = (left_signal  > BLACK_LEVEL);
    right_black = (right_signal > BLACK_LEVEL);

    /*
     * Both sensors see black:
     * line is centered enough, keep going.
     */
    if (left_black && right_black)
    {
        DriveForward();
        return;
    }

    /*
     * Left sensor sees black:
     * line is on the left side, turn left.
     */
    if (left_black)
    {
        last_line_side = 1;
        TurnLeft();
        return;
    }

    /*
     * Right sensor sees black:
     * line is on the right side, turn right.
     */
    if (right_black)
    {
        last_line_side = -1;
        TurnRight();
        return;
    }

    /*
     * Both sensors see white:
     * keep turning toward the last known line side.
     */
    SearchForLine();
}
