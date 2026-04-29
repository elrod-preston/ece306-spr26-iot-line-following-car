//===========================================================================
// File Name : command.c
//
// Description:
//   Simplified IOT command coordinator.
//
// Supported commands:
//
//   PR(string)
//      print string to LCD row 0
//
//   M(+60,+60,1000)
//      left motor  = +60 percent PWM
//      right motor = +60 percent PWM
//      run time    = 1000 ms
//
//   M(+60,+60,~)
//      hold motor command forever, normally used with BLACK / WHITE condition
//
//   M(+60,+60,~),BLACK
//      run motor command until black line is detected
//
//   M(+60,+60,~),BLACK100
//      run until averaged raw >= calibrate_black - 100
//
//   M(+60,+60,~),WHITE100
//      run until averaged raw <= calibrate_white + 100
//
//   PA1000
//      pause / motors off for 1000 ms
//
//   F1000
//   B1000
//   L500
//   R500
//      simple timed movement using DEFAULT_SPEED / TURN_SPEED
//
//   F~
//   B~
//   L~
//   R~
//      hold movement forever, normally used with BLACK / WHITE condition
//
//   F~,BLACK
//      drive forward until black line is detected
//
//   F~,BLACK100
//      drive forward until averaged raw >= calibrate_black - 100
//
//   F~,WHITE100
//      drive forward until averaged raw <= calibrate_white + 100
//
//   BLKFL
//   BLKFL1000
//   BLKFL~
//   BLKFL~,WHITE
//      black line follow forever, timed, or until condition
//
//   BLACK
//   BLACK100
//   WHITE
//   WHITE100
//      blocking sensor conditions
//
//   STOP / S
//   RST / RESET
//
// Display during waiting:
//   Row 0 = Waiting
//   Row 1 = for input
//   Row 2 = ID line 1
//   Row 3 = ID line 2
//
// Display during course:
//   Row 0 = PR(...) text, such as Arrived 01 / BL Start / Intercept
//   Row 1 = ID line 1
//   Row 2 = ID line 2
//   Row 3 = first 5 chars of last command + seconds counter
//
// Display at final BL Stop:
//   Row 0 = BL Stop
//   Row 1 = custom completion line 1
//   Row 2 = custom completion line 2
//   Row 3 = Time: XXXs
//
// Notes:
//   - No sscanf.
//   - No stdio.h.
//   - Detection is serviced outside this coordinator.
//   - This coordinator only reads detect_left_raw / detect_right_raw.
//===========================================================================

#include <msp430.h>
#include <string.h>

#include "Core\\lib\\motor.h"
#include "Core\\lib\\display.h"
#include "Core\\lib\\iot.h"

#include "Addon\\lib\\command.h"
#include "Addon\\lib\\follow.h"
#include "Addon\\lib\\calibrate.h"
#include "Addon\\lib\\detect.h"

//===========================================================================
// External timebase
//===========================================================================

extern volatile unsigned long system_ticks_ms;

//===========================================================================
// Defines
//===========================================================================

#define CMD_BUF_SIZE          (300u)
#define CMD_TOKEN_SIZE        (60u)

#define DEFAULT_SPEED         (50)
#define TURN_SPEED            (50)

#define ST_IDLE               (0u)
#define ST_MOTOR              (1u)
#define ST_PAUSE              (2u)
#define ST_BLKFL              (3u)
#define ST_WAIT               (4u)
#define ST_DONE               (5u)

#define WAIT_NONE             (0u)
#define WAIT_BLACK            (1u)
#define WAIT_WHITE            (2u)

#define WAIT_GOOD_REQUIRED    (3u)
#define WAIT_AVG_SAMPLES      (5u)
#define WAIT_AVG_SAMPLE_MS    (10u)

#define TIMER_MAX_SECONDS     (999u)

#define LAST_CMD_CHARS        (5u)

//===========================================================================
// State
//===========================================================================

static unsigned char coord_state;
static unsigned char coord_busy;

static char command_buf[CMD_BUF_SIZE];
static char iot_buf[CMD_BUF_SIZE];

static unsigned int command_index;

static unsigned long start_ms;
static unsigned long run_ms;

static unsigned char command_hold;

static unsigned char wait_type;
static unsigned int wait_margin;
static unsigned char wait_good_count;

static unsigned int wait_left_samples[WAIT_AVG_SAMPLES];
static unsigned int wait_right_samples[WAIT_AVG_SAMPLES];
static unsigned char wait_avg_index;
static unsigned char wait_avg_count;
static unsigned long wait_avg_last_ms;

static unsigned char timer_running;
static unsigned long timer_start_ms;
static unsigned int timer_seconds;
static unsigned int last_timer_seconds;

static char id_line_1[11];
static char id_line_2[11];

static char done_line_1[11];
static char done_line_2[11];

static char lcd_line[11];

static char top_line[11];
static unsigned char top_visible;
static unsigned char top_final;

static char last_cmd_line[6];

//===========================================================================
// Small helpers
//===========================================================================

static unsigned char IsSpace(char c)
{
    return ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'));
}

static unsigned char IsDigit(char c)
{
    return ((c >= '0') && (c <= '9'));
}

static char ToUpper(char c)
{
    if ((c >= 'a') && (c <= 'z'))
    {
        return (char)(c - 32);
    }

    return c;
}

static unsigned char SameNoCase(char *a, const char *b)
{
    unsigned int i;

    i = 0u;

    while ((a[i] != '\0') && (b[i] != '\0'))
    {
        if (ToUpper(a[i]) != ToUpper(b[i]))
        {
            return 0u;
        }

        i++;
    }

    return ((a[i] == '\0') && (b[i] == '\0'));
}

static unsigned char StartsWithNoCase(char *text, const char *prefix)
{
    unsigned int i;

    i = 0u;

    while (prefix[i] != '\0')
    {
        if (ToUpper(text[i]) != ToUpper(prefix[i]))
        {
            return 0u;
        }

        i++;
    }

    return 1u;
}

static void Copy10(char *dst, const char *src)
{
    unsigned int i;

    for (i = 0u; i < 10u; i++)
    {
        dst[i] = ' ';
    }

    dst[10] = '\0';

    if (src == 0)
    {
        return;
    }

    for (i = 0u; (i < 10u) && (src[i] != '\0'); i++)
    {
        dst[i] = src[i];
    }
}

static void Blank10(char *dst)
{
    unsigned int i;

    for (i = 0u; i < 10u; i++)
    {
        dst[i] = ' ';
    }

    dst[10] = '\0';
}

static void SkipSpaces(char *cmd, unsigned int *index)
{
    while (IsSpace(cmd[*index]))
    {
        (*index)++;
    }
}

//===========================================================================
// Timer
//===========================================================================

static void Timer_Update(void);

static void Timer_Reset(void)
{
    timer_running = 0u;
    timer_start_ms = 0ul;
    timer_seconds = 0u;
    last_timer_seconds = 9999u;
}

static void Timer_Start(void)
{
    if (!timer_running)
    {
        timer_running = 1u;
        timer_start_ms = system_ticks_ms - ((unsigned long)timer_seconds * 1000ul);
        last_timer_seconds = 9999u;
    }
}

static void Timer_Stop(void)
{
    Timer_Update();
    timer_running = 0u;
}

static void Timer_Update(void)
{
    unsigned long sec;

    if (!timer_running)
    {
        return;
    }

    sec = (system_ticks_ms - timer_start_ms) / 1000ul;

    if (sec > TIMER_MAX_SECONDS)
    {
        sec = TIMER_MAX_SECONDS;
    }

    timer_seconds = (unsigned int)sec;
}

static void FormatCourseTimerLine(char *line)
{
    unsigned int sec;
    unsigned int i;

    sec = timer_seconds;

    if (sec > TIMER_MAX_SECONDS)
    {
        sec = TIMER_MAX_SECONDS;
    }

    for (i = 0u; i < LAST_CMD_CHARS; i++)
    {
        line[i] = last_cmd_line[i];
    }

    line[5] = ' ';
    line[6] = (char)('0' + ((sec / 100u) % 10u));
    line[7] = (char)('0' + ((sec / 10u) % 10u));
    line[8] = (char)('0' + (sec % 10u));
    line[9] = 's';
    line[10] = '\0';
}

static void FormatFinalTimerLine(char *line)
{
    unsigned int sec;

    sec = timer_seconds;

    if (sec > TIMER_MAX_SECONDS)
    {
        sec = TIMER_MAX_SECONDS;
    }

    line[0] = 'T';
    line[1] = 'i';
    line[2] = 'm';
    line[3] = 'e';
    line[4] = ':';
    line[5] = ' ';
    line[6] = (char)('0' + ((sec / 100u) % 10u));
    line[7] = (char)('0' + ((sec / 10u) % 10u));
    line[8] = (char)('0' + (sec % 10u));
    line[9] = 's';
    line[10] = '\0';
}

//===========================================================================
// LCD
//===========================================================================

static void LCD_Line(unsigned char row, const char *text)
{
    Copy10(lcd_line, text);
    Display_Text(row, lcd_line);
}

static void ClearTopLine(void)
{
    top_visible = 0u;
    top_final = 0u;
    Blank10(top_line);
}

static void SetTopLine(const char *text)
{
    top_visible = 1u;
    top_final = 0u;

    Copy10(top_line, text);

    if (text != 0)
    {
        if (StartsWithNoCase((char *)text, "BL Stop"))
        {
            top_final = 1u;
        }
    }
}

static void ClearLastCommand(void)
{
    unsigned int i;

    for (i = 0u; i < LAST_CMD_CHARS; i++)
    {
        last_cmd_line[i] = ' ';
    }

    last_cmd_line[LAST_CMD_CHARS] = '\0';
}

static void SetLastCommand(char *cmd)
{
    unsigned int i;
    unsigned int j;

    ClearLastCommand();

    if (cmd == 0)
    {
        return;
    }

    i = 0u;
    j = 0u;

    while (IsSpace(cmd[i]))
    {
        i++;
    }

    while ((cmd[i] != '\0') &&
           (cmd[i] != ',') &&
           (cmd[i] != ';') &&
           (j < LAST_CMD_CHARS))
    {
        if (!IsSpace(cmd[i]))
        {
            last_cmd_line[j] = cmd[i];
            j++;
        }

        i++;
    }
}

static void LCD_Render(void)
{
    Timer_Update();

    if ((timer_running == 0u) &&
        (timer_seconds == 0u) &&
        (coord_state == ST_IDLE))
    {
        LCD_Line(0u, "Waiting");
        LCD_Line(1u, "for input");
        LCD_Line(2u, id_line_1);
        LCD_Line(3u, id_line_2);
        UpdateDisplay();
        return;
    }

    if (top_final)
    {
        LCD_Line(0u, top_line);
        LCD_Line(1u, done_line_1);
        LCD_Line(2u, done_line_2);

        FormatFinalTimerLine(lcd_line);
        Display_Text(3u, lcd_line);

        UpdateDisplay();

        last_timer_seconds = timer_seconds;
        return;
    }

    if (top_visible)
    {
        LCD_Line(0u, top_line);
    }
    else
    {
        LCD_Line(0u, "");
    }

    LCD_Line(1u, id_line_1);
    LCD_Line(2u, id_line_2);

    FormatCourseTimerLine(lcd_line);
    Display_Text(3u, lcd_line);

    UpdateDisplay();

    last_timer_seconds = timer_seconds;
}

//===========================================================================
// Manual number parsers
//===========================================================================

static unsigned char ParseUnsigned(char *cmd,
                                   unsigned int *index,
                                   unsigned long *out)
{
    unsigned long value;
    unsigned char found;

    value = 0ul;
    found = 0u;

    SkipSpaces(cmd, index);

    while (IsDigit(cmd[*index]))
    {
        found = 1u;
        value = (value * 10ul) + (unsigned long)(cmd[*index] - '0');
        (*index)++;
    }

    SkipSpaces(cmd, index);

    if (!found)
    {
        return 0u;
    }

    *out = value;
    return 1u;
}

static unsigned char ParseSigned(char *cmd,
                                 unsigned int *index,
                                 signed int *out)
{
    signed int sign;
    unsigned long value;

    sign = 1;
    value = 0ul;

    SkipSpaces(cmd, index);

    if (cmd[*index] == '+')
    {
        (*index)++;
    }
    else if (cmd[*index] == '-')
    {
        sign = -1;
        (*index)++;
    }

    if (!ParseUnsigned(cmd, index, &value))
    {
        return 0u;
    }

    if (value > 100ul)
    {
        value = 100ul;
    }

    *out = (signed int)value * sign;
    return 1u;
}

//===========================================================================
// Command buffer / tokenizer
//
// Splits on comma or semicolon, except inside (...).
// This preserves spaces and case inside PR(...).
//===========================================================================

static void ClearCommandBuffer(void)
{
    command_buf[0] = '\0';
    command_index = 0u;
}

static unsigned char GetNextToken(char *token)
{
    unsigned int ti;
    unsigned int depth;
    char c;

    ti = 0u;
    depth = 0u;

    token[0] = '\0';

    while ((command_buf[command_index] == ',') ||
           (command_buf[command_index] == ';') ||
           IsSpace(command_buf[command_index]))
    {
        command_index++;
    }

    if (command_buf[command_index] == '\0')
    {
        return 0u;
    }

    while (command_buf[command_index] != '\0')
    {
        c = command_buf[command_index];

        if (c == '(')
        {
            depth++;
        }
        else if ((c == ')') && (depth > 0u))
        {
            depth--;
        }

        if ((depth == 0u) && ((c == ',') || (c == ';')))
        {
            break;
        }

        if (ti < (CMD_TOKEN_SIZE - 1u))
        {
            token[ti] = c;
            ti++;
        }

        command_index++;
    }

    while ((ti > 0u) && IsSpace(token[ti - 1u]))
    {
        ti--;
    }

    token[ti] = '\0';

    if ((command_buf[command_index] == ',') ||
        (command_buf[command_index] == ';'))
    {
        command_index++;
    }

    return 1u;
}

//===========================================================================
// Condition logic
//===========================================================================

static void WaitAverageReset(void)
{
    unsigned char i;

    for (i = 0u; i < WAIT_AVG_SAMPLES; i++)
    {
        wait_left_samples[i] = 0u;
        wait_right_samples[i] = 0u;
    }

    wait_avg_index = 0u;
    wait_avg_count = 0u;
    wait_avg_last_ms = 0ul;
}

static void ClearWait(void)
{
    wait_type = WAIT_NONE;
    wait_margin = 0u;
    wait_good_count = 0u;
    WaitAverageReset();
}

static unsigned char WaitAverageSampleAlreadyUsed(unsigned int left_raw,
                                                  unsigned int right_raw)
{
    unsigned char i;

    for (i = 0u; i < wait_avg_count; i++)
    {
        if ((wait_left_samples[i] == left_raw) &&
            (wait_right_samples[i] == right_raw))
        {
            return 1u;
        }
    }

    return 0u;
}

static unsigned char WaitAverageUpdate(void)
{
    unsigned int left_now;
    unsigned int right_now;

    if (wait_avg_count != 0u)
    {
        if ((system_ticks_ms - wait_avg_last_ms) < WAIT_AVG_SAMPLE_MS)
        {
            return 0u;
        }
    }

    left_now = detect_left_raw;
    right_now = detect_right_raw;

    /*
     * Only accept a reading if this left/right pair is different from
     * every reading currently inside the running-average buffer.
     *
     * This prevents the command loop from filling the 5-sample average
     * with the same ADC result if this task runs faster than detect.c
     * updates the raw sensor globals.
     */
    if (WaitAverageSampleAlreadyUsed(left_now, right_now))
    {
        return 0u;
    }

    wait_avg_last_ms = system_ticks_ms;

    wait_left_samples[wait_avg_index] = left_now;
    wait_right_samples[wait_avg_index] = right_now;

    wait_avg_index++;

    if (wait_avg_index >= WAIT_AVG_SAMPLES)
    {
        wait_avg_index = 0u;
    }

    if (wait_avg_count < WAIT_AVG_SAMPLES)
    {
        wait_avg_count++;
    }

    return 1u;
}

static void WaitAverageGet(unsigned int *left_avg, unsigned int *right_avg)
{
    unsigned char i;
    unsigned long left_sum;
    unsigned long right_sum;

    left_sum = 0ul;
    right_sum = 0ul;

    if (wait_avg_count == 0u)
    {
        *left_avg = detect_left_raw;
        *right_avg = detect_right_raw;
        return;
    }

    for (i = 0u; i < wait_avg_count; i++)
    {
        left_sum += (unsigned long)wait_left_samples[i];
        right_sum += (unsigned long)wait_right_samples[i];
    }

    *left_avg = (unsigned int)(left_sum / (unsigned long)wait_avg_count);
    *right_avg = (unsigned int)(right_sum / (unsigned long)wait_avg_count);
}

static unsigned int BlackThreshold(unsigned int black_cal,
                                   unsigned int sensitivity)
{
    if (sensitivity >= black_cal)
    {
        return 0u;
    }

    return (black_cal - sensitivity);
}

static unsigned int WhiteThreshold(unsigned int white_cal,
                                   unsigned int sensitivity)
{
    unsigned long threshold;

    threshold = (unsigned long)white_cal + (unsigned long)sensitivity;

    if (threshold > 65535ul)
    {
        threshold = 65535ul;
    }

    return (unsigned int)threshold;
}

static unsigned char ConditionSampleGood(unsigned int left_raw,
                                         unsigned int right_raw)
{
    unsigned int left_threshold;
    unsigned int right_threshold;

    if (wait_type == WAIT_BLACK)
    {
        left_threshold = BlackThreshold((unsigned int)calibrate_black_left,
                                        wait_margin);

        right_threshold = BlackThreshold((unsigned int)calibrate_black_right,
                                         wait_margin);

        if ((left_raw >= left_threshold) ||
            (right_raw >= right_threshold))
        {
            return 1u;
        }

        return 0u;
    }

    if (wait_type == WAIT_WHITE)
    {
        left_threshold = WhiteThreshold((unsigned int)calibrate_white_left,
                                        wait_margin);

        right_threshold = WhiteThreshold((unsigned int)calibrate_white_right,
                                         wait_margin);

        if ((left_raw <= left_threshold) ||
            (right_raw <= right_threshold))
        {
            return 1u;
        }

        return 0u;
    }

    return 1u;
}

static unsigned char ConditionDone(void)
{
    unsigned int left_avg;
    unsigned int right_avg;

    if (wait_type == WAIT_NONE)
    {
        return 1u;
    }

    if (!WaitAverageUpdate())
    {
        return 0u;
    }

    WaitAverageGet(&left_avg, &right_avg);

    if (ConditionSampleGood(left_avg, right_avg))
    {
        if (wait_good_count < WAIT_GOOD_REQUIRED)
        {
            wait_good_count++;
        }

        if (wait_good_count >= WAIT_GOOD_REQUIRED)
        {
            return 1u;
        }
    }
    else
    {
        wait_good_count = 0u;
    }

    return 0u;
}

static unsigned char ParseCondition(char *cmd,
                                    unsigned char *type,
                                    unsigned int *margin)
{
    unsigned int i;
    unsigned long value;

    if (StartsWithNoCase(cmd, "BLACK"))
    {
        *type = WAIT_BLACK;
        *margin = 0u;
        i = 5u;

        if (cmd[i] == '\0')
        {
            return 1u;
        }

        if (!ParseUnsigned(cmd, &i, &value))
        {
            return 0u;
        }

        if (cmd[i] != '\0')
        {
            return 0u;
        }

        if (value > 65535ul)
        {
            value = 65535ul;
        }

        *margin = (unsigned int)value;
        return 1u;
    }

    if (StartsWithNoCase(cmd, "WHITE"))
    {
        *type = WAIT_WHITE;
        *margin = 0u;
        i = 5u;

        if (cmd[i] == '\0')
        {
            return 1u;
        }

        if (!ParseUnsigned(cmd, &i, &value))
        {
            return 0u;
        }

        if (cmd[i] != '\0')
        {
            return 0u;
        }

        if (value > 65535ul)
        {
            value = 65535ul;
        }

        *margin = (unsigned int)value;
        return 1u;
    }

    return 0u;
}

static void StartWait(unsigned char type, unsigned int margin)
{
    Timer_Start();

    wait_type = type;
    wait_margin = margin;
    wait_good_count = 0u;
    WaitAverageReset();

    coord_busy = 1u;

    if (coord_state == ST_IDLE)
    {
        Motor_Stop();
        coord_state = ST_WAIT;
    }

    LCD_Render();
}

//===========================================================================
// Command parsers
//===========================================================================

static unsigned char ParsePrint(char *cmd, char **text)
{
    unsigned int len;

    if ((ToUpper(cmd[0]) != 'P') ||
        (ToUpper(cmd[1]) != 'R') ||
        (cmd[2] != '('))
    {
        return 0u;
    }

    len = (unsigned int)strlen(cmd);

    if (len < 4u)
    {
        return 0u;
    }

    if (cmd[len - 1u] != ')')
    {
        return 0u;
    }

    cmd[len - 1u] = '\0';
    *text = &cmd[3];

    return 1u;
}

static unsigned char ParseM(char *cmd,
                            signed int *left,
                            signed int *right,
                            unsigned long *ms,
                            unsigned char *hold)
{
    unsigned int i;

    i = 0u;

    *ms = 0ul;
    *hold = 0u;

    if (ToUpper(cmd[i]) != 'M')
    {
        return 0u;
    }

    i++;

    SkipSpaces(cmd, &i);

    if (cmd[i] != '(')
    {
        return 0u;
    }

    i++;

    if (!ParseSigned(cmd, &i, left))
    {
        return 0u;
    }

    if (cmd[i] != ',')
    {
        return 0u;
    }

    i++;

    if (!ParseSigned(cmd, &i, right))
    {
        return 0u;
    }

    if (cmd[i] != ',')
    {
        return 0u;
    }

    i++;

    SkipSpaces(cmd, &i);

    if (cmd[i] == '~')
    {
        *hold = 1u;
        i++;
        SkipSpaces(cmd, &i);
    }
    else
    {
        if (!ParseUnsigned(cmd, &i, ms))
        {
            return 0u;
        }
    }

    if (cmd[i] != ')')
    {
        return 0u;
    }

    i++;

    SkipSpaces(cmd, &i);

    return (cmd[i] == '\0');
}

static unsigned char ParsePause(char *cmd, unsigned long *ms)
{
    unsigned int i;

    if ((ToUpper(cmd[0]) != 'P') || (ToUpper(cmd[1]) != 'A'))
    {
        return 0u;
    }

    i = 2u;

    if (!ParseUnsigned(cmd, &i, ms))
    {
        return 0u;
    }

    return (cmd[i] == '\0');
}

static unsigned char ParseTimedOrHold(char *cmd,
                                      unsigned int index,
                                      unsigned long *ms,
                                      unsigned char *hold)
{
    *ms = 0ul;
    *hold = 0u;

    SkipSpaces(cmd, &index);

    if (cmd[index] == '~')
    {
        index++;
        SkipSpaces(cmd, &index);

        if (cmd[index] != '\0')
        {
            return 0u;
        }

        *hold = 1u;
        return 1u;
    }

    if (cmd[index] == '\0')
    {
        *hold = 1u;
        return 1u;
    }

    if (!ParseUnsigned(cmd, &index, ms))
    {
        return 0u;
    }

    return (cmd[index] == '\0');
}

static unsigned char ParseSimpleMove(char *cmd,
                                     signed int *left,
                                     signed int *right,
                                     unsigned long *ms,
                                     unsigned char *hold)
{
    char action;

    action = ToUpper(cmd[0]);

    if (!ParseTimedOrHold(cmd, 1u, ms, hold))
    {
        return 0u;
    }

    if (action == 'F')
    {
        *left = DEFAULT_SPEED;
        *right = DEFAULT_SPEED;
        return 1u;
    }

    if (action == 'B')
    {
        *left = -DEFAULT_SPEED;
        *right = -DEFAULT_SPEED;
        return 1u;
    }

    if (action == 'L')
    {
        *left = -TURN_SPEED;
        *right = TURN_SPEED;
        return 1u;
    }

    if (action == 'R')
    {
        *left = TURN_SPEED;
        *right = -TURN_SPEED;
        return 1u;
    }

    return 0u;
}

static unsigned char ParseBLKFL(char *cmd,
                                unsigned long *ms,
                                unsigned char *hold)
{
    if (!StartsWithNoCase(cmd, "BLKFL"))
    {
        return 0u;
    }

    return ParseTimedOrHold(cmd, 5u, ms, hold);
}

//===========================================================================
// Execution
//===========================================================================

static void EndCurrentCommand(void)
{
    Motor_Stop();

    coord_state = ST_IDLE;
    start_ms = 0ul;
    run_ms = 0ul;
    command_hold = 0u;
}

static void StopCourse(void)
{
    EndCurrentCommand();
    ClearWait();
    ClearCommandBuffer();
    coord_busy = 0u;
    Timer_Stop();
}

static void ResetCourse(void)
{
    EndCurrentCommand();
    ClearWait();
    ClearCommandBuffer();
    coord_busy = 0u;
    Timer_Reset();
}

static void CancelCurrentForNewCommand(void)
{
    EndCurrentCommand();
    ClearWait();
    ClearCommandBuffer();
    coord_busy = 0u;
}

static void StartMotorCommand(signed int left,
                              signed int right,
                              unsigned long ms,
                              unsigned char hold)
{
    signed char left_state;
    signed char right_state;

    unsigned int left_speed;
    unsigned int right_speed;

    Timer_Start();

    left_state = motor_off;
    right_state = motor_off;

    left_speed = 0u;
    right_speed = 0u;

    if (left > 0)
    {
        left_state = motor_forward;
        left_speed = (unsigned int)left;
    }
    else if (left < 0)
    {
        left_state = motor_reverse;
        left_speed = (unsigned int)(-left);
    }

    if (right > 0)
    {
        right_state = motor_forward;
        right_speed = (unsigned int)right;
    }
    else if (right < 0)
    {
        right_state = motor_reverse;
        right_speed = (unsigned int)(-right);
    }

    Motor_SetWheels(left_state, left_speed,
                    right_state, right_speed);

    start_ms = system_ticks_ms;
    run_ms = ms;
    command_hold = hold;

    coord_state = ST_MOTOR;
    coord_busy = 1u;

    LCD_Render();
}

static void StartPause(unsigned long ms)
{
    Timer_Start();

    Motor_Stop();

    start_ms = system_ticks_ms;
    run_ms = ms;
    command_hold = 0u;

    coord_state = ST_PAUSE;
    coord_busy = 1u;

    LCD_Render();
}

static void StartBlackFollow(unsigned long ms, unsigned char hold)
{
    Timer_Start();

    Motor_Stop();

    start_ms = system_ticks_ms;
    run_ms = ms;
    command_hold = hold;

    coord_state = ST_BLKFL;
    coord_busy = 1u;

    Follow_Init();
    LCD_Render();
}

//===========================================================================
// Chain handling
//===========================================================================

static void StartNextCommand(void);

static unsigned char TryStartInlineCondition(void)
{
    unsigned int saved_index;
    char token[CMD_TOKEN_SIZE];

    unsigned char type;
    unsigned int margin;

    saved_index = command_index;

    if (!GetNextToken(token))
    {
        command_index = saved_index;
        return 0u;
    }

    if (ParseCondition(token, &type, &margin))
    {
        StartWait(type, margin);
        return 1u;
    }

    command_index = saved_index;
    return 0u;
}

static void StartNextCommand(void)
{
    char token[CMD_TOKEN_SIZE];
    char *print_text;

    signed int left;
    signed int right;
    unsigned long ms;
    unsigned char hold;

    unsigned char type;
    unsigned int margin;

    while (GetNextToken(token))
    {
        if (ParsePrint(token, &print_text))
        {
            SetTopLine(print_text);
            LCD_Render();
            continue;
        }

        if (SameNoCase(token, "STOP") || SameNoCase(token, "S"))
        {
            StopCourse();
            LCD_Render();
            return;
        }

        if (SameNoCase(token, "RST") || SameNoCase(token, "RESET"))
        {
            CommandCoordinator_ResetCourse();
            return;
        }

        if (ParseM(token, &left, &right, &ms, &hold))
        {
            StartMotorCommand(left, right, ms, hold);

            if (hold)
            {
                TryStartInlineCondition();
            }

            return;
        }

        if (ParsePause(token, &ms))
        {
            StartPause(ms);
            return;
        }

        if (ParseCondition(token, &type, &margin))
        {
            StartWait(type, margin);
            return;
        }

        if (ParseBLKFL(token, &ms, &hold))
        {
            StartBlackFollow(ms, hold);

            if (hold)
            {
                TryStartInlineCondition();
            }

            return;
        }

        if (ParseSimpleMove(token, &left, &right, &ms, &hold))
        {
            StartMotorCommand(left, right, ms, hold);

            if (hold)
            {
                TryStartInlineCondition();
            }

            return;
        }

        SetTopLine("Bad cmd");
        StopCourse();
        LCD_Render();
        return;
    }

    EndCurrentCommand();
    ClearWait();
    ClearCommandBuffer();
    coord_busy = 0u;
    coord_state = ST_DONE;

    LCD_Render();
}

//===========================================================================
// IOT service
//===========================================================================

static void ServiceIOT(void)
{
    if (IOT_MessageAvailable() == 0u)
    {
        return;
    }

    IOT_GetMessage(iot_buf, CMD_BUF_SIZE);

    if (iot_buf[0] != '\0')
    {
        CommandCoordinator_LoadCommand(iot_buf);
    }
}

//===========================================================================
// Public functions
//===========================================================================

void CommandCoordinator_Init(void)
{
    Copy10(id_line_1, "FName");
    Copy10(id_line_2, "LName");

    Copy10(done_line_1, "That was");
    Copy10(done_line_2, "easy!! ;-)");

    iot_buf[0] = '\0';

    CommandCoordinator_ResetCourse();
}

void CommandCoordinator_ResetCourse(void)
{
    ResetCourse();
    ClearTopLine();
    ClearLastCommand();
    LCD_Render();
}

void CommandCoordinator_LoadCommand(char *cmd)
{
    CancelCurrentForNewCommand();
    ClearTopLine();

    if (cmd == 0)
    {
        LCD_Render();
        return;
    }

    SetLastCommand(cmd);

    strncpy(command_buf, cmd, CMD_BUF_SIZE - 1u);
    command_buf[CMD_BUF_SIZE - 1u] = '\0';

    command_index = 0u;
    coord_busy = 1u;

    Timer_Start();

    StartNextCommand();
}

void CommandCoordinator_Task(void)
{
    ServiceIOT();

    Timer_Update();

    if ((timer_running) &&
        (timer_seconds != last_timer_seconds))
    {
        LCD_Render();
    }

    if (wait_type != WAIT_NONE)
    {
        if (coord_state == ST_BLKFL)
        {
            Follow_Task();
        }

        if (ConditionDone())
        {
            ClearWait();
            EndCurrentCommand();
            StartNextCommand();
        }

        return;
    }

    if (coord_state == ST_BLKFL)
    {
        Follow_Task();
    }

    if ((coord_state == ST_IDLE) || (coord_state == ST_DONE))
    {
        return;
    }

    if (command_hold)
    {
        return;
    }

    if ((coord_state == ST_MOTOR) ||
        (coord_state == ST_PAUSE) ||
        (coord_state == ST_BLKFL))
    {
        if ((system_ticks_ms - start_ms) >= run_ms)
        {
            EndCurrentCommand();
            StartNextCommand();
        }
    }
}

void CommandCoordinator_Stop(void)
{
    StopCourse();
    LCD_Render();
}

unsigned char CommandCoordinator_IsBusy(void)
{
    if (coord_busy)
    {
        return 1u;
    }

    if ((coord_state != ST_IDLE) && (coord_state != ST_DONE))
    {
        return 1u;
    }

    if (wait_type != WAIT_NONE)
    {
        return 1u;
    }

    return 0u;
}

//===========================================================================
// Compatibility functions
//===========================================================================

void CommandCoordinator_SetIDLines(char *line1, char *line2)
{
    Copy10(id_line_1, line1);
    Copy10(id_line_2, line2);
    LCD_Render();
}

void CommandCoordinator_SetDetectorValues(unsigned int left, unsigned int right)
{
    (void)left;
    (void)right;
}
