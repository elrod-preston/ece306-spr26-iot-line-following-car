//===========================================================================
// File Name : ports.h
//
// Description: This file contains the macros used for ports
//
// Author: Preston Elrod
// Date: Feb 2026
// Compiler: Built with Code Composer Version: CCS12.4.0.00007_win64
//===========================================================================



//------------------------------------------------------------------------------
// Port 1 Pins
#define RED_LED     (0x01) // P1.0 RedLED
#define A1_SEEED    (0x02) // P1.1 A1_SEEED
#define V_DETECT_L  (0x08) // P1.3 V_DETECT_L
#define V_DETECT_R  (0x04) // P1.2 V_DETECT_R
#define A4_SEEED    (0x10) // P1.4 A4_SEEED
#define V_THUMB     (0x20) // P1.5 V_THUMB
#define UCA0RXD     (0x40) // P1.6 UCA0RXD
#define UCA0TXD     (0x80) // P1.7 UCA0TXD


//------------------------------------------------------------------------------
// Port 2 Pins
#define SLOW_CLK    (0x01) // P2.0 SLOW_CLK
#define CHECK_BAT   (0x02) // P2.1 CHECK_BAT
#define IR_LED      (0x04) // P2.2 IR_LED
#define SW2         (0x08) // P2.3 SW2
#define IOT_RUN_RED (0x10) // P2.4 IOT_RUN_CPU
#define DAC_ENB     (0x20) // P2.5 DAC_ENB
#define LFXOUT      (0x40) // P2.6 LFXOUT
#define LFXIN       (0x80) // P2.7 LFXIN


//------------------------------------------------------------------------------
// Port 3 Pins


#define TEST_PROBE   (0x01) // P3.0 TEST_PROBE
#define OA2O         (0x02) // P3.1 OA2O
#define OA2N         (0x04) // P3.2 OA2-
#define OA2P         (0x08) // P3.3 OA2+
#define SMCLK        (0x10) // P3.4 SMCLK
#define DAC_CNTL     (0x20) // P3.5 DAC_CNTL
#define IOT_LINK_GRN (0x40) // P3.6 IOT_LINK_CPU
#define IOT_EN       (0x80) // P3.7 IOT_EN_CPU


//------------------------------------------------------------------------------
// Port 4 Pins
#define RESET_LCD   (0x01) // P4.0 RESET_LCD
#define SW1         (0x02) // P4.1 SW1
#define UCA1RXD     (0x04) // P4.2 UCA1RXD
#define UCA1TXD     (0x08) // P4.3 UCA1TXD
#define UCB1_CS_LCD (0x10) // P4.4 UCB1_CS_LCD
#define UCB1CLK     (0x20) // P4.5 UCB1CLK
#define UCB1SIMO    (0x40) // P4.6 UCB1SIMO
#define UCB1SOMI    (0x80) // P4.7 UCB1SOMI


//------------------------------------------------------------------------------
// Port 5 Pins
#define V_BAT        (0x01) // P5.0 V_BAT
#define V_5_0        (0x02) // P5.1 V_5_0
#define V_DAC        (0x04) // P5.2 V_DAC
#define V_3_3        (0x08) // P5.3 V_3_3
#define IOT_BOOT_CPU (0x10) // P5.4 IOT_BOOT_CPU


//------------------------------------------------------------------------------
// Port 6 Pins
#define LCD_BACKLITE (0x01) // P6.0 LCD_BACKLITE
#define R_FORWARD    (0x04) // P6.1 R_FORWARD
#define L_FORWARD    (0x02) // P6.2 L_FORWARD
#define R_REVERSE    (0x10) // P6.3 R_REVERSE
#define L_REVERSE    (0x08) // P6.4 L_REVERSE
#define P6_5         (0x20) // P6.5 P6_5
#define GRN_LED      (0x40) // P6.6 GRN_LED
//------------------------------------------------------------------------------
