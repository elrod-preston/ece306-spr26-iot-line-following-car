#ifndef IOT_H
#define IOT_H

//===========================================================================
// File Name : iot.h
//
// Description:
//   Public interface and constants for simplified ESP AT IOT driver.
//
// Notes:
//   - No command queue.
//   - Latest authorized command is accessed with:
//       IOT_MessageAvailable()
//       IOT_GetMessage()
//       IOT_ClearMessage()
//   - IOT driver owns LCD only before authorization.
//===========================================================================

//---------------------------------------------------------------------------
// Serial dependency
//---------------------------------------------------------------------------

#include "Core\\lib\\serial.h"

//---------------------------------------------------------------------------
// Wi-Fi credentials
//---------------------------------------------------------------------------
// Change these in this file, or define them somewhere before including iot.h.
//---------------------------------------------------------------------------

#ifndef IOT_WIFI1_SSID
#define IOT_WIFI1_SSID      "ncsu"
#endif

#ifndef IOT_WIFI1_PASSWORD
#define IOT_WIFI1_PASSWORD  ""
#endif

#ifndef IOT_WIFI2_SSID
#define IOT_WIFI2_SSID      "REMOVED"
#endif

#ifndef IOT_WIFI2_PASSWORD
#define IOT_WIFI2_PASSWORD  "REMOVED"
#endif

//---------------------------------------------------------------------------
// App authorization
//---------------------------------------------------------------------------

#ifndef IOT_AUTH_PASS
#define IOT_AUTH_PASS       "REMOVED"
#endif

//---------------------------------------------------------------------------
// TCP server settings
//---------------------------------------------------------------------------

#define IOT_SERVER_PORT     (8888u)
#define IOT_PORT_TEXT       ":8888"

//---------------------------------------------------------------------------
// Buffer sizes
//---------------------------------------------------------------------------

#define IOT_LINE_MAX        (300u)
#define IOT_MSG_MAX         (300u)
#define IOT_CMD_MAX         (96u)

//---------------------------------------------------------------------------
// LCD settings
//---------------------------------------------------------------------------

#define IOT_LCD_LINE_LEN    (10u)
#define IOT_LCD_ROW         (3u)

//---------------------------------------------------------------------------
// Timing settings in milliseconds
//---------------------------------------------------------------------------

#define IOT_RESET_HOLD_MS       (150u)
#define IOT_BOOT_WAIT_MS        (3000u)
#define IOT_CMD_TIMEOUT_MS      (2000u)
#define IOT_JOIN_TIMEOUT_MS     (15000u)
#define IOT_RETRY_WAIT_MS       (3000u)

#define IOT_PING_MS            (3000u)
#define IOT_RED_TX_PULSE_MS    (75u)
#define IOT_GREEN_RX_PULSE_MS  (1000u)

//---------------------------------------------------------------------------
// Auth globals
//---------------------------------------------------------------------------

extern volatile unsigned char iot_authenticated;
extern volatile unsigned char iot_auth_link;

//===========================================================================
// Init / reset / task
//===========================================================================

void IOT_Init(void);
void IOT_Reset(void);
void IOT_Task(void);

//===========================================================================
// Raw send
//===========================================================================

unsigned char IOT_SendCommand(const char *text);

//===========================================================================
// Status
//===========================================================================

unsigned char IOT_Active(void);
unsigned char IOT_Ready(void);
unsigned char IOT_WiFiConnected(void);
unsigned char IOT_Authorized(void);

//===========================================================================
// Raw RX line access
//===========================================================================

unsigned char IOT_LineAvailable(void);
void IOT_ClearLine(void);
void IOT_GetLine(char *dst, unsigned int max_len);
const char *IOT_PeekLine(void);

//===========================================================================
// Latest authorized +IPD payload access
//===========================================================================

unsigned char IOT_MessageAvailable(void);
void IOT_ClearMessage(void);
void IOT_GetMessage(char *dst, unsigned int max_len);
const char *IOT_PeekMessage(void);

//===========================================================================
// Network info
//===========================================================================

const char *IOT_PeekSSID(void);
const char *IOT_PeekIP(void);

#endif

