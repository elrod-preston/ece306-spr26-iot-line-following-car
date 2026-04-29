#ifndef SERIAL_H_
#define SERIAL_H_

#include "msp430.h"

#define BEGINNING          (0u)

#define LARGE_RING_SIZE    (256u)
#define SMALL_RING_SIZE    (16u)

#define IOT_DATA_LINES     (4u)
#define IOT_DATA_COLS      (32u)
#define IOT_PROCESS_SIZE   (32u)

#define BAUD_115200        (0u)

extern volatile unsigned int  iot_rx_wr;
extern volatile unsigned int  iot_rx_rd;
extern volatile char          IOT_Ring_Rx[LARGE_RING_SIZE];

extern volatile unsigned int  usb_rx_wr;
extern volatile unsigned int  usb_rx_rd;
extern volatile char          USB_Ring_Rx[SMALL_RING_SIZE];

extern volatile char          iot_TX_buf[LARGE_RING_SIZE];
extern volatile unsigned int  iot_tx;
extern volatile unsigned char iot_tx_active;

extern volatile char          IOT_Data[IOT_DATA_LINES][IOT_DATA_COLS];
extern volatile unsigned int  line;
extern volatile unsigned int  character;
extern volatile unsigned int  nextline;
extern volatile unsigned int  iot_parse;
extern volatile unsigned int  boot_state;
extern volatile unsigned int  ip_address_found;
extern volatile unsigned int  iot_index;

extern char                   ip_address[20];

extern char                   process_buffer[IOT_PROCESS_SIZE];
extern char                   pb_index;

void Init_Serial_UCA0(unsigned char speed);
void Init_Serial_UCA1(unsigned char speed);

void USCI_A0_transmit(void);
void USCI_A1_transmit(void);

void IOT_Process(void);

#endif
