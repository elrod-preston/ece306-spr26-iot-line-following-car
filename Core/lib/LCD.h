#ifndef LCD_H
#define LCD_H

//==============================================================================
// File Name : LCD.h
//
// Description:
//   DOGS104-A / SSD1803A LCD driver interface.
//   Drop-in replacement for old binary LCD driver symbols.
//
//==============================================================================

#define LCD_ROWS          (4u)
#define LCD_COLS          (10u)
#define LCD_STRING_SIZE   (11u)      // 10 visible chars + '\0'

//==============================================================================
// REQUIRED OLD GLOBALS
//==============================================================================

extern volatile unsigned char update_display;
extern volatile unsigned char display_changed;

extern volatile unsigned char lcd_spi_error;

extern char display_line[LCD_ROWS][LCD_STRING_SIZE];
extern char *display[LCD_ROWS];

//==============================================================================
// LOW-LEVEL LCD FUNCTIONS
//==============================================================================

void LCD_SPI_Init(void);
void LCD_SPI_Write(char byte);

void LCD_ChipSelect_Active(void);
void LCD_ChipSelect_Idle(void);

void LCD_Reset_Active(void);
void LCD_Reset_Idle(void);
void LCD_Hardware_Reset(void);

void Reset_LCD(void);

void Write_LCD_Ins(char instruction);
void Write_LCD_Data(char data);

void ClrDisplay(void);
void lcd_clear(void);

void lcd_putc(char c);
void lcd_puts(char *s);

void SetPostion(char pos);       // original misspelled compatibility function
void SetPosition(char pos);

void DisplayOnOff(char data);

void lcd_command(char data);
void lcd_write(unsigned char c);

unsigned char CheckBusy(void);

void lcd_out(char *s, char line, char position);

//==============================================================================
// REQUIRED OLD LCD API
//==============================================================================

void Init_LCD(void);
void Display_Update(char p_L1, char p_L2, char p_L3, char p_L4);

//==============================================================================
// HELPER FUNCTIONS
//==============================================================================

void LCD_Load_Line(unsigned char row, const char *text);
void LCD_Test_Force_Write(void);

#endif
