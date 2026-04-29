//===========================================================================
// File Name : line.h
//
// Description:
//   Simple line intercept / alignment sequence using existing calibration
//   globals and the new motor/display interfaces.


#ifndef LINE_H
#define LINE_H

void Line_Init(void);
void Line_Task(void);
unsigned char Line_IsDone(void);

#endif
