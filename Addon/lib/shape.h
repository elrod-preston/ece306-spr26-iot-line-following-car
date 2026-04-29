//===========================================================================
// File Name : shape.h
//
// Description:
//  Simple timed motion script driven by the NEW timer system:
//    - Uses Time_Sequence (increments every 200ms in TB0 CCR0 ISR)
//    - No SPI/LCD updates in ISRs; this module only calls ChangeDisplay()
//      from main-context (Shape_Task).
//
// Procedure:
//  1) Forward 1s, pause 1s
//  2) Reverse 2s, pause 1s
//  3) Forward 1s, pause 1s
//  4) Spin CW 3s, pause 2s
//  5) Spin CCW 3s, pause 2s
//===========================================================================

#ifndef __SHAPE_H__
#define __SHAPE_H__

void Shape_Init(void);
void Shape_Task(void);

#endif
