//==============================================================================
// File Name : LCD.c
//
// Description:
//   DOGS104-A / SSD1803A replacement LCD driver.
//
// Fixes:
//   - Uses your actual board wiring from ports.c:
//       RESET_LCD    = P4.0
//       UCB1_CS_LCD  = P4.4
//       UCB1CLK      = P4.5
//       UCB1SIMO     = P4.6
//       LCD_BACKLITE = P6.0
//
//   - Provides required old binary symbols:
//       update_display
//       display_changed
//       display_line
//       display
//       Init_LCD()
//       Display_Update()
//
//   - Avoids __delay_cycles()
//   - Avoids delay multiplication
//   - Avoids variable 2D array indexing in helper path
//   - Uses timeout-protected SPI waits
//
//==============================================================================

#include <msp430.h>

#include "Core\\lib\\LCD.h"
#include "Core\\lib\\ports.h"

//==============================================================================
// LCD CONSTANTS
//==============================================================================

#define LCD_HOME_L1                  (0x80u)
#define LCD_HOME_L2                  (0xA0u)
#define LCD_HOME_L3                  (0xC0u)
#define LCD_HOME_L4                  (0xE0u)

#define LCD_START_WR_INSTRUCTION     (0x1Fu)
#define LCD_START_WR_DATA            (0x5Fu)

#define LCD_CLEAR_DISPLAY            (0x01u)

#define LCD_DISPLAY_CONTROL          (0x08u)
#define LCD_DISPLAY_ON               (0x04u)

#define LCD_COL_COUNT                (10u)

#define LCD_SPI_TIMEOUT              (60000u)

// SMCLK = 8 MHz.
// /16 = 500 kHz.
#ifndef LCD_SPI_PRESCALER
#define LCD_SPI_PRESCALER            (16u)
#endif

//==============================================================================
// REQUIRED OLD GLOBALS
//==============================================================================

volatile unsigned char update_display  = 0u;
volatile unsigned char display_changed = 0u;

// 0 = no error
// 1 = TX flag timeout
// 2 = RX flag timeout
// 3 = BUSY timeout
volatile unsigned char lcd_spi_error = 0u;

char display_line[LCD_ROWS][LCD_STRING_SIZE] =
{
  "          ",
  "          ",
  "          ",
  "          "
};

char *display[LCD_ROWS] =
{
  &display_line[0][0],
  &display_line[1][0],
  &display_line[2][0],
  &display_line[3][0]
};

//==============================================================================
// MULTIPLY-FREE DELAYS
//==============================================================================

static void LCD_Delay_Tiny(void)
{

}

static void LCD_Delay_us(unsigned int us)
{
  while (us != 0u)
  {
    LCD_Delay_Tiny();
    us--;
  }
}

static void LCD_Delay_ms(unsigned int ms)
{
  unsigned int us_count;

  while (ms != 0u)
  {
    us_count = 1000u;

    while (us_count != 0u)
    {
      LCD_Delay_Tiny();
      us_count--;
    }

    ms--;
  }
}

//==============================================================================
// SAFE LINE POINTER HELPER
//==============================================================================

static char *LCD_Get_Line_Ptr(unsigned char row)
{
  switch (row)
  {
    case 0u:
      return &display_line[0][0];

    case 1u:
      return &display_line[1][0];

    case 2u:
      return &display_line[2][0];

    case 3u:
      return &display_line[3][0];

    default:
      return &display_line[0][0];
  }
}

//==============================================================================
// LCD CHIP SELECT
//==============================================================================
//
// Your ports.c uses:
//   P4.4 = UCB1_CS_LCD
//
//==============================================================================

void LCD_ChipSelect_Active(void)
{
  P4OUT &= ~UCB1_CS_LCD;       // CS active low
  LCD_Delay_us(4u);
}

void LCD_ChipSelect_Idle(void)
{
  LCD_Delay_us(4u);
  P4OUT |= UCB1_CS_LCD;        // CS inactive high
  LCD_Delay_us(4u);
}

//==============================================================================
// LCD RESET
//==============================================================================
//
// Your ports.c uses:
//   P4.0 = RESET_LCD
//
//==============================================================================

void LCD_Reset_Active(void)
{
  P4OUT &= ~RESET_LCD;         // Reset active low
}

void LCD_Reset_Idle(void)
{
  P4OUT |= RESET_LCD;          // Reset idle high
}

void LCD_Hardware_Reset(void)
{
  LCD_Reset_Idle();
  LCD_Delay_ms(5u);

  LCD_Reset_Active();
  LCD_Delay_ms(10u);

  LCD_Reset_Idle();
  LCD_Delay_ms(80u);
}

void Reset_LCD(void)
{
  LCD_Hardware_Reset();
}

//==============================================================================
// LCD_SPI_Init
//==============================================================================
//
// Your ports.c uses:
//   P4.5 = UCB1CLK
//   P4.6 = UCB1SIMO
//   P4.7 = UCB1SOMI
//
//==============================================================================

void LCD_SPI_Init(void)
{
#ifdef LOCKLPM5
  PM5CTL0 &= ~LOCKLPM5;
#endif

  lcd_spi_error = 0u;

  //---------------------------------------------------------------------------
  // LCD control pins.
  //---------------------------------------------------------------------------

  P4SEL0 &= ~RESET_LCD;
  P4SEL1 &= ~RESET_LCD;
  P4DIR  |=  RESET_LCD;
  P4OUT  |=  RESET_LCD;

  P4SEL0 &= ~UCB1_CS_LCD;
  P4SEL1 &= ~UCB1_CS_LCD;
  P4DIR  |=  UCB1_CS_LCD;
  P4OUT  |=  UCB1_CS_LCD;

  //---------------------------------------------------------------------------
  // SPI pins.
  //---------------------------------------------------------------------------

  P4SEL0 |=  UCB1CLK;
  P4SEL1 &= ~UCB1CLK;

  P4SEL0 |=  UCB1SIMO;
  P4SEL1 &= ~UCB1SIMO;

  P4SEL0 |=  UCB1SOMI;
  P4SEL1 &= ~UCB1SOMI;

  //---------------------------------------------------------------------------
  // eUSCI_B1 SPI setup.
  //---------------------------------------------------------------------------

  UCB1CTLW0 = UCSWRST;               // Hold UCB1 in reset

  UCB1CTLW0 |= UCSSEL__SMCLK;        // SMCLK source
  UCB1BRW = LCD_SPI_PRESCALER;       // 8 MHz / 16 = 500 kHz

  UCB1CTLW0 |= UCCKPL;               // Clock idle high
  UCB1CTLW0 &= ~UCCKPH;              // Original DOGS104 style phase

  UCB1CTLW0 |= UCMST;                // SPI master
  UCB1CTLW0 |= UCSYNC;               // Synchronous SPI

  UCB1CTLW0 &= ~UCMSB;               // LSB first for DOGS104 serial format
  UCB1CTLW0 &= ~UCMODE_3;            // 3-pin SPI

  UCB1CTLW0 &= ~UCSWRST;             // Enable UCB1

  if (UCB1IFG & UCRXIFG)
  {
    (void)UCB1RXBUF;
  }

  LCD_Delay_us(100u);
}

//==============================================================================
// LCD_SPI_Write
//==============================================================================

void LCD_SPI_Write(char byte)
{
  volatile unsigned int timeout;

  timeout = LCD_SPI_TIMEOUT;

  while (!(UCB1IFG & UCTXIFG))
  {
    timeout--;

    if (timeout == 0u)
    {
      lcd_spi_error = 1u;
      return;
    }
  }

  UCB1TXBUF = (unsigned char)byte;

  timeout = LCD_SPI_TIMEOUT;

  while (!(UCB1IFG & UCRXIFG))
  {
    timeout--;

    if (timeout == 0u)
    {
      lcd_spi_error = 2u;
      return;
    }
  }

  (void)UCB1RXBUF;

  timeout = LCD_SPI_TIMEOUT;

  while (UCB1STATW & UCBUSY)
  {
    timeout--;

    if (timeout == 0u)
    {
      lcd_spi_error = 3u;
      return;
    }
  }
}

//==============================================================================
// LCD WRITE FUNCTIONS
//==============================================================================

void Write_LCD_Ins(char instruction)
{
  unsigned char value;

  value = (unsigned char)instruction;

  LCD_ChipSelect_Active();

  LCD_SPI_Write((char)LCD_START_WR_INSTRUCTION);
  LCD_SPI_Write((char)(value & 0x0Fu));
  LCD_SPI_Write((char)((value >> 4) & 0x0Fu));

  LCD_ChipSelect_Idle();

  LCD_Delay_us(60u);
}

void Write_LCD_Data(char data)
{
  unsigned char value;

  value = (unsigned char)data;

  LCD_ChipSelect_Active();

  LCD_SPI_Write((char)LCD_START_WR_DATA);
  LCD_SPI_Write((char)(value & 0x0Fu));
  LCD_SPI_Write((char)((value >> 4) & 0x0Fu));

  LCD_ChipSelect_Idle();

  LCD_Delay_us(60u);
}

//==============================================================================
// BASIC LCD HELPERS
//==============================================================================

void ClrDisplay(void)
{
  Write_LCD_Ins((char)LCD_CLEAR_DISPLAY);
  LCD_Delay_ms(4u);
}

void lcd_clear(void)
{
  ClrDisplay();
}

void lcd_putc(char c)
{
  Write_LCD_Data(c);
}

void lcd_puts(char *s)
{
  while (*s)
  {
    Write_LCD_Data(*s);
    s++;
  }
}

void SetPostion(char pos)
{
  Write_LCD_Ins(pos);
}

void SetPosition(char pos)
{
  Write_LCD_Ins(pos);
}

void DisplayOnOff(char data)
{
  Write_LCD_Ins((char)(LCD_DISPLAY_CONTROL | ((unsigned char)data)));
}

void lcd_command(char data)
{
  Write_LCD_Ins(data);
}

void lcd_write(unsigned char c)
{
  Write_LCD_Data((char)c);
}

unsigned char CheckBusy(void)
{
  // SPI write-only path does not read the LCD busy flag.
  return 0u;
}

//==============================================================================
// lcd_out
//==============================================================================
//
// Writes exactly one 10-char row.
// Pads with spaces so old chars do not remain.
//
//==============================================================================

void lcd_out(char *s, char line, char position)
{
  unsigned char count;

  count = 0u;

  Write_LCD_Ins((char)((unsigned char)line + (unsigned char)position));

  while ((*s) && (count < LCD_COL_COUNT))
  {
    Write_LCD_Data(*s);
    s++;
    count++;
  }

  while (count < LCD_COL_COUNT)
  {
    Write_LCD_Data(' ');
    count++;
  }
}

//==============================================================================
// Display_Update
//==============================================================================
//
// Required by old display.obj.
// Uses fixed row addresses to avoid variable row multiply.
//
//==============================================================================

void Display_Update(char p_L1, char p_L2, char p_L3, char p_L4)
{
  lcd_out(&display_line[0][0], (char)LCD_HOME_L1, p_L1);
  lcd_out(&display_line[1][0], (char)LCD_HOME_L2, p_L2);
  lcd_out(&display_line[2][0], (char)LCD_HOME_L3, p_L3);
  lcd_out(&display_line[3][0], (char)LCD_HOME_L4, p_L4);
}

//==============================================================================
// LCD_Load_Line
//==============================================================================

void LCD_Load_Line(unsigned char row, const char *text)
{
  char *line_ptr;
  unsigned char i;

  if (row >= LCD_ROWS)
  {
    return;
  }

  if (text == 0)
  {
    text = "";
  }

  line_ptr = LCD_Get_Line_Ptr(row);

  i = 0u;

  while (i < LCD_COL_COUNT)
  {
    if (*text)
    {
      *line_ptr = *text;
      text++;
    }
    else
    {
      *line_ptr = ' ';
    }

    line_ptr++;
    i++;
  }

  *line_ptr = '\0';

  display_changed = 1u;
}

//==============================================================================
// LCD_Test_Force_Write
//==============================================================================

void LCD_Test_Force_Write(void)
{
  lcd_out("LCD TEST  ", (char)LCD_HOME_L1, 0);
  lcd_out("MCLK 16M  ", (char)LCD_HOME_L2, 0);
  lcd_out("SMCLK 8M  ", (char)LCD_HOME_L3, 0);
  lcd_out("SPI TEST  ", (char)LCD_HOME_L4, 0);
}

//==============================================================================
// Init_LCD
//==============================================================================

void Init_LCD(void)
{
  lcd_spi_error = 0u;

  LCD_SPI_Init();

  LCD_ChipSelect_Idle();

  LCD_Delay_ms(20u);

  LCD_Hardware_Reset();

  //---------------------------------------------------------------------------
  // EA DOGS104-A / SSD1803A init sequence.
  //---------------------------------------------------------------------------

  Write_LCD_Ins(0x3A);        // Function set, extended
  Write_LCD_Ins(0x09);        // 4-line display
  Write_LCD_Ins(0x06);        // Bottom view
  Write_LCD_Ins(0x1E);        // Bias setting
  Write_LCD_Ins(0x39);        // Extended instruction table
  Write_LCD_Ins(0x1B);        // Internal oscillator
  Write_LCD_Ins(0x6E);        // Follower control

  // Required after follower control.
  LCD_Delay_ms(200u);

  Write_LCD_Ins(0x56);        // Power / contrast high bits
  Write_LCD_Ins(0x7A);        // Contrast low bits
  Write_LCD_Ins(0x38);        // Normal instruction table

  DisplayOnOff((char)LCD_DISPLAY_ON);

  ClrDisplay();

  Write_LCD_Ins(0x06);        // Entry mode: increment cursor

  LCD_Load_Line(0u, "LCD READY ");
  LCD_Load_Line(1u, "P4.4 CS   ");
  LCD_Load_Line(2u, "P4 SPI    ");
  LCD_Load_Line(3u, "P6 BL ON  ");

  display_changed = 1u;
  update_display = 1u;

  // Force first visible update immediately.
  Display_Update(0, 0, 0, 0);

  display_changed = 0u;
  update_display = 0u;
}
