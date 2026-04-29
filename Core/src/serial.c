//==============================================================================
// File Name : serial.c
//
// Description:
//   UCA0 = IOT
//   UCA1 = USB
//
//   115200 only
//   Anything received on USB is transmitted on IOT
//   Anything received on IOT is transmitted on USB
//==============================================================================

#include "msp430.h"
#include "Core\\lib\\serial.h"

//------------------------------------------------------------------------------
// RX ring-buffer storage
//------------------------------------------------------------------------------
volatile unsigned int  iot_rx_wr  = BEGINNING;
volatile unsigned int  iot_rx_rd  = BEGINNING;
volatile char          IOT_Ring_Rx[LARGE_RING_SIZE];

volatile unsigned int  usb_rx_wr  = BEGINNING;
volatile unsigned int  usb_rx_rd  = BEGINNING;
volatile char          USB_Ring_Rx[SMALL_RING_SIZE];

//------------------------------------------------------------------------------
// Mirror queues
//------------------------------------------------------------------------------
static volatile char          usb_to_iot[LARGE_RING_SIZE];
static volatile unsigned int  usb_to_iot_wr = BEGINNING;
static volatile unsigned int  usb_to_iot_rd = BEGINNING;

static volatile char          iot_to_usb[LARGE_RING_SIZE];
static volatile unsigned int  iot_to_usb_wr = BEGINNING;
static volatile unsigned int  iot_to_usb_rd = BEGINNING;

//------------------------------------------------------------------------------
// Optional direct transmit buffers
//------------------------------------------------------------------------------
volatile char          iot_TX_buf[LARGE_RING_SIZE];
volatile unsigned int  iot_tx = 0;
volatile unsigned char iot_tx_active = 0;

char                   process_buffer[IOT_PROCESS_SIZE];
char                   pb_index = 0;
static volatile unsigned char usb_tx_active = 0;

//------------------------------------------------------------------------------
// IOT process / parse state
//------------------------------------------------------------------------------
volatile char          IOT_Data[IOT_DATA_LINES][IOT_DATA_COLS];
volatile unsigned int  line             = 0;
volatile unsigned int  character        = 0;
volatile unsigned int  nextline         = 0;
volatile unsigned int  iot_parse        = 0;
volatile unsigned int  boot_state       = 0;
volatile unsigned int  ip_address_found = 0;
volatile unsigned int  iot_index        = 0;
char                   ip_address[20];

//==============================================================================
// Helpers
//==============================================================================
static unsigned int Next_Large(unsigned int index)
{
  index++;
  if (index >= LARGE_RING_SIZE) {
    index = BEGINNING;
  }
  return index;
}

static unsigned int Next_Small(unsigned int index)
{
  index++;
  if (index >= SMALL_RING_SIZE) {
    index = BEGINNING;
  }
  return index;
}

static void Queue_USB_To_IOT(char value)
{
  unsigned int next;

  next = Next_Large(usb_to_iot_wr);
  if (next != usb_to_iot_rd) {
    usb_to_iot[usb_to_iot_wr] = value;
    usb_to_iot_wr = next;
  }
}

static void Queue_IOT_To_USB(char value)
{
  unsigned int next;

  next = Next_Large(iot_to_usb_wr);
  if (next != iot_to_usb_rd) {
    iot_to_usb[iot_to_usb_wr] = value;
    iot_to_usb_wr = next;
  }
}

//==============================================================================
// Init_Serial_UCA0
// 115200 only
//==============================================================================
void Init_Serial_UCA0(unsigned char speed)
{
  unsigned int i;
  unsigned int j;

  (void)speed;

  for (i = 0; i < LARGE_RING_SIZE; i++) {
    IOT_Ring_Rx[i] = 0x00;
    usb_to_iot[i]  = 0x00;
    iot_to_usb[i]  = 0x00;
    iot_TX_buf[i]  = 0x00;
  }

  for (i = 0; i < IOT_DATA_LINES; i++) {
    for (j = 0; j < IOT_DATA_COLS; j++) {
      IOT_Data[i][j] = 0x00;
    }
  }

  for (i = 0; i < 20; i++) {
    ip_address[i] = 0x00;
  }

  iot_rx_wr       = BEGINNING;
  iot_rx_rd       = BEGINNING;
  usb_to_iot_wr   = BEGINNING;
  usb_to_iot_rd   = BEGINNING;
  iot_to_usb_wr   = BEGINNING;
  iot_to_usb_rd   = BEGINNING;
  iot_tx          = 0;
  iot_tx_active   = 0;

  line             = 0;
  character        = 0;
  nextline         = 0;
  iot_parse        = 0;
  boot_state       = 0;
  ip_address_found = 0;
  iot_index        = 0;

  UCA0CTLW0  = UCSWRST;
  UCA0CTLW0 |= UCSSEL__SMCLK;
  UCA0CTLW0 &= ~UCMSB;
  UCA0CTLW0 &= ~UCSPB;
  UCA0CTLW0 &= ~UCPEN;
  UCA0CTLW0 &= ~UCSYNC;
  UCA0CTLW0 &= ~UC7BIT;
  UCA0CTLW0 |= UCMODE_0;

  // 8 MHz SMCLK, 115200
  UCA0BRW   = 4;
  UCA0MCTLW = 0x5551;

  UCA0CTLW0 &= ~UCSWRST;
  UCA0IE     = 0x00;
  UCA0IE    |= UCRXIE;
}

//==============================================================================
// Init_Serial_UCA1
// 115200 only
//==============================================================================
void Init_Serial_UCA1(unsigned char speed)
{
  unsigned int i;

  (void)speed;

  for (i = 0; i < SMALL_RING_SIZE; i++) {
    USB_Ring_Rx[i] = 0x00;
  }

  for (i = 0; i < IOT_PROCESS_SIZE; i++) {
    process_buffer[i] = 0x00;
  }

  usb_rx_wr     = BEGINNING;
  usb_rx_rd     = BEGINNING;
  pb_index      = 0;
  usb_tx_active = 0;

  UCA1CTLW0  = UCSWRST;
  UCA1CTLW0 |= UCSSEL__SMCLK;
  UCA1CTLW0 &= ~UCMSB;
  UCA1CTLW0 &= ~UCSPB;
  UCA1CTLW0 &= ~UCPEN;
  UCA1CTLW0 &= ~UCSYNC;
  UCA1CTLW0 &= ~UC7BIT;
  UCA1CTLW0 |= UCMODE_0;

  // 8 MHz SMCLK, 115200
  UCA1BRW   = 4;
  UCA1MCTLW = 0x5551;

  UCA1CTLW0 &= ~UCSWRST;
  UCA1IE     = 0x00;
  UCA1IE    |= UCRXIE;
}

//==============================================================================
// Optional string transmit on UCA0
//==============================================================================
void USCI_A0_transmit(void)
{
  if (iot_TX_buf[0] != 0x00) {
    iot_tx = 0;
    iot_tx_active = 1;
    UCA0IE |= UCTXIE;
  }
}

//==============================================================================
// Optional string transmit on UCA1
//==============================================================================
void USCI_A1_transmit(void)
{
  if (process_buffer[0] != 0x00) {
    pb_index = 0;
    usb_tx_active = 1;
    UCA1IE |= UCTXIE;
  }
}

//==============================================================================
// eUSCI_A0 ISR
// UCA0 = IOT
//==============================================================================
#pragma vector = EUSCI_A0_VECTOR
__interrupt void eUSCI_A0_ISR(void)
{
  char iot_receive;
  unsigned int temp;

  switch (__even_in_range(UCA0IV, 0x08)) {

    case 0:
      break;

    case 2:   // RXIFG
      iot_receive = UCA0RXBUF;

      temp = iot_rx_wr;
      IOT_Ring_Rx[temp] = iot_receive;
      iot_rx_wr = Next_Large(iot_rx_wr);

      Queue_IOT_To_USB(iot_receive);
      UCA1IE |= UCTXIE;
      break;

    case 4:   // TXIFG
      if (iot_tx_active) {
        if (iot_TX_buf[iot_tx] != 0x00) {
          UCA0TXBUF = iot_TX_buf[iot_tx++];
          if (iot_TX_buf[iot_tx] == 0x00) {
            iot_tx = 0;
            iot_tx_active = 0;
          }
        } else {
          iot_tx = 0;
          iot_tx_active = 0;
        }

        if ((!iot_tx_active) && (usb_to_iot_rd == usb_to_iot_wr)) {
          UCA0IE &= ~UCTXIE;
        }
      }
      else if (usb_to_iot_rd != usb_to_iot_wr) {
        UCA0TXBUF = usb_to_iot[usb_to_iot_rd];
        usb_to_iot_rd = Next_Large(usb_to_iot_rd);

        if (usb_to_iot_rd == usb_to_iot_wr) {
          UCA0IE &= ~UCTXIE;
        }
      }
      else {
        UCA0IE &= ~UCTXIE;
      }
      break;

    default:
      break;
  }
}

//==============================================================================
// eUSCI_A1 ISR
// UCA1 = USB
//==============================================================================
#pragma vector = EUSCI_A1_VECTOR
__interrupt void eUSCI_A1_ISR(void)
{
  char usb_receive;
  unsigned int temp;

  switch (__even_in_range(UCA1IV, 0x08)) {

    case 0:
      break;

    case 2:   // RXIFG
      usb_receive = UCA1RXBUF;

      temp = usb_rx_wr;
      USB_Ring_Rx[temp] = usb_receive;
      usb_rx_wr = Next_Small(usb_rx_wr);

      Queue_USB_To_IOT(usb_receive);
      UCA0IE |= UCTXIE;
      break;

    case 4:   // TXIFG
      if (usb_tx_active) {
        if (process_buffer[(unsigned int)pb_index] != 0x00) {
          UCA1TXBUF = process_buffer[(unsigned int)pb_index++];
          if (process_buffer[(unsigned int)pb_index] == 0x00) {
            pb_index = 0;
            usb_tx_active = 0;
          }
        } else {
          pb_index = 0;
          usb_tx_active = 0;
        }

        if ((!usb_tx_active) && (iot_to_usb_rd == iot_to_usb_wr)) {
          UCA1IE &= ~UCTXIE;
        }
      }
      else if (iot_to_usb_rd != iot_to_usb_wr) {
        UCA1TXBUF = iot_to_usb[iot_to_usb_rd];
        iot_to_usb_rd = Next_Large(iot_to_usb_rd);

        if (iot_to_usb_rd == iot_to_usb_wr) {
          UCA1IE &= ~UCTXIE;
        }
      }
      else {
        UCA1IE &= ~UCTXIE;
      }
      break;

    default:
      break;
  }
}

//==============================================================================
// IOT_Process
//==============================================================================
void IOT_Process(void)
{
  unsigned int iot_rx_wr_temp;

  iot_rx_wr_temp = iot_rx_wr;

  if (iot_rx_wr_temp != iot_rx_rd) {

    IOT_Data[line][character] = IOT_Ring_Rx[iot_rx_rd++];
    if (iot_rx_rd >= LARGE_RING_SIZE) {
      iot_rx_rd = BEGINNING;
    }

    if (IOT_Data[line][character] == 0x0A) {
      character = 0;
      line++;
      if (line >= IOT_DATA_LINES) {
        line = 0;
      }

      nextline = line + 1;
      if (nextline >= IOT_DATA_LINES) {
        nextline = 0;
      }
    }
    else {
      character++;
      if (character >= IOT_DATA_COLS) {
        character = 0;
      }
    }
  }
}
