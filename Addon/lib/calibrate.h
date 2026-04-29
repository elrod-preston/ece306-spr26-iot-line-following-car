#ifndef CALIBRATE_H
#define CALIBRATE_H

//---------------------------------------------------------------------------
// Calibration state values
//---------------------------------------------------------------------------
#define CAL_ST_WHITE   (0u)
#define CAL_ST_BLACK   (1u)
#define CAL_ST_READY   (2u)
#define CAL_ST_DONE    (3u)

//---------------------------------------------------------------------------
// Exposed calibration globals
//---------------------------------------------------------------------------
extern volatile unsigned int calibrate_white_left;
extern volatile unsigned int calibrate_white_right;
extern volatile unsigned int calibrate_black_left;
extern volatile unsigned int calibrate_black_right;

extern volatile unsigned char calibrate_have_white;
extern volatile unsigned char calibrate_have_black;
extern volatile unsigned char calibrate_done;
extern volatile unsigned int calibrate_state;

//---------------------------------------------------------------------------
// One-time setup when entering calibration state
//---------------------------------------------------------------------------
void Calibrate_Init(void);

//---------------------------------------------------------------------------
// Run one calibration tick
//---------------------------------------------------------------------------
void Calibrate_Task(void);

//---------------------------------------------------------------------------
// Returns 1 when calibration is complete
//---------------------------------------------------------------------------
unsigned char Calibrate_IsDone(void);

#endif
