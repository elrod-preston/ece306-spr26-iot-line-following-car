//===========================================================================
// File Name : macros.h
//
// Description: Common macros used by the project
//
// Author: Preston Elrod
// Date: Feb 2026
// Compiler: Built with Code Composer Version: CCS12.4.0.00007_win64
//===========================================================================

#ifndef MACROS_H
#define MACROS_H

#include "msp430.h"

/* ------------------------------------------------------------------------- */
/* General constants                                                          */
/* ------------------------------------------------------------------------- */
#define USE_GPIO                (0x00u)
#define USE_SMCLK               (0x01u)

#define ALWAYS                  (1u)
#define TRUE                    (1u)
#define FALSE                   (0u)
#define RESET_STATE             (0u)

/* ------------------------------------------------------------------------- */
/* Misc                                                                       */
/* ------------------------------------------------------------------------- */
#define P4PUD                   (P4OUT)   /* Alias used for switch pull-up */
#define RESET_REGISTER          (0x0000u)

/* ------------------------------------------------------------------------- */
/* Clock configuration                                                        */
/* NOTE:
 * This should usually be declared 'extern' here and defined once in a .c file
 * Example in some .c file:
 *    int SMCLK_CONFIG = USE_GPIO;
 */
/* ------------------------------------------------------------------------- */
extern int SMCLK_CONFIG;



/* ------------------------------------------------------------------------- */
/* Timer ID divider bits                                                      */
/* ------------------------------------------------------------------------- */

//#define ID_1                    (0x0040u) /* /2 */
//#define ID_2                    (0x0080u) /* /4 */
//#define ID_3                    (0x00C0u) /* /8 */

//#define ID__2                   (0x0040u) /* /2 */
//#define ID__4                   (0x0080u) /* /4 */
//#define ID__8                   (0x00C0u) /* /8 */

/* ------------------------------------------------------------------------- */
/* TBIDEX divider bits                                                        */
/* ------------------------------------------------------------------------- */
//#define TBIDEX_0                (0x0000u) /* Divide by 1 */
//#define TBIDEX_1                (0x0001u) /* Divide by 2 */
//#define TBIDEX_2                (0x0002u) /* Divide by 3 */
//#define TBIDEX_3                (0x0003u) /* Divide by 4 */
//#define TBIDEX_4                (0x0004u) /* Divide by 5 */
//#define TBIDEX_5                (0x0005u) /* Divide by 6 */
//#define TBIDEX_6                (0x0006u) /* Divide by 7 */
//#define TBIDEX_7                (0x0007u) /* Divide by 8 */

//#define TBIDEX__1               (0x0000u) /* Divide by 1 */
//#define TBIDEX__2               (0x0001u) /* Divide by 2 */
//#define TBIDEX__3               (0x0002u) /* Divide by 3 */
//#define TBIDEX__4               (0x0003u) /* Divide by 4 */
//#define TBIDEX__5               (0x0004u) /* Divide by 5 */
//#define TBIDEX__6               (0x0005u) /* Divide by 6 */
//#define TBIDEX__7               (0x0006u) /* Divide by 7 */
//#define TBIDEX__8               (0x0007u) /* Divide by 8 */
/* ------------------------------------------------------------------------- */
/* Time sequence macros                                                       */
/* ------------------------------------------------------------------------- */
#define Time_Sequence_Rate      (50u)                  /* 50 ms base */
#define S1250                   (1250u / Time_Sequence_Rate)
#define S1000                   (1000u / Time_Sequence_Rate)
#define S750                    (750u  / Time_Sequence_Rate)
#define S500                    (500u  / Time_Sequence_Rate)
#define S250                    (250u  / Time_Sequence_Rate)

/* ------------------------------------------------------------------------- */
/* TB0 timing intervals
 *
 * TB0 clock = 8MHz / 8 / 8 = 125kHz
 * 1 tick = 8us
 * ------------------------------------------------------------------------- */
#define TB0CCR0_INTERVAL        (6250u)   /* 50ms = 6250 counts */
#define TB0CCR1_INTERVAL        (2500u)   /* 20ms = 2500 counts */
#define TB0CCR2_INTERVAL_1MS    (125u)    /* 1ms  = 125 counts */

#define FIFTY_MS_COUNT          (50u)

/* ------------------------------------------------------------------------- */
/* Motor PWM settings
 *
 * TB3 clock = 8MHz / 1 / 1 = 8MHz
 * PWM frequency = 8,000,000 / WHEEL_PERIOD
 * ------------------------------------------------------------------------- */
#define WHEEL_OFF               (0u)
#define WHEEL_FULL              (WHEEL_PERIOD)

#endif /* MACROS_H */
