#ifndef COMMAND_H
#define COMMAND_H

//===========================================================================
// File Name : command.h
//
// Project 10 command coordinator.
//
// App buttons:
//   RST     -> reset course display/timer
//   F       -> forward hold
//   B       -> backward hold
//   L       -> left turn hold
//   R       -> right turn hold
//   STOP    -> emergency stop
//   SL      -> slow left correction
//   SR      -> slow right correction
//   BLKFL   -> start black-line autonomous mode, left intercept assumed
//   EXIT    -> exit circle and stop
//
// Typed commands:
//   P0      -> clear arrived display
//   P1-P7   -> display Arrived 01 through Arrived 07
//   P8      -> mark pad 8 / clear arrived display
//===========================================================================

void CommandCoordinator_Init(void);
void CommandCoordinator_Task(void);
void CommandCoordinator_LoadCommand(char *cmd);
void CommandCoordinator_Stop(void);
void CommandCoordinator_ResetCourse(void);
void CommandCoordinator_SetIDLines(char *line1, char *line2);
unsigned char CommandCoordinator_IsBusy(void);

#endif
