/*
//===========================================================================
// File Name : shape.c
//===========================================================================

#include <msp430.h>
#include <string.h>

#include "Core\lib\display.h"
#include "Core\lib\motor.h"
#include "Addon\lib\shape.h"

// From interrupt.c: increments every 200ms
extern volatile unsigned int Time_Sequence;

// 200ms ticks
#define T1S   (5u)
#define T2S   (10u)
#define T3S   (15u)

// simple step state
static unsigned char step = 0;

static void ShowStepText(unsigned char s)
{
    ChangeDisplay("Motion:", 0);

    switch(s)
    {
        case 0: ChangeDisplay("FWD 1s",      1); break;
        case 1: ChangeDisplay("PAUSE 1s",    1); break;
        case 2: ChangeDisplay("REV 2s",      1); break;
        case 3: ChangeDisplay("PAUSE 1s",    1); break;
        case 4: ChangeDisplay("FWD 1s",      1); break;
        case 5: ChangeDisplay("PAUSE 1s",    1); break;
        case 6: ChangeDisplay("SPIN CW 3s",  1); break;
        case 7: ChangeDisplay("PAUSE 2s",    1); break;
        case 8: ChangeDisplay("SPIN CCW 3s", 1); break;
        case 9: ChangeDisplay("PAUSE 2s",    1); break;
        default: ChangeDisplay("DONE",       1); break;
    }

    ChangeDisplay(" ",        2);
    ChangeDisplay("Running",  3);
}

static void LoadStep(unsigned char s)
{
    // scripts are "cmd + ticks"
    // keep these small and obvious
    switch(s)
    {
        case 0: CommandCoordinator("F5");  break;   // forward 1s
        case 1: CommandCoordinator("~5");  break;   // pause 1s
        case 2: CommandCoordinator("R10"); break;   // reverse 2s
        case 3: CommandCoordinator("~5");  break;   // pause 1s
        case 4: CommandCoordinator("F5");  break;   // forward 1s
        case 5: CommandCoordinator("~5");  break;   // pause 1s
        case 6: CommandCoordinator(">15"); break;   // spin CW 3s (uses >)
        case 7: CommandCoordinator("~10"); break;   // pause 2s
        case 8: CommandCoordinator("<15"); break;   // spin CCW 3s (uses <)
        case 9: CommandCoordinator("~10"); break;   // pause 2s
        default:
            CommandCoordinator("~1");              // safety stop
            break;
    }

    ShowStepText(s);
}

void Shape_Init(void)
{
    step = 0;

    ChangeDisplay("Motion Demo", 0);
    ChangeDisplay("Starting...", 1);
    ChangeDisplay(" ",           2);
    ChangeDisplay(" ",           3);

    LoadStep(step);
}

void Shape_Task(void)
{
    static unsigned int last_ts = 0;

    // run once per 200ms tick
    if (Time_Sequence == last_ts)
        return;
    last_ts = Time_Sequence;

    // advance current loaded script
    if (CommandCoordinator(0))
    {
        step++;
        if (step <= 9)
        {
            LoadStep(step);
        }
        else
        {
            // done
            ChangeDisplay("Motion:",  0);
            ChangeDisplay("DONE",     1);
            ChangeDisplay("Stopped",  2);
            ChangeDisplay(" ",        3);

            WheelCommand_Tick('~');  // stop immediately
        }
    }
}
*/
