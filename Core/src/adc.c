//===========================================================================
// File Name : adc.c
//
// Description: Simple blocking ADC driver
//              Uses AVCC as reference
//              Returns raw value or millivolts
//
//
// Author: Preston Elrod
// Date: Feb 2026
//===========================================================================

#include <msp430.h>
#include "Core\lib\ports.h"
#include "Core\lib\adc.h"

//===========================================================================
// Init_ADC
// Configure ADC for 12-bit single channel conversion
//===========================================================================
void Init_ADC(void)
{
    // --- Configure Analog Pins ---
    P1DIR &= ~(V_DETECT_L | V_DETECT_R | V_THUMB);
    P1OUT &= ~(V_DETECT_L | V_DETECT_R | V_THUMB);
    P1SELC |=  (V_DETECT_L | V_DETECT_R | V_THUMB);


    // --- Disable ADC before config ---
    ADCCTL0 &= ~ADCENC;

    // --- ADCCTL0 ---
    ADCCTL0 = 0;
    ADCCTL0 |= ADCSHT_2;     // 16 clock sample time
    ADCCTL0 |= ADCON;        // ADC ON

    // --- ADCCTL1 ---
    ADCCTL1 = 0;
    ADCCTL1 |= ADCSHS_0;     // trigger from ADCSC
    ADCCTL1 |= ADCSHP;       // use sampling timer
    ADCCTL1 |= ADCDIV_0;     // clock divide /1
    ADCCTL1 |= ADCSSEL_0;    // MODCLK
    ADCCTL1 |= ADCCONSEQ_0;  // single-channel, single-conversion

    // --- ADCCTL2 ---
    ADCCTL2 = 0;
    ADCCTL2 |= ADCPDIV0;     // pre-divider /1
    ADCCTL2 |= ADCRES_1;     // 10-bit resolution
    ADCCTL2 &= ~ADCDF;       // unsigned
    ADCCTL2 &= ~ADCSR;       // <=200ksps

    // --- Memory Control ---
    ADCMCTL0 = 0;
    ADCMCTL0 |= ADCSREF_0;   // VR+ = AVCC, VR- = AVSS
}

//===========================================================================
// ADC_ReadChannel
// Blocking read of selected channel
// Returns raw 12-bit value (0–4095)
//===========================================================================
unsigned ADC_ReadChannel(unsigned channel)
{
    unsigned value;

    ADCCTL0 &= ~ADCENC;          // allow channel change

    ADCMCTL0 &= ~ADCINCH_15;     // clear channel field
    ADCMCTL0 |= channel;         // set desired channel

    ADCCTL0 |= ADCENC;
    ADCCTL0 |= ADCSC;            // start conversion

    while (ADCCTL1 & ADCBUSY);   // wait until complete

    value = ADCMEM0;



    return value;
}

//===========================================================================
// ADC_ReadMilliVolts
// Returns millivolts.
// Normal channels: millivolts at ADC pin
// Battery channel (ADC_CH_BAT): returns corrected VBAT millivolts
//
// Formula at pin: Vnode = (ADC * Vref) / 4095
// Divider: VBAT = Vnode * ((RTOP + RBOT) / RBOT)
//===========================================================================
unsigned ADC_ReadMilliVolts(unsigned channel)
{
    unsigned long mv;

    mv = (unsigned long)ADC_ReadChannel(channel);
    mv = mv * (unsigned long)ADC_VREF_MV;
    mv = mv / 1024UL;  // 10-bit  scale

    // Special case: battery divider correction
    if (channel == (unsigned)ADC_CH_BAT) {
        mv = mv * (VBAT_DIV_RTOP_OHMS + VBAT_DIV_RBOT_OHMS);
        mv = mv / VBAT_DIV_RBOT_OHMS;
    }

    return (unsigned)mv;
}
