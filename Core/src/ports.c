//===========================================================================
// File Name : ports.c
//
// Description: This file configures all of the ports on the board
//
// Author: Preston Elrod
// Date: Feb 2026
// Compiler: Built with Code Composer Version: CCS12.4.0.00007_win64
//===========================================================================
//------------------------------------------------------------------------------

#include <Core\lib\ports.h>
#include <Core\lib\macros.h>
#include "msp430.h"

//------------------------------------------------------------------------------

void Init_Port1(void){ // Configure Port 1
//------------------------------------------------------------------------------
 P1OUT = 0x00;                 // Clear ALL P1 outputs low (affects any P1 pins configured as outputs)
 P1DIR = 0x00;                 // Set ALL P1 pins as inputs initially (direction default)

 P1SEL0 &= ~RED_LED;           // Select GPIO function for P1.0 (RED_LED) by clearing SEL0 bit
 P1SEL1 &= ~RED_LED;           // Select GPIO function for P1.0 (RED_LED) by clearing SEL1 bit
 P1OUT  &= ~RED_LED;           // Drive P1.0 (RED_LED) low (LED off)
 P1DIR  |=  RED_LED;           // Set P1.0 (RED_LED) as output

 P1SEL0 |=  A1_SEEED;          // Select ADC/analog function for P1.1 (A1_SEEED) by setting SEL0
 P1SEL1 |=  A1_SEEED;          // Select ADC/analog function for P1.1 (A1_SEEED) by setting SEL1
 P1DIR  &= ~A1_SEEED;          // Ensure P1.1 (A1_SEEED) is input

 P1SEL0 |=  V_DETECT_L;        // Select ADC/analog function for P1.2 (V_DETECT_L) by setting SEL0
 P1SEL1 |=  V_DETECT_L;        // Select ADC/analog function for P1.2 (V_DETECT_L) by setting SEL1
 P1DIR  &= ~V_DETECT_L;        // Ensure P1.2 (V_DETECT_L) is input

 P1SEL0 |=  V_DETECT_R;        // Select ADC/analog function for P1.3 (V_DETECT_R) by setting SEL0
 P1SEL1 |=  V_DETECT_R;        // Select ADC/analog function for P1.3 (V_DETECT_R) by setting SEL1
 P1DIR  &= ~V_DETECT_R;        // Ensure P1.3 (V_DETECT_R) is input

 P1SEL0 |=  A4_SEEED;          // Select ADC/analog function for P1.4 (A4_SEEED) by setting SEL0
 P1SEL1 |=  A4_SEEED;          // Select ADC/analog function for P1.4 (A4_SEEED) by setting SEL1
 P1DIR  &= ~A4_SEEED;          // Ensure P1.4 (A4_SEEED) is input

 P1SEL0 |=  V_THUMB;           // Select ADC/analog function for P1.5 (V_THUMB) by setting SEL0
 P1SEL1 |=  V_THUMB;           // Select ADC/analog function for P1.5 (V_THUMB) by setting SEL1
 P1DIR  &= ~V_THUMB;           // Ensure P1.5 (V_THUMB) is input

 P1SEL0 |=  UCA0RXD;           // Select UART function for P1.6 (UCA0RXD) by setting SEL0
 P1SEL1 &= ~UCA0RXD;           // Select UART function for P1.6 (UCA0RXD) by clearing SEL1
 P1DIR  &= ~UCA0RXD;           // Ensure P1.6 (UCA0RXD) is input (UART RX from IOT)

 P1SEL0 |=  UCA0TXD;           // Select UART function for P1.7 (UCA0TXD) by setting SEL0
 P1SEL1 &= ~UCA0TXD;           // Select UART function for P1.7 (UCA0TXD) by clearing SEL1
 P1DIR  |=  UCA0TXD;           // Set P1.7 (UCA0TXD) as output (UART TX to IOT)
//------------------------------------------------------------------------------
}



void Init_Port2(void){ // Configure Port 2
//------------------------------------------------------------------------------
 P2OUT = 0x00;                 // Clear ALL P2 outputs low (affects any P2 pins configured as outputs)
 P2DIR = 0x00;                 // Set ALL P2 pins as inputs initially (direction default)

 P2SEL0 &= ~RESET_LCD;         // Select GPIO for P2.0 (RESET_LCD) by clearing SEL0
 P2SEL1 &= ~RESET_LCD;         // Select GPIO for P2.0 (RESET_LCD) by clearing SEL1
 P2OUT  &= ~RESET_LCD;         // Drive P2.0 (RESET_LCD) low initially
 P2DIR  |=  RESET_LCD;         // Set P2.0 (RESET_LCD) as output

 P2SEL0 &= ~CHECK_BAT;         // Select GPIO for P2.1 (CHECK_BAT) by clearing SEL0
 P2SEL1 &= ~CHECK_BAT;         // Select GPIO for P2.1 (CHECK_BAT) by clearing SEL1
 P2OUT  |=  CHECK_BAT;         // Drive P2.1 (CHECK_BAT) high initially
 P2DIR  |=  CHECK_BAT;         // Set P2.1 (CHECK_BAT) as output

 P2SEL0 &= ~IR_LED;            // Select GPIO for P2.2 (IR_LED) by clearing SEL0
 P2SEL1 &= ~IR_LED;            // Select GPIO for P2.2 (IR_LED) by clearing SEL1
 P2OUT  &= ~IR_LED;            // Drive P2.2 (IR_LED) low (IR LED off)
 P2DIR  |=  IR_LED;            // Set P2.2 (IR_LED) as output

 // SW2 interrupt setup

 P2SEL0 &= ~SW2;               // Select GPIO for P2.3 (SW2) by clearing SEL0
 P2SEL1 &= ~SW2;               // Select GPIO for P2.3 (SW2) by clearing SEL1
 P2OUT  |=  SW2;               // Set P2.3 output high to choose pull-up when REN enabled
 P2DIR  &= ~SW2;               // Set P2.3 (SW2) as input
 P2REN  |=  SW2;               // Enable resistor on P2.3 (SW2) (with OUT=1 => pull-up)

 P2IE   &= ~SW2;               // Disable SW2 interrupt during config
 P2IFG  &= ~SW2;               // Clear any pending IFG first

 P2IES  |=  SW2;               // Set P2.3 (SW2) for high-to-low edge interrupt (press = 1->0)

 (void)P2IN;                   // Dummy read to let input settle
 P2IFG  &= ~SW2;               // Clear IFG again after edge/pull setup
 P2IE   |=  SW2;               // Enable interrupt on P2.3 (SW2)

 P2SEL0 &= ~IOT_RUN_RED;       // Select GPIO for P2.4 (IOT_RUN_RED) by clearing SEL0
 P2SEL1 &= ~IOT_RUN_RED;       // Select GPIO for P2.4 (IOT_RUN_RED) by clearing SEL1
 P2OUT  &= ~IOT_RUN_RED;       // Drive P2.4 (IOT_RUN_RED) low initially
 P2DIR  |=  IOT_RUN_RED;       // Set P2.4 (IOT_RUN_RED) as output

 P2SEL0 &= ~DAC_ENB;           // Select GPIO for P2.5 (DAC_ENB) by clearing SEL0
 P2SEL1 &= ~DAC_ENB;           // Select GPIO for P2.5 (DAC_ENB) by clearing SEL1
 P2OUT  |=  DAC_ENB;           // Drive P2.5 (DAC_ENB) high initially
 P2DIR  |=  DAC_ENB;           // Set P2.5 (DAC_ENB) as output

 P2SEL0 &= ~LFXOUT;            // Select LFXT function for P2.6 (LFXOUT)
 P2SEL1 |=  LFXOUT;            // Select LFXT function for P2.6 (LFXOUT)

 P2SEL0 &= ~LFXIN;             // Select LFXT function for P2.7 (LFXIN)
 P2SEL1 |=  LFXIN;             // Select LFXT function for P2.7 (LFXIN)
//------------------------------------------------------------------------------
}



void Init_Port3(void){ // Configure Port 3
//------------------------------------------------------------------------------
 P3OUT = 0x00;                 // Clear ALL P3 outputs low (affects any P3 pins configured as outputs)
 P3DIR = 0x00;                 // Set ALL P3 pins as inputs initially (direction default)

 P3SEL0 &= ~TEST_PROBE;        // Select GPIO for P3.0 (TEST_PROBE) by clearing SEL0
 P3SEL1 &= ~TEST_PROBE;        // Select GPIO for P3.0 (TEST_PROBE) by clearing SEL1
 P3DIR  &= ~TEST_PROBE;        // Ensure P3.0 (TEST_PROBE) is input

 P3SEL0 |=  OA2O;              // Select analog function for P3.1 (OA2O) by setting SEL0
 P3SEL1 |=  OA2O;              // Select analog function for P3.1 (OA2O) by setting SEL1
 P3DIR  &= ~OA2O;              // Ensure P3.1 (OA2O) is input

 P3SEL0 |=  OA2N;              // Select analog function for P3.2 (OA2N) by setting SEL0
 P3SEL1 |=  OA2N;              // Select analog function for P3.2 (OA2N) by setting SEL1
 P3DIR  &= ~OA2N;              // Ensure P3.2 (OA2N) is input

 P3SEL0 |=  OA2P;              // Select analog function for P3.3 (OA2P) by setting SEL0
 P3SEL1 |=  OA2P;              // Select analog function for P3.3 (OA2P) by setting SEL1
 P3DIR  &= ~OA2P;              // Ensure P3.3 (OA2P) is input

 P3SEL0 |=  SMCLK;             // Select clock function for P3.4 (SMCLK) by setting SEL0
 P3SEL1 &= ~SMCLK;             // Select clock function for P3.4 (SMCLK) by clearing SEL1
 P3DIR  |=  SMCLK;             // Set P3.4 (SMCLK) as output

 P3SEL0 &= ~DAC_CNTL;          // Select GPIO for P3.5 (DAC_CNTL) by clearing SEL0
 P3SEL1 &= ~DAC_CNTL;          // Select GPIO for P3.5 (DAC_CNTL) by clearing SEL1
 P3OUT  &= ~DAC_CNTL;          // Drive P3.5 (DAC_CNTL) low initially
 P3DIR  |=  DAC_CNTL;          // Set P3.5 (DAC_CNTL) as output

 P3SEL0 &= ~IOT_LINK_GRN;      // Select GPIO for P3.6 (IOT_LINK_GRN) by clearing SEL0
 P3SEL1 &= ~IOT_LINK_GRN;      // Select GPIO for P3.6 (IOT_LINK_GRN) by clearing SEL1
 P3OUT  &= ~IOT_LINK_GRN;      // Drive P3.6 (IOT_LINK_GRN) low initially
 P3DIR  |=  IOT_LINK_GRN;      // Set P3.6 (IOT_LINK_GRN) as output

 P3SEL0 &= ~IOT_EN;            // Select GPIO for P3.7 (IOT_EN) by clearing SEL0
 P3SEL1 &= ~IOT_EN;            // Select GPIO for P3.7 (IOT_EN) by clearing SEL1
 P3OUT  &= ~IOT_EN;            // Drive P3.7 (IOT_EN) low initially to hold IOT in reset
 P3DIR  |=  IOT_EN;            // Set P3.7 (IOT_EN) as output
//------------------------------------------------------------------------------
}



void Init_Port4(void){ // Configure Port 4
//------------------------------------------------------------------------------
 P4OUT = 0x00;                 // Clear ALL P4 outputs low (affects any P4 pins configured as outputs)
 P4DIR = 0x00;                 // Set ALL P4 pins as inputs initially (direction default)

 P4SEL0 &= ~RESET_LCD;         // Select GPIO for P4.0 (RESET_LCD) by clearing SEL0
 P4SEL1 &= ~RESET_LCD;         // Select GPIO for P4.0 (RESET_LCD) by clearing SEL1
 P4OUT  |=  RESET_LCD;         // Drive P4.0 (RESET_LCD) high
 P4DIR  |=  RESET_LCD;         // Set P4.0 (RESET_LCD) as output

 // SW1 interrupt setup

 P4IE   &= ~SW1;               // Disable SW1 interrupt during config
 P4IFG  &= ~SW1;               // Clear any pending IFG first

 P4SEL0 &= ~SW1;               // Select GPIO for P4.1 (SW1) by clearing SEL0
 P4SEL1 &= ~SW1;               // Select GPIO for P4.1 (SW1) by clearing SEL1
 P4DIR  &= ~SW1;               // Set P4.1 (SW1) as input

 P4OUT  |=  SW1;               // Set P4.1 output high to choose pull-up when REN enabled
 P4REN  |=  SW1;               // Enable resistor on P4.1 (SW1) (with OUT=1 => pull-up)

 P4IES  |=  SW1;               // Set P4.1 (SW1) for high-to-low edge interrupt (press = 1->0)

 (void)P4IN;                   // Dummy read to let input settle
 P4IFG  &= ~SW1;               // Clear IFG again after edge/pull setup
 P4IE   |=  SW1;               // Enable interrupt on P4.1 (SW1)

 P4SEL0 |=  UCA1RXD;           // Select UART function for P4.2 (UCA1RXD) by setting SEL0
 P4SEL1 &= ~UCA1RXD;           // Select UART function for P4.2 (UCA1RXD) by clearing SEL1
 P4DIR  &= ~UCA1RXD;           // Ensure P4.2 (UCA1RXD) is input (UART RX)

 P4SEL0 |=  UCA1TXD;           // Select UART function for P4.3 (UCA1TXD) by setting SEL0
 P4SEL1 &= ~UCA1TXD;           // Select UART function for P4.3 (UCA1TXD) by clearing SEL1
 P4DIR  |=  UCA1TXD;           // Set P4.3 (UCA1TXD) as output (UART TX)

 P4SEL0 &= ~UCB1_CS_LCD;       // Select GPIO for P4.4 (UCB1_CS_LCD) by clearing SEL0
 P4SEL1 &= ~UCB1_CS_LCD;       // Select GPIO for P4.4 (UCB1_CS_LCD) by clearing SEL1
 P4OUT  |=  UCB1_CS_LCD;       // Drive P4.4 (UCB1_CS_LCD) high (CS inactive)
 P4DIR  |=  UCB1_CS_LCD;       // Set P4.4 (UCB1_CS_LCD) as output

 P4SEL0 |=  UCB1CLK;           // Select SPI function for P4.5 (UCB1CLK) by setting SEL0
 P4SEL1 &= ~UCB1CLK;           // Select SPI function for P4.5 (UCB1CLK) by clearing SEL1

 P4SEL0 |=  UCB1SIMO;          // Select SPI function for P4.6 (UCB1SIMO) by setting SEL0
 P4SEL1 &= ~UCB1SIMO;          // Select SPI function for P4.6 (UCB1SIMO) by clearing SEL1

 P4SEL0 |=  UCB1SOMI;          // Select SPI function for P4.7 (UCB1SOMI) by setting SEL0
 P4SEL1 &= ~UCB1SOMI;          // Select SPI function for P4.7 (UCB1SOMI) by clearing SEL1
//------------------------------------------------------------------------------
}



//------------------------------------------------------------------------------
// Port 5 init
void Init_Port5(void){ // Configure Port 5
//------------------------------------------------------------------------------
 P5OUT = 0x00;                 // Clear ALL P5 outputs low (affects any P5 pins configured as outputs)
 P5DIR = 0x00;                 // Set ALL P5 pins as inputs initially (direction default)

 P5SEL0 |=  V_BAT;             // Select analog function for P5.0 (V_BAT) by setting SEL0
 P5SEL1 |=  V_BAT;             // Select analog function for P5.0 (V_BAT) by setting SEL1
 P5DIR  &= ~V_BAT;             // Ensure P5.0 (V_BAT) is input

 P5SEL0 |=  V_5_0;             // Select analog function for P5.1 (V_5_0) by setting SEL0
 P5SEL1 |=  V_5_0;             // Select analog function for P5.1 (V_5_0) by setting SEL1
 P5DIR  &= ~V_5_0;             // Ensure P5.1 (V_5_0) is input

 P5SEL0 |=  V_DAC;             // Select analog function for P5.2 (V_DAC) by setting SEL0
 P5SEL1 |=  V_DAC;             // Select analog function for P5.2 (V_DAC) by setting SEL1
 P5DIR  &= ~V_DAC;             // Ensure P5.2 (V_DAC) is input

 P5SEL0 |=  V_3_3;             // Select analog function for P5.3 (V_3_3) by setting SEL0
 P5SEL1 |=  V_3_3;             // Select analog function for P5.3 (V_3_3) by setting SEL1
 P5DIR  &= ~V_3_3;             // Ensure P5.3 (V_3_3) is input

 P5SEL0 &= ~IOT_BOOT_CPU;      // Select GPIO for P5.4 (IOT_BOOT_CPU) by clearing SEL0
 P5SEL1 &= ~IOT_BOOT_CPU;      // Select GPIO for P5.4 (IOT_BOOT_CPU) by clearing SEL1
 P5OUT  |=  IOT_BOOT_CPU;      // Drive P5.4 (IOT_BOOT_CPU) high for normal run mode
 P5DIR  |=  IOT_BOOT_CPU;      // Set P5.4 (IOT_BOOT_CPU) as output
//------------------------------------------------------------------------------
}



//------------------------------------------------------------------------------
// Port 6 init
void Init_Port6(void){ // Configure Port 6
//------------------------------------------------------------------------------
 P6OUT = 0x00;                 // Clear ALL P6 outputs low (affects any P6 pins configured as outputs)
 P6DIR = 0x00;                 // Set ALL P6 pins as inputs initially (direction default)

 P6SEL0 &= ~LCD_BACKLITE;      // Select GPIO for P6.0 (LCD_BACKLITE) by clearing SEL0
 P6SEL1 &= ~LCD_BACKLITE;      // Select GPIO for P6.0 (LCD_BACKLITE) by clearing SEL1
 P6OUT  &= ~LCD_BACKLITE;      // Drive P6.0 (LCD_BACKLITE) low initially
 P6DIR  |=  LCD_BACKLITE;      // Set P6.0 (LCD_BACKLITE) as output

 P6SEL0 |=  R_FORWARD;         // Select TB3.2 function for P6.1 (R_FORWARD) by setting SEL0
 P6SEL1 &= ~R_FORWARD;         // Select TB3.2 function for P6.1 (R_FORWARD) by clearing SEL1
 P6OUT  &= ~R_FORWARD;         // Drive P6.1 (R_FORWARD) low initially
 P6DIR  |=  R_FORWARD;         // Set P6.1 (R_FORWARD) as output

 P6SEL0 |=  L_FORWARD;         // Select TB3.3 function for P6.2 (L_FORWARD) by setting SEL0
 P6SEL1 &= ~L_FORWARD;         // Select TB3.3 function for P6.2 (L_FORWARD) by clearing SEL1
 P6OUT  &= ~L_FORWARD;         // Drive P6.2 (L_FORWARD) low initially
 P6DIR  |=  L_FORWARD;         // Set P6.2 (L_FORWARD) as output

 P6SEL0 |=  R_REVERSE;         // Select TB3.4 function for P6.3 (R_REVERSE) by setting SEL0
 P6SEL1 &= ~R_REVERSE;         // Select TB3.4 function for P6.3 (R_REVERSE) by clearing SEL1
 P6OUT  &= ~R_REVERSE;         // Drive P6.3 (R_REVERSE) low initially
 P6DIR  |=  R_REVERSE;         // Set P6.3 (R_REVERSE) as output

 P6SEL0 |=  L_REVERSE;         // Select TB3.5 function for P6.4 (L_REVERSE) by setting SEL0
 P6SEL1 &= ~L_REVERSE;         // Select TB3.5 function for P6.4 (L_REVERSE) by clearing SEL1
 P6OUT  &= ~L_REVERSE;         // Drive P6.4 (L_REVERSE) low initially
 P6DIR  |=  L_REVERSE;         // Set P6.4 (L_REVERSE) as output

 P6SEL0 &= ~P6_5;              // Select GPIO for P6.5 (P6_5) by clearing SEL0
 P6SEL1 &= ~P6_5;              // Select GPIO for P6.5 (P6_5) by clearing SEL1
 P6DIR  &= ~P6_5;              // Ensure P6.5 (P6_5) is input

 P6SEL0 &= ~GRN_LED;           // Select GPIO for P6.6 (GRN_LED) by clearing SEL0
 P6SEL1 &= ~GRN_LED;           // Select GPIO for P6.6 (GRN_LED) by clearing SEL1
 P6OUT  &= ~GRN_LED;           // Drive P6.6 (GRN_LED) low (LED off)
 P6DIR  |=  GRN_LED;           // Set P6.6 (GRN_LED) as output
//------------------------------------------------------------------------------
}



void Init_Ports(void)
{
  Init_Port1();
  Init_Port2();
  Init_Port3();
  Init_Port4();
  Init_Port5();
  Init_Port6();
}
