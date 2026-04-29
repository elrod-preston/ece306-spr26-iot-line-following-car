//===========================================================================
// File Name : iot.c
//
// Description:
//   Simplified robust IOT controller for ESP AT commands using UCA0.
//
// Behavior:
//   - Resets ESP module
//   - Runs compact AT setup sequence
//   - Tries Wi-Fi #1 first
//   - If Wi-Fi #1 fails, waits briefly, then tries Wi-Fi #2
//   - Starts TCP server on port 8888
//   - Displays IP only once on first successful connection
//   - After first IP display, IOT never writes LCD text again
//   - Backlight:
//       connecting / reconnecting = 1.0s flash
//       ready but no auth user     = 0.5s flash
//       auth user connected        = off
//===========================================================================

#include "Core\\lib\\iot.h"
#include "Core\\lib\\ports.h"
#include "Core\\lib\\display.h"

#include <msp430.h>
#include <string.h>

extern volatile unsigned long system_ticks_ms;

//===========================================================================
// Timing
//===========================================================================

#define IOT_BACKLITE_CONNECT_FLASH_MS   (3000u)
#define IOT_BACKLITE_AUTH_FLASH_MS      (500u)
#define IOT_WIFI_SWITCH_WAIT_MS         (3000u)

//===========================================================================
// State values
//===========================================================================

#define IOT_ST_RESET_HOLD               (0u)
#define IOT_ST_BOOT_WAIT                (1u)
#define IOT_ST_SEND_STEP                (2u)
#define IOT_ST_WAIT_STEP                (3u)
#define IOT_ST_READY                    (4u)
#define IOT_ST_RETRY_WAIT               (5u)
#define IOT_ST_NEXT_WIFI_WAIT           (6u)

//===========================================================================
// Step values
//===========================================================================

#define IOT_STEP_AT                     (0u)
#define IOT_STEP_ATE0                   (1u)
#define IOT_STEP_SLEEP_OFF              (2u)
#define IOT_STEP_AUTOCONN_OFF           (3u)
#define IOT_STEP_STA_MODE               (4u)
#define IOT_STEP_DISCONNECT             (5u)
#define IOT_STEP_JOIN_WIFI              (6u)
#define IOT_STEP_GET_IP                 (7u)
#define IOT_STEP_SERVER_OFF             (8u)
#define IOT_STEP_MUX_ON                 (9u)
#define IOT_STEP_SERVER_ON              (10u)

//===========================================================================
// Public auth globals
//===========================================================================

volatile unsigned char iot_authenticated = 0u;
volatile unsigned char iot_auth_link     = 255u;

//===========================================================================
// Private globals
//===========================================================================

static unsigned char iot_state = IOT_ST_RESET_HOLD;
static unsigned char iot_step  = IOT_STEP_AT;

static unsigned char iot_active            = 0u;
static unsigned char iot_ready             = 0u;
static unsigned char iot_wifi_connected    = 0u;
static unsigned char iot_line_available    = 0u;
static unsigned char iot_message_available = 0u;

static unsigned char iot_cmd_ok    = 0u;
static unsigned char iot_cmd_error = 0u;
static unsigned char iot_got_ip    = 0u;
static unsigned char iot_wifi_try  = 0u;

static unsigned char iot_build_index = 0u;
static unsigned char iot_rx_overflow = 0u;

static unsigned char iot_ip_display_done = 0u;

static unsigned long iot_state_start_ms       = 0u;
static unsigned long iot_red_pulse_until_ms   = 0u;
static unsigned long iot_green_pulse_until_ms = 0u;

static char iot_line_build[IOT_LINE_MAX + 1u];
static char iot_line_last[IOT_LINE_MAX + 1u];
static char iot_message_last[IOT_MSG_MAX + 1u];

static char iot_ssid[33];
static char iot_ip[20];
static char iot_join_cmd[IOT_CMD_MAX + 1u];

//===========================================================================
// Small helpers
//===========================================================================

static unsigned long IOT_Ticks(void)
{
  unsigned long now;

  now = system_ticks_ms;

  return now;
}

static void IOT_BackliteOn(void)
{
  P6OUT |= LCD_BACKLITE;
}

static void IOT_BackliteOff(void)
{
  P6OUT &= ~LCD_BACKLITE;
}

static void IOT_ClearAuth(void)
{
  iot_authenticated = 0u;
  iot_auth_link = 255u;
}

static void IOT_ClearStepFlags(void)
{
  iot_cmd_ok = 0u;
  iot_cmd_error = 0u;
  iot_got_ip = 0u;
}

static const char *IOT_GetSSID(void)
{
  if (iot_wifi_try == 0u)
  {
    return IOT_WIFI1_SSID;
  }

  return IOT_WIFI2_SSID;
}

static const char *IOT_GetPASS(void)
{
  if (iot_wifi_try == 0u)
  {
    return IOT_WIFI1_PASSWORD;
  }

  return IOT_WIFI2_PASSWORD;
}

static void IOT_ServiceBacklite(unsigned long now_ms)
{
  unsigned long flash_ms;

  if ((iot_state != IOT_ST_READY) || (iot_wifi_connected == 0u))
  {
    flash_ms = IOT_BACKLITE_CONNECT_FLASH_MS;
  }
  else if (iot_authenticated == 0u)
  {
    flash_ms = IOT_BACKLITE_AUTH_FLASH_MS;
  }
  else
  {
    IOT_BackliteOff();
    return;
  }

  if (((now_ms / flash_ms) & 1u) == 0u)
  {
    IOT_BackliteOn();
  }
  else
  {
    IOT_BackliteOff();
  }
}

static void IOT_ServiceLEDs(unsigned long now_ms)
{
  if (iot_active != 0u)
  {
    if (now_ms < iot_red_pulse_until_ms)
    {
      P2OUT |= IOT_RUN_RED;
    }
    else
    {
      P2OUT &= ~IOT_RUN_RED;
    }
  }
  else
  {
    P2OUT |= IOT_RUN_RED;
  }

  if (iot_wifi_connected != 0u)
  {
    if (now_ms < iot_green_pulse_until_ms)
    {
      P3OUT |= IOT_LINK_GRN;
    }
    else
    {
      P3OUT &= ~IOT_LINK_GRN;
    }
  }
  else
  {
    P3OUT |= IOT_LINK_GRN;
  }
}

static void IOT_Copy10(char *dst, const char *src)
{
  unsigned int i;

  if (src == 0)
  {
    src = "";
  }

  for (i = 0u; i < 10u; i++)
  {
    if (src[i] != 0)
    {
      dst[i] = src[i];
    }
    else
    {
      dst[i] = ' ';
    }
  }

  dst[10] = 0;
}

static void IOT_LCDRowOnce(unsigned int row, const char *text)
{
  char line[11];

  if (row > 3u)
  {
    return;
  }

  if (iot_ip_display_done != 0u)
  {
    return;
  }

  IOT_Copy10(line, text);
  Display_Text(row, line);
}

static void IOT_ShowIPOnce(void)
{
  char oct0[11];
  char oct1[11];
  char oct2[11];
  char oct3[11];
  char line3[11];

  unsigned int i;
  unsigned int j;
  unsigned char oct;

  if (iot_ip_display_done != 0u)
  {
    return;
  }

  if (iot_ip[0] == 0)
  {
    return;
  }

  memset(oct0, 0, sizeof(oct0));
  memset(oct1, 0, sizeof(oct1));
  memset(oct2, 0, sizeof(oct2));
  memset(oct3, 0, sizeof(oct3));
  memset(line3, 0, sizeof(line3));

  i = 0u;
  j = 0u;
  oct = 0u;

  while (iot_ip[i] != 0)
  {
    if (iot_ip[i] == '.')
    {
      oct++;
      j = 0u;
    }
    else if (j < 10u)
    {
      if (oct == 0u)
      {
        oct0[j] = iot_ip[i];
      }
      else if (oct == 1u)
      {
        oct1[j] = iot_ip[i];
      }
      else if (oct == 2u)
      {
        oct2[j] = iot_ip[i];
      }
      else
      {
        oct3[j] = iot_ip[i];
      }

      j++;
    }

    i++;
  }

  j = 0u;
  i = 0u;

  while ((oct3[i] != 0) && (j < 10u))
  {
    line3[j++] = oct3[i++];
  }

  if (j < 10u) line3[j++] = ':';
  if (j < 10u) line3[j++] = '8';
  if (j < 10u) line3[j++] = '8';
  if (j < 10u) line3[j++] = '8';
  if (j < 10u) line3[j++] = '8';

  line3[j] = 0;

  IOT_LCDRowOnce(0u, oct0);
  IOT_LCDRowOnce(1u, oct1);
  IOT_LCDRowOnce(2u, oct2);
  IOT_LCDRowOnce(3u, line3);

  iot_ip_display_done = 1u;
}

//===========================================================================
// Step helpers
//===========================================================================

static unsigned long IOT_StepTimeout(void)
{
  if (iot_step == IOT_STEP_JOIN_WIFI)
  {
    return IOT_JOIN_TIMEOUT_MS;
  }

  if ((iot_step == IOT_STEP_DISCONNECT) ||
      (iot_step == IOT_STEP_SERVER_OFF))
  {
    return 700u;
  }

  return IOT_CMD_TIMEOUT_MS;
}

static const char *IOT_StepCommand(void)
{
  switch (iot_step)
  {
    case IOT_STEP_AT:
      return "AT\r\n";

    case IOT_STEP_ATE0:
      return "ATE0\r\n";

    case IOT_STEP_SLEEP_OFF:
      return "AT+SLEEP=0\r\n";

    case IOT_STEP_AUTOCONN_OFF:
      return "AT+CWAUTOCONN=0\r\n";

    case IOT_STEP_STA_MODE:
      return "AT+CWMODE=1\r\n";

    case IOT_STEP_DISCONNECT:
      return "AT+CWQAP\r\n";

    case IOT_STEP_GET_IP:
      return "AT+CIFSR\r\n";

    case IOT_STEP_SERVER_OFF:
      return "AT+CIPSERVER=0\r\n";

    case IOT_STEP_MUX_ON:
      return "AT+CIPMUX=1\r\n";

    case IOT_STEP_SERVER_ON:
      return "AT+CIPSERVER=1,8888\r\n";

    default:
      return "";
  }
}

static unsigned char IOT_BuildJoinCommand(void)
{
  const char *ssid;
  const char *pass;

  unsigned int ssid_len;
  unsigned int pass_len;

  ssid = IOT_GetSSID();
  pass = IOT_GetPASS();

  if (ssid[0] == 0)
  {
    return 0u;
  }

  ssid_len = (unsigned int)strlen(ssid);
  pass_len = (unsigned int)strlen(pass);

  if ((10u + ssid_len + 3u + pass_len + 3u) > IOT_CMD_MAX)
  {
    return 0u;
  }

  strcpy(iot_join_cmd, "AT+CWJAP=\"");
  strcat(iot_join_cmd, ssid);
  strcat(iot_join_cmd, "\",\"");
  strcat(iot_join_cmd, pass);
  strcat(iot_join_cmd, "\"\r\n");

  strncpy(iot_ssid, ssid, sizeof(iot_ssid) - 1u);
  iot_ssid[sizeof(iot_ssid) - 1u] = 0;

  return 1u;
}

static void IOT_StartRetry(unsigned long now_ms)
{
  iot_ready = 0u;
  iot_wifi_connected = 0u;
  iot_state = IOT_ST_RETRY_WAIT;
  iot_state_start_ms = now_ms;

  IOT_ClearAuth();
}

static void IOT_TryNextWifiOrRetry(unsigned long now_ms)
{
  iot_ready = 0u;
  iot_wifi_connected = 0u;
  iot_got_ip = 0u;
  iot_ip[0] = 0;

  IOT_ClearAuth();

  /*
   * Wi-Fi #1 failed.
   * Do NOT reset immediately.
   * Wait briefly, then try Wi-Fi #2.
   */
  if ((iot_wifi_try == 0u) && (IOT_WIFI2_SSID[0] != 0))
  {
    iot_wifi_try = 1u;
    iot_step = IOT_STEP_JOIN_WIFI;
    iot_state = IOT_ST_NEXT_WIFI_WAIT;
    iot_state_start_ms = now_ms;
    return;
  }

  IOT_StartRetry(now_ms);
}

static void IOT_FailStep(unsigned long now_ms)
{
  /*
   * Once we are at Wi-Fi join or later, Wi-Fi #1 failure should fall
   * through to Wi-Fi #2 before doing a full ESP reset.
   */
  if (iot_step >= IOT_STEP_JOIN_WIFI)
  {
    IOT_TryNextWifiOrRetry(now_ms);
    return;
  }

  IOT_StartRetry(now_ms);
}

static void IOT_NextStep(unsigned long now_ms)
{
  if (iot_step == IOT_STEP_SERVER_ON)
  {
    iot_ready = 1u;
    iot_state = IOT_ST_READY;
    iot_state_start_ms = now_ms;
    return;
  }

  iot_step++;
  iot_state = IOT_ST_SEND_STEP;
}

//===========================================================================
// RX parsing
//===========================================================================

static void IOT_HandleIPD(const char *line, unsigned long now_ms)
{
  const char *p;
  const char *payload;

  unsigned int value;
  unsigned int i;

  unsigned char link_num;

  if (strncmp(line, "+IPD,", 5u) != 0)
  {
    return;
  }

  p = line + 5u;
  value = 0u;

  while ((*p >= '0') && (*p <= '9'))
  {
    value = (value * 10u) + (unsigned int)(*p - '0');
    p++;
  }

  if ((*p != ',') || (value > 255u))
  {
    return;
  }

  link_num = (unsigned char)value;

  while ((*p != 0) && (*p != ':'))
  {
    p++;
  }

  if (*p != ':')
  {
    return;
  }

  payload = p + 1u;

  iot_green_pulse_until_ms = now_ms + IOT_GREEN_RX_PULSE_MS;

  if (strcmp(payload, IOT_AUTH_PASS) == 0)
  {
    iot_authenticated = 1u;
    iot_auth_link = link_num;

    iot_message_last[0] = 0;
    iot_message_available = 0u;

    return;
  }

  if ((iot_authenticated != 0u) && (link_num == iot_auth_link))
  {
    for (i = 0u; i < IOT_MSG_MAX; i++)
    {
      if (payload[i] == 0)
      {
        break;
      }

      iot_message_last[i] = payload[i];
    }

    iot_message_last[i] = 0;
    iot_message_available = 1u;
  }
}

static void IOT_ParseIPFromLine(void)
{
  char *p;
  unsigned int i;

  p = strstr(iot_line_last, "STAIP,\"");

  if (p == 0)
  {
    return;
  }

  p += 7;
  i = 0u;

  while ((p[i] != 0) &&
         (p[i] != '"') &&
         (i < (sizeof(iot_ip) - 1u)))
  {
    iot_ip[i] = p[i];
    i++;
  }

  iot_ip[i] = 0;

  if (i > 0u)
  {
    iot_got_ip = 1u;
    iot_wifi_connected = 1u;
  }
}

static void IOT_HandleClosedLink(void)
{
  unsigned int i;
  unsigned int link_num;

  if (iot_authenticated == 0u)
  {
    return;
  }

  if (strstr(iot_line_last, ",CLOSED") == 0)
  {
    return;
  }

  i = 0u;
  link_num = 0u;

  while ((iot_line_last[i] >= '0') && (iot_line_last[i] <= '9'))
  {
    link_num = (link_num * 10u) + (unsigned int)(iot_line_last[i] - '0');
    i++;
  }

  if ((i > 0u) && (link_num == iot_auth_link))
  {
    IOT_ClearAuth();
    iot_message_last[0] = 0;
    iot_message_available = 0u;
  }
}

static void IOT_HandleLine(unsigned long now_ms)
{
  iot_line_available = 1u;
  iot_red_pulse_until_ms = now_ms + IOT_RED_TX_PULSE_MS;

  if ((strcmp(iot_line_last, "OK") == 0) ||
      (strcmp(iot_line_last, "SEND OK") == 0) ||
      (strstr(iot_line_last, "no change") != 0))
  {
    iot_cmd_ok = 1u;
  }

  if ((strcmp(iot_line_last, "ERROR") == 0) ||
      (strcmp(iot_line_last, "FAIL") == 0) ||
      (strstr(iot_line_last, "+CWJAP:") != 0))
  {
    iot_cmd_error = 1u;
  }

  if ((strstr(iot_line_last, "WIFI CONNECTED") != 0) ||
      (strstr(iot_line_last, "ALREADY CONNECTED") != 0))
  {
    iot_wifi_connected = 1u;
  }

  if ((strstr(iot_line_last, "WIFI GOT IP") != 0) ||
      (strstr(iot_line_last, "GOT IP") != 0))
  {
    iot_wifi_connected = 1u;
    iot_got_ip = 1u;
  }

  if ((strstr(iot_line_last, "WIFI DISCONNECT") != 0) ||
      (strstr(iot_line_last, "No AP") != 0))
  {
    iot_wifi_connected = 0u;
    IOT_ClearAuth();
  }

  IOT_ParseIPFromLine();
  IOT_HandleClosedLink();
  IOT_HandleIPD(iot_line_last, now_ms);

  iot_build_index = 0u;
  iot_rx_overflow = 0u;
  iot_line_build[0] = 0;
}

static void IOT_ParseRX(unsigned long now_ms)
{
  unsigned char ch;

  while (iot_rx_rd != iot_rx_wr)
  {
    ch = (unsigned char)IOT_Ring_Rx[iot_rx_rd];
    iot_rx_rd++;

    if (iot_rx_rd >= LARGE_RING_SIZE)
    {
      iot_rx_rd = BEGINNING;
    }

    if ((ch == '\r') || (ch == '\n'))
    {
      if ((iot_rx_overflow == 0u) && (iot_build_index > 0u))
      {
        iot_line_build[iot_build_index] = 0;

        strncpy(iot_line_last, iot_line_build, IOT_LINE_MAX);
        iot_line_last[IOT_LINE_MAX] = 0;

        IOT_HandleLine(now_ms);
      }

      iot_build_index = 0u;
      iot_rx_overflow = 0u;
      iot_line_build[0] = 0;
    }
    else if (ch != 0u)
    {
      if ((iot_rx_overflow == 0u) && (iot_build_index < IOT_LINE_MAX))
      {
        iot_line_build[iot_build_index++] = (char)ch;
        iot_line_build[iot_build_index] = 0;
      }
      else
      {
        iot_rx_overflow = 1u;
      }
    }
  }
}

//===========================================================================
// Init / reset
//===========================================================================

void IOT_Init(void)
{
  Init_Serial_UCA0(BAUD_115200);
  IOT_Reset();
}

void IOT_Reset(void)
{
  P5OUT |= IOT_BOOT_CPU;
  P3OUT &= ~IOT_EN;

  P2OUT |= IOT_RUN_RED;
  P3OUT |= IOT_LINK_GRN;

  P6DIR |= LCD_BACKLITE;
  IOT_BackliteOff();

  iot_state = IOT_ST_RESET_HOLD;
  iot_step = IOT_STEP_AT;

  iot_active = 0u;
  iot_ready = 0u;
  iot_wifi_connected = 0u;
  iot_line_available = 0u;
  iot_message_available = 0u;

  iot_cmd_ok = 0u;
  iot_cmd_error = 0u;
  iot_got_ip = 0u;
  iot_wifi_try = 0u;

  iot_build_index = 0u;
  iot_rx_overflow = 0u;

  iot_state_start_ms = IOT_Ticks();
  iot_red_pulse_until_ms = 0u;
  iot_green_pulse_until_ms = 0u;

  memset(iot_line_build, 0, sizeof(iot_line_build));
  memset(iot_line_last, 0, sizeof(iot_line_last));
  memset(iot_message_last, 0, sizeof(iot_message_last));
  memset(iot_ssid, 0, sizeof(iot_ssid));
  memset(iot_ip, 0, sizeof(iot_ip));
  memset(iot_join_cmd, 0, sizeof(iot_join_cmd));

  IOT_ClearAuth();

  iot_rx_rd = iot_rx_wr;
}

//===========================================================================
// Main IOT task
//===========================================================================

void IOT_Task(void)
{
  unsigned long now_ms;
  unsigned long timeout_ms;

  const char *cmd;

  now_ms = IOT_Ticks();

  IOT_ParseRX(now_ms);
  IOT_ServiceLEDs(now_ms);

  switch (iot_state)
  {
    case IOT_ST_RESET_HOLD:
      if ((now_ms - iot_state_start_ms) >= IOT_RESET_HOLD_MS)
      {
        P3OUT |= IOT_EN;

        iot_active = 1u;
        iot_state = IOT_ST_BOOT_WAIT;
        iot_state_start_ms = now_ms;
      }
      break;

    case IOT_ST_BOOT_WAIT:
      if ((now_ms - iot_state_start_ms) >= IOT_BOOT_WAIT_MS)
      {
        iot_step = IOT_STEP_AT;
        iot_state = IOT_ST_SEND_STEP;
      }
      break;

    case IOT_ST_SEND_STEP:
      if (iot_step == IOT_STEP_JOIN_WIFI)
      {
        if (IOT_BuildJoinCommand() == 0u)
        {
          IOT_FailStep(now_ms);
          break;
        }

        cmd = iot_join_cmd;
      }
      else
      {
        cmd = IOT_StepCommand();
      }

      if (IOT_SendCommand(cmd) != 0u)
      {
        IOT_ClearStepFlags();
        iot_state = IOT_ST_WAIT_STEP;
        iot_state_start_ms = now_ms;
      }
      break;

    case IOT_ST_WAIT_STEP:
      timeout_ms = IOT_StepTimeout();

      /*
       * Cleanup commands.
       * OK, ERROR, or timeout are all acceptable.
       */
      if ((iot_step == IOT_STEP_DISCONNECT) ||
          (iot_step == IOT_STEP_SERVER_OFF))
      {
        if ((iot_cmd_ok != 0u) ||
            (iot_cmd_error != 0u) ||
            ((now_ms - iot_state_start_ms) >= timeout_ms))
        {
          IOT_NextStep(now_ms);
        }
      }

      /*
       * Wi-Fi join.
       * If Wi-Fi #1 gets +CWJAP / ERROR, it switches to Wi-Fi #2.
       */
      else if (iot_step == IOT_STEP_JOIN_WIFI)
      {
        if ((iot_cmd_ok != 0u) && (iot_wifi_connected != 0u))
        {
          IOT_NextStep(now_ms);
        }
        else if (iot_cmd_error != 0u)
        {
          IOT_FailStep(now_ms);
        }
        else if ((now_ms - iot_state_start_ms) >= timeout_ms)
        {
          if (iot_got_ip != 0u)
          {
            IOT_NextStep(now_ms);
          }
          else
          {
            IOT_FailStep(now_ms);
          }
        }
      }

      /*
       * IP query.
       * Prefer final OK, but allow continuing if STAIP was already seen.
       */
      else if (iot_step == IOT_STEP_GET_IP)
      {
        if (iot_cmd_ok != 0u)
        {
          IOT_NextStep(now_ms);
        }
        else if (iot_cmd_error != 0u)
        {
          if (iot_got_ip != 0u)
          {
            IOT_NextStep(now_ms);
          }
          else
          {
            IOT_FailStep(now_ms);
          }
        }
        else if ((now_ms - iot_state_start_ms) >= timeout_ms)
        {
          if (iot_got_ip != 0u)
          {
            IOT_NextStep(now_ms);
          }
          else
          {
            IOT_FailStep(now_ms);
          }
        }
      }

      /*
       * Normal AT setup commands.
       */
      else
      {
        if (iot_cmd_ok != 0u)
        {
          IOT_NextStep(now_ms);
        }
        else if ((iot_cmd_error != 0u) ||
                 ((now_ms - iot_state_start_ms) >= timeout_ms))
        {
          IOT_FailStep(now_ms);
        }
      }
      break;

    case IOT_ST_READY:
      if (iot_wifi_connected == 0u)
      {
        IOT_FailStep(now_ms);
      }
      break;

    case IOT_ST_NEXT_WIFI_WAIT:
      if ((now_ms - iot_state_start_ms) >= IOT_WIFI_SWITCH_WAIT_MS)
      {
        IOT_ClearStepFlags();
        iot_step = IOT_STEP_JOIN_WIFI;
        iot_state = IOT_ST_SEND_STEP;
      }
      break;

    case IOT_ST_RETRY_WAIT:
      if ((now_ms - iot_state_start_ms) >= IOT_RETRY_WAIT_MS)
      {
        IOT_Reset();
      }
      break;

    default:
      IOT_Reset();
      break;
  }

  if ((iot_state == IOT_ST_READY) && (iot_ip_display_done == 0u))
  {
    IOT_ShowIPOnce();
  }

  IOT_ServiceBacklite(now_ms);
}

//===========================================================================
// Raw send
//===========================================================================

unsigned char IOT_SendCommand(const char *text)
{
  unsigned int i;
  unsigned long now_ms;

  if (iot_active == 0u)
  {
    return 0u;
  }

  if (iot_tx_active != 0u)
  {
    return 0u;
  }

  i = 0u;

  while ((text[i] != 0) && (i < (LARGE_RING_SIZE - 1u)))
  {
    iot_TX_buf[i] = text[i];
    i++;
  }

  iot_TX_buf[i] = 0;
  USCI_A0_transmit();

  now_ms = IOT_Ticks();
  iot_red_pulse_until_ms = now_ms + IOT_RED_TX_PULSE_MS;

  return 1u;
}

//===========================================================================
// Status
//===========================================================================

unsigned char IOT_Active(void)
{
  return iot_active;
}

unsigned char IOT_Ready(void)
{
  return iot_ready;
}

unsigned char IOT_WiFiConnected(void)
{
  return iot_wifi_connected;
}

unsigned char IOT_Authorized(void)
{
  return iot_authenticated;
}

//===========================================================================
// Raw RX line access
//===========================================================================

unsigned char IOT_LineAvailable(void)
{
  return iot_line_available;
}

void IOT_ClearLine(void)
{
  iot_line_last[0] = 0;
  iot_line_available = 0u;
}

void IOT_GetLine(char *dst, unsigned int max_len)
{
  if (max_len == 0u)
  {
    return;
  }

  strncpy(dst, iot_line_last, max_len - 1u);
  dst[max_len - 1u] = 0;

  iot_line_available = 0u;
}

const char *IOT_PeekLine(void)
{
  return iot_line_last;
}

//===========================================================================
// Latest authorized +IPD payload access
//===========================================================================

unsigned char IOT_MessageAvailable(void)
{
  return iot_message_available;
}

void IOT_ClearMessage(void)
{
  iot_message_last[0] = 0;
  iot_message_available = 0u;
}

void IOT_GetMessage(char *dst, unsigned int max_len)
{
  if (max_len == 0u)
  {
    return;
  }

  strncpy(dst, iot_message_last, max_len - 1u);
  dst[max_len - 1u] = 0;

  iot_message_available = 0u;
}

const char *IOT_PeekMessage(void)
{
  return iot_message_last;
}

//===========================================================================
// Network info
//===========================================================================

const char *IOT_PeekSSID(void)
{
  return iot_ssid;
}

const char *IOT_PeekIP(void)
{
  return iot_ip;
}
