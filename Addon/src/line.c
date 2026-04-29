//===========================================================================
// File Name : line.c
//
// Description:
//   Robust non-PID line intercept / alignment sequence.
//
// Command behavior:
//   LINE runs until the intercept sequence is complete.
//
// Sequence:
//   1) Drive forward until either sensor sees black
//   2) Overshoot forward
//   3) Reverse until one sensor is black and the other is white
//   4) Overshoot reverse
//   5) Fast tank turn away from detected side until both sensors lose line
//   6) Overshoot fast turn
//   7) Slow tank/search turn until both sensors see black
//   8) Overshoot slow turn
//   9) Stop and report done
//
// Notes:
//   - No PID
//   - No display
//   - No ADC_ReadChannel()
//   - No Detect_Task()
//   - No detect_request
//   - Uses detect_left_raw / detect_right_raw directly
//   - Uses calibrate_white_* and calibrate_black_* directly
//   - Uses signed black signal like follow.c:
//       positive = black
//       near 0   = midpoint
//       negative = white
//   - Handles either sensor polarity:
//       black > white OR black < white
//===========================================================================

#include <msp430.h>

#include "Core\\lib\\ports.h"
#include "Core\\lib\\motor.h"

#include "Addon\\lib\\line.h"
#include "Addon\\lib\\detect.h"
#include "Addon\\lib\\calibrate.h"

//===========================================================================
// External timebase
//===========================================================================

extern volatile unsigned long system_ticks_ms;

//===========================================================================
// Main timing
//===========================================================================

// Minimum action/update time.
// This prevents rapid state flipping and makes each motor action last long
// enough to physically move the robot.
#define LINE_UPDATE_MS                  (50u)

//===========================================================================
// Motor tuning
//===========================================================================

#define LINE_FORWARD_SPEED              (45u)
#define LINE_REVERSE_SPEED              (35u)
#define LINE_FAST_TURN_SPEED            (55u)
#define LINE_SLOW_TURN_SPEED            (28u)

//===========================================================================
// Sensor tuning
//===========================================================================

// Signed black signal:
//   positive = black
//   negative = white
//
// These are intentionally similar to follow.c, but used only for simple
// conditions, not PID.
#define SIGNAL_LIMIT                    (1024L)

#define BLACK_PRESENT_LEVEL             (45L)
#define WHITE_PRESENT_LEVEL             (-45L)

#define LINE_STABLE_COUNT               (3u)

//===========================================================================
// Overshoot timing
//===========================================================================

#define LINE_OVERSHOOT_BLACK_MS         (200ul)
#define LINE_OVERSHOOT_EDGE_MS          (0ul)
#define LINE_OVERSHOOT_LOST_MS          (100ul)
#define LINE_OVERSHOOT_BOTH_BLACK_MS    (200ul)

//===========================================================================
// Safety timeouts
//===========================================================================

#define LINE_REVERSE_TIMEOUT_MS         (1800ul)
#define LINE_FAST_TURN_TIMEOUT_MS       (1600ul)
#define LINE_SLOW_FIND_TIMEOUT_MS       (5000ul)

//===========================================================================
// State values
//===========================================================================

#define LINE_ST_FORWARD_TO_BLACK        (0u)
#define LINE_ST_OVERSHOOT_BLACK         (1u)
#define LINE_ST_REVERSE_TO_EDGE         (2u)
#define LINE_ST_OVERSHOOT_EDGE          (3u)
#define LINE_ST_FAST_TURN_LOSE          (4u)
#define LINE_ST_OVERSHOOT_LOST          (5u)
#define LINE_ST_SLOW_FIND_BOTH          (6u)
#define LINE_ST_OVERSHOOT_BOTH_BLACK    (7u)
#define LINE_ST_DONE                    (8u)

//===========================================================================
// Side / turn values
//===========================================================================

#define LINE_SIDE_NONE                  (0u)
#define LINE_SIDE_LEFT                  (1u)
#define LINE_SIDE_RIGHT                 (2u)

#define LINE_TURN_NONE                  (0u)
#define LINE_TURN_LEFT                  (1u)
#define LINE_TURN_RIGHT                 (2u)

//===========================================================================
// Internal globals
//===========================================================================

static unsigned char line_state;
static unsigned char line_done;

static unsigned char line_side;
static unsigned char line_last_slow_turn;

static unsigned char stable_count;

static unsigned long state_start_ms;
static unsigned long last_update_ms;

static int current_left_state;
static int current_right_state;

static unsigned int current_left_speed;
static unsigned int current_right_speed;

//===========================================================================
// Small helpers
//===========================================================================

static signed long ShiftRightSigned(signed long value, unsigned char shift)
{
    if (value >= 0L)
    {
        return value >> shift;
    }

    return -((-value) >> shift);
}

static signed long ClampLong(signed long value,
                             signed long min_value,
                             signed long max_value)
{
    if (value < min_value)
    {
        return min_value;
    }

    if (value > max_value)
    {
        return max_value;
    }

    return value;
}

//===========================================================================
// Calibration helper
//===========================================================================

static unsigned char CalibrationReady(void)
{
    if (!calibrate_have_white)
    {
        return 0u;
    }

    if (!calibrate_have_black)
    {
        return 0u;
    }

    if (calibrate_white_left == calibrate_black_left)
    {
        return 0u;
    }

    if (calibrate_white_right == calibrate_black_right)
    {
        return 0u;
    }

    return 1u;
}

//===========================================================================
// Sensor helper
//
// Returns signed black signal:
//
//   positive = sensor sees black
//   near 0   = near midpoint
//   negative = sensor sees white
//
// Uses shifts instead of division.
// This matches the style used in follow.c.
//===========================================================================

static signed long GetBlackSignal(unsigned int raw,
                                  unsigned int white_cal,
                                  unsigned int black_cal)
{
    signed long mid;
    signed long signal;
    signed long range;

    mid = ((signed long)white_cal + (signed long)black_cal) >> 1;

    if (black_cal > white_cal)
    {
        // Black reads higher than white.
        signal = (signed long)raw - mid;
        range = (signed long)black_cal - (signed long)white_cal;
    }
    else
    {
        // Black reads lower than white.
        signal = mid - (signed long)raw;
        range = (signed long)white_cal - (signed long)black_cal;
    }

    /*
     * Approximate normalization without division.
     *
     * Larger calibration range means the signal is naturally stronger,
     * so scale it down. Smaller range means weak signal, so scale it up.
     */
    if (range >= 2048L)
    {
        signal = ShiftRightSigned(signal, 1u);
    }
    else if (range >= 1024L)
    {
        signal = signal;
    }
    else if (range >= 512L)
    {
        signal = signal << 1;
    }
    else if (range >= 256L)
    {
        signal = signal << 2;
    }
    else if (range >= 128L)
    {
        signal = signal << 3;
    }
    else
    {
        signal = signal << 4;
    }

    signal = ClampLong(signal, -SIGNAL_LIMIT, SIGNAL_LIMIT);

    return signal;
}

static signed long LeftSignal(void)
{
    return GetBlackSignal((unsigned int)detect_left_raw,
                          (unsigned int)calibrate_white_left,
                          (unsigned int)calibrate_black_left);
}

static signed long RightSignal(void)
{
    return GetBlackSignal((unsigned int)detect_right_raw,
                          (unsigned int)calibrate_white_right,
                          (unsigned int)calibrate_black_right);
}

static unsigned char IsBlack(signed long signal)
{
    return (signal > BLACK_PRESENT_LEVEL);
}

static unsigned char IsWhite(signed long signal)
{
    return (signal < WHITE_PRESENT_LEVEL);
}

//===========================================================================
// State helpers
//===========================================================================

static unsigned long StateElapsed(void)
{
    return system_ticks_ms - state_start_ms;
}

static void EnterState(unsigned char next_state)
{
    line_state = next_state;
    stable_count = 0u;
    state_start_ms = system_ticks_ms;
}

static unsigned char Stable(unsigned char condition)
{
    if (condition)
    {
        if (stable_count < 255u)
        {
            stable_count++;
        }
    }
    else
    {
        stable_count = 0u;
    }

    return (stable_count >= LINE_STABLE_COUNT);
}

//===========================================================================
// Motor helpers
//===========================================================================

static void SetWheels(int left_state,
                      unsigned int left_speed,
                      int right_state,
                      unsigned int right_speed)
{
    motor_left_state = left_state;
    motor_left_speed = left_speed;

    motor_right_state = right_state;
    motor_right_speed = right_speed;

    current_left_state = left_state;
    current_left_speed = left_speed;

    current_right_state = right_state;
    current_right_speed = right_speed;
}

static void HoldCurrentWheels(void)
{
    motor_left_state = current_left_state;
    motor_left_speed = current_left_speed;

    motor_right_state = current_right_state;
    motor_right_speed = current_right_speed;
}

static void WheelsOff(void)
{
    SetWheels(motor_off, 0u,
              motor_off, 0u);
}

static void Forward(void)
{
    SetWheels(motor_forward, LINE_FORWARD_SPEED,
              motor_forward, LINE_FORWARD_SPEED);
}

static void Reverse(void)
{
    SetWheels(motor_reverse, LINE_REVERSE_SPEED,
              motor_reverse, LINE_REVERSE_SPEED);
}

static void LeftTurnFast(void)
{
    SetWheels(motor_reverse, LINE_FAST_TURN_SPEED,
              motor_forward, LINE_FAST_TURN_SPEED);
}

static void RightTurnFast(void)
{
    SetWheels(motor_forward, LINE_FAST_TURN_SPEED,
              motor_reverse, LINE_FAST_TURN_SPEED);
}

static void LeftTurnSlow(void)
{
    SetWheels(motor_reverse, LINE_SLOW_TURN_SPEED,
              motor_forward, LINE_SLOW_TURN_SPEED);

    line_last_slow_turn = LINE_TURN_LEFT;
}

static void RightTurnSlow(void)
{
    SetWheels(motor_forward, LINE_SLOW_TURN_SPEED,
              motor_reverse, LINE_SLOW_TURN_SPEED);

    line_last_slow_turn = LINE_TURN_RIGHT;
}

static void FastTurnAwayFromDetectedSide(void)
{
    if (line_side == LINE_SIDE_LEFT)
    {
        RightTurnFast();
    }
    else if (line_side == LINE_SIDE_RIGHT)
    {
        LeftTurnFast();
    }
    else
    {
        LeftTurnFast();
    }
}

static void SlowTurnTowardDetectedSide(void)
{
    if (line_side == LINE_SIDE_LEFT)
    {
        LeftTurnSlow();
    }
    else if (line_side == LINE_SIDE_RIGHT)
    {
        RightTurnSlow();
    }
    else
    {
        LeftTurnSlow();
    }
}

static void Done(void)
{
    WheelsOff();

    line_state = LINE_ST_DONE;
    line_done = 1u;
}

//===========================================================================
// Public functions
//===========================================================================

void Line_Init(void)
{
    line_done = 0u;

    line_state = LINE_ST_FORWARD_TO_BLACK;

    line_side = LINE_SIDE_NONE;
    line_last_slow_turn = LINE_TURN_NONE;

    stable_count = 0u;

    state_start_ms = system_ticks_ms;
    last_update_ms = system_ticks_ms;

    current_left_state = motor_off;
    current_right_state = motor_off;

    current_left_speed = 0u;
    current_right_speed = 0u;

    /*
     * Turn on IR LED like follow.c.
     */
    P2DIR |= IR_LED;
    P2OUT |= IR_LED;

    Forward();
}

void Line_Task(void)
{
    unsigned long now;

    signed long left_signal;
    signed long right_signal;

    unsigned char left_black;
    unsigned char right_black;
    unsigned char left_white;
    unsigned char right_white;

    now = system_ticks_ms;

    if (line_done)
    {
        HoldCurrentWheels();
        return;
    }

    if (!CalibrationReady())
    {
        Done();
        return;
    }

    /*
     * Minimum motor-action time.
     * Keep the current command active between decision updates.
     */
    if ((now - last_update_ms) < LINE_UPDATE_MS)
    {
        HoldCurrentWheels();
        return;
    }

    last_update_ms = now;

    left_signal = LeftSignal();
    right_signal = RightSignal();

    left_black = IsBlack(left_signal);
    right_black = IsBlack(right_signal);

    left_white = IsWhite(left_signal);
    right_white = IsWhite(right_signal);

    switch (line_state)
    {
    //-----------------------------------------------------------------------
    // Step 1:
    // Drive forward until either sensor sees black.
    //-----------------------------------------------------------------------
    case LINE_ST_FORWARD_TO_BLACK:
        Forward();

        if (left_black || right_black)
        {
            if (left_black && !right_black)
            {
                line_side = LINE_SIDE_LEFT;
            }
            else if (right_black && !left_black)
            {
                line_side = LINE_SIDE_RIGHT;
            }
            else
            {
                line_side = LINE_SIDE_NONE;
            }

            EnterState(LINE_ST_OVERSHOOT_BLACK);
        }
        break;

    //-----------------------------------------------------------------------
    // Step 2:
    // Keep driving forward after black is first detected.
    //-----------------------------------------------------------------------
    case LINE_ST_OVERSHOOT_BLACK:
        Forward();

        if (StateElapsed() >= LINE_OVERSHOOT_BLACK_MS)
        {
            EnterState(LINE_ST_REVERSE_TO_EDGE);
        }
        break;

    //-----------------------------------------------------------------------
    // Step 3:
    // Reverse until the robot is sitting on a line edge.
    //
    // Edge means:
    //   one sensor sees black
    //   the other sensor sees white
    //-----------------------------------------------------------------------
    case LINE_ST_REVERSE_TO_EDGE:
        Reverse();

        if (Stable((left_black && right_white) ||
                   (right_black && left_white)))
        {
            if (left_black)
            {
                line_side = LINE_SIDE_LEFT;
            }
            else
            {
                line_side = LINE_SIDE_RIGHT;
            }

            EnterState(LINE_ST_OVERSHOOT_EDGE);
        }
        else if (StateElapsed() >= LINE_REVERSE_TIMEOUT_MS)
        {
            EnterState(LINE_ST_FAST_TURN_LOSE);
        }
        break;

    //-----------------------------------------------------------------------
    // Step 4:
    // Keep reversing after the edge is found.
    //-----------------------------------------------------------------------
    case LINE_ST_OVERSHOOT_EDGE:
        Reverse();

        if (StateElapsed() >= LINE_OVERSHOOT_EDGE_MS)
        {
            EnterState(LINE_ST_FAST_TURN_LOSE);
        }
        break;

    //-----------------------------------------------------------------------
    // Step 5:
    // Fast tank turn away from the detected side until both sensors are white.
    //-----------------------------------------------------------------------
    case LINE_ST_FAST_TURN_LOSE:
        FastTurnAwayFromDetectedSide();

        if (Stable(left_white && right_white))
        {
            EnterState(LINE_ST_OVERSHOOT_LOST);
        }
        else if (StateElapsed() >= LINE_FAST_TURN_TIMEOUT_MS)
        {
            EnterState(LINE_ST_SLOW_FIND_BOTH);
        }
        break;

    //-----------------------------------------------------------------------
    // Step 6:
    // Keep fast turning briefly after the line is lost.
    //-----------------------------------------------------------------------
    case LINE_ST_OVERSHOOT_LOST:
        FastTurnAwayFromDetectedSide();

        if (StateElapsed() >= LINE_OVERSHOOT_LOST_MS)
        {
            EnterState(LINE_ST_SLOW_FIND_BOTH);
        }
        break;

    //-----------------------------------------------------------------------
    // Step 7:
    // Slow search turn until both sensors see black.
    //-----------------------------------------------------------------------
    case LINE_ST_SLOW_FIND_BOTH:
        if (left_black && right_black)
        {
            if (Stable(1u))
            {
                EnterState(LINE_ST_OVERSHOOT_BOTH_BLACK);
            }

            break;
        }

        stable_count = 0u;

        /*
         * If only one sensor sees black, turn toward that side so the other
         * sensor can come onto the line.
         */
        if (left_black && !right_black)
        {
            line_side = LINE_SIDE_LEFT;
            LeftTurnSlow();
        }
        else if (right_black && !left_black)
        {
            line_side = LINE_SIDE_RIGHT;
            RightTurnSlow();
        }
        else
        {
            SlowTurnTowardDetectedSide();
        }

        if (StateElapsed() >= LINE_SLOW_FIND_TIMEOUT_MS)
        {
            EnterState(LINE_ST_FORWARD_TO_BLACK);
        }
        break;

    //-----------------------------------------------------------------------
    // Step 8:
    // Keep slow turning briefly after both sensors see black.
    //-----------------------------------------------------------------------
    case LINE_ST_OVERSHOOT_BOTH_BLACK:
        if (line_last_slow_turn == LINE_TURN_RIGHT)
        {
            RightTurnSlow();
        }
        else
        {
            LeftTurnSlow();
        }

        if (StateElapsed() >= LINE_OVERSHOOT_BOTH_BLACK_MS)
        {
            Done();
        }
        break;

    case LINE_ST_DONE:
        Done();
        break;

    default:
        Done();
        break;
    }
}

unsigned char Line_IsDone(void)
{
    return line_done;
}
