// ------------------------------------------------------------------------------
//
//  Description: Clock Initialization
//
//  MSP430FR2355
//  MCLK  = 16MHz
//  SMCLK = 8MHz
//  ACLK  = XT1CLK = 32768Hz
//
//  FLL reference = REFOCLK for stable DCO trim testing
// ------------------------------------------------------------------------------

#include <Core\lib\functions.h>
#include <Core\lib\macros.h>
#include "msp430.h"

// MACROS========================================================================
#define MCLK_FREQ_MHZ      (16)
#define CLEAR_REGISTER     (0X0000)

void Init_Clocks(void);
void Software_Trim(void);

void Init_Clocks(void)
{
  WDTCTL = WDTPW | WDTHOLD;

  // Required for MSP430FR2355 when MCLK is above 8MHz.
  FRCTL0 = FRCTLPW | NWAITS_1;

  // Clear oscillator fault flags.
  do {
    CSCTL7 &= ~(XT1OFFG | DCOFFG);
    SFRIFG1 &= ~OFIFG;
  } while (SFRIFG1 & OFIFG);

  // Disable FLL while configuring it.
  __bis_SR_register(SCG0);

  CSCTL0 = CLEAR_REGISTER;

  // 16MHz DCO range.
  // TI example uses:
  // DCOFTRIMEN_1 | DCOFTRIM0 | DCOFTRIM1 | DCORSEL_5
  CSCTL1 = DCOFTRIMEN_1 |
           DCOFTRIM0   |
           DCOFTRIM1   |
           DCORSEL_5;

  // 32768Hz * (487 + 1) = 15.990784MHz
  CSCTL2 = FLLD_0 | 487;

  // Use REFOCLK for FLL reference during testing.
  // This matches TI's common example style and removes XT1 startup/stability
  // from the DCO trim path.
  CSCTL3 = SELREF__REFOCLK;

  __delay_cycles(3);

  // Enable FLL.
  __bic_SR_register(SCG0);

  Software_Trim();

  // ACLK can still be XT1.
  // MCLK and SMCLK use DCOCLKDIV.
  CSCTL4 = SELA__XT1CLK | SELMS__DCOCLKDIV;

  // MCLK = 16MHz, SMCLK = 8MHz.
  // Assignment is important. Do not use |= here.
  CSCTL5 = DIVM__1 | DIVS__2;

  // Small settle time before UART/IOT starts.
  __delay_cycles(160000);   // about 10ms at 16MHz

  PM5CTL0 &= ~LOCKLPM5;
}

void Software_Trim(void)
{
  unsigned int oldDcoTap = 0xffff;
  unsigned int newDcoTap = 0xffff;
  unsigned int newDcoDelta = 0xffff;
  unsigned int bestDcoDelta = 0xffff;
  unsigned int csCtl0Copy = 0;
  unsigned int csCtl1Copy = 0;
  unsigned int csCtl0Read = 0;
  unsigned int csCtl1Read = 0;
  unsigned int dcoFreqTrim = 3;
  unsigned char endLoop = 0;

  do {
    CSCTL0 = 0x100;                         // DCO tap = 256

    do {
      CSCTL7 &= ~DCOFFG;
    } while (CSCTL7 & DCOFFG);

    // TI recommends waiting for FLLUNLOCK to stabilize before polling.
    __delay_cycles((unsigned int)3000 * MCLK_FREQ_MHZ);

    while ((CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)) &&
           ((CSCTL7 & DCOFFG) == 0));

    csCtl0Read = CSCTL0;
    csCtl1Read = CSCTL1;

    oldDcoTap = newDcoTap;
    newDcoTap = csCtl0Read & 0x01ff;
    dcoFreqTrim = (csCtl1Read & 0x0070) >> 4;

    if (newDcoTap < 256) {
      newDcoDelta = 256 - newDcoTap;

      if ((oldDcoTap != 0xffff) &&
          (oldDcoTap >= 256)) {
        endLoop = 1;
      } else {
        dcoFreqTrim--;
        CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim << 4);
      }
    } else {
      newDcoDelta = newDcoTap - 256;

      if (oldDcoTap < 256) {
        endLoop = 1;
      } else {
        dcoFreqTrim++;
        CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim << 4);
      }
    }

    if (newDcoDelta < bestDcoDelta) {
      csCtl0Copy = csCtl0Read;
      csCtl1Copy = csCtl1Read;
      bestDcoDelta = newDcoDelta;
    }

  } while (endLoop == 0);

  CSCTL0 = csCtl0Copy;
  CSCTL1 = csCtl1Copy;

  while (CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));
}
