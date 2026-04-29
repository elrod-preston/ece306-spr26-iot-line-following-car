//===========================================================================
// File Name : utils.h
//
// Description: Generic helper functions (simple / old compiler friendly)
//
// Author: Preston Elrod
// Date: Feb 2026
//===========================================================================

#ifndef __UTILS_H__
#define __UTILS_H__

// Returns pointer to static string buffer.
// Format: 10 characters wide, right-aligned, padded with spaces.
// Example: "       123"
char* ToString(unsigned v);
char* FloatToString(float v, unsigned width, unsigned decimals);
#endif /* __UTILS_H__ */
