#ifndef DETECT_H
#define DETECT_H

//===========================================================================
// File Name : detect.h
//
// Description:
//   Simple two-sensor raw ADC detection module.
//
// What this module does:
//   - Waits for an external request flag
//   - Reads two IR detector ADC channels once
//   - Exposes only left and right raw ADC values
//   - Clears the request flag after reading
//
// Setup procedure:
//   1) Call Detect_Init() once during startup
//   2) Set detect_request = 1 whenever you want one sensor read
//   3) Call Detect_Task() repeatedly in the main loop
//   4) Read detect_left_raw and detect_right_raw after detect_request returns to 0
//===========================================================================

extern volatile unsigned int detect_left_raw;
extern volatile unsigned int detect_right_raw;

extern volatile unsigned char detect_request;

void Detect_Init(void);
void Detect_Task(void);

#endif
