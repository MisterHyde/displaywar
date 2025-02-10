/**
 *  \file lcd_w162_x9lg.c
 *
 *  \brief Code to communicate with the W162_X9LG OLED display.
 *
 *  Things to consider and do:
 *      - Currently only a successive pin number on the same port for the DB4-7 will work.
 *      - Also the line break does not work properly. There is always a character missing.
 *      - Another bug is that I was not able to set the DDRAM to 0xF and write something.
 *        If this was done nothing was displayed on the last segment of the first line.
 *        I have not read out the content of the DDRAM at this address.
 *  It's not working perfect but how Mediocrates has said: "Meh, good enough"!
 */
#include <avr/io.h>
#include <util/delay.h>

/////////////////////////////////////////////////////////////////////////////
// Defines
/////////////////////////////////////////////////////////////////////////////
// Define magic
#define DDR(x) (*(&x - 1))      /* address of data direction register of port x */
#if defined(__AVR_ATmega64__) || defined(__AVR_ATmega128__)
    /* on ATmega64/128 PINF is on port 0x00 and not 0x60 */
    #define PIN(x) ( &PORTF==&(x) ? _SFR_IO8(0x00) : (*(&x - 2)) )
#else
	#define PIN(x) (*(&x - 2))    /* address of input register of port x          */
#endif

// Pin defintions
#ifndef LCD_USER_PINS
#define LCD_DATA4_PORT PORTD
#define LCD_DATA4_PIN 0
#define LCD_DATA5_PORT PORTD
#define LCD_DATA5_PIN 1
#define LCD_DATA6_PORT PORTD
#define LCD_DATA6_PIN 2
#define LCD_DATA7_PORT PORTD
#define LCD_DATA7_PIN 3

#define LCD_RS_DDR DDRD
#define LCD_RS_PORT PORTD
#define LCD_RS_PIN 7
#define LCD_RW_DDR DDRD
#define LCD_RW_PORT PORTD
#define LCD_RW_PIN 6
#define LCD_E_DDR DDRD
#define LCD_E_PORT PORTD
#define LCD_E_PIN 5
#endif /*LCD_USER_PINS*/

// Delay times
#define LCD_DELAY_BOOTUP 16000
#define LCD_DELAY_INIT 5000
#define LCD_DELAY_INIT_4BIT 64
#define LCD_DELAY_INIT_REP 64
#define LCD_DELAY_BUSY_FLAG 4
#define LCD_DELAY_ENABLE_PULSE 1

// instruction register bit positions, see HD44780U data sheet
#define LCD_INSTR_CLR_DISP                          0
#define LCD_INSTR_RETURN_HOME                       1
#define LCD_INSTR_ENTRY_MODE                        2
#define LCD_INSTR_ENTRY_MODE_INC                        1
#define LCD_INSTR_ENTRY_MODE_SHIFT                      0
#define LCD_INSTR_DISP_ONOFF_CTRL                   3
#define LCD_INSTR_DISP_ONOFF_CTRL_DISP_ON               2
#define LCD_INSTR_DISP_ONOFF_CTRL_CURSOR_ON             1
#define LCD_INSTR_DISP_ONOFF_CTRL_CURSOR_BLINK          0
#define LCD_INSTR_SET_INTERNAL_POWER_CURSOR_DISP    4
#define LCD_INSTR_SET_INTERNAL_POWER_CURSOR_DISP1       0
#define LCD_INSTR_SET_INTERNAL_POWER_CURSOR_DISP2       1
#define LCD_INSTR_SET_INTERNAL_POWER_CURSOR_DISP3       2
#define LCD_INSTR_FUNCTION_SET                      5
#define LCD_INSTR_FUNCTION_SET_8_BIT_MODE               4
#define LCD_INSTR_FUNCTION_SET_TWO_LINE_DISPLAY         3
#define LCD_INSTR_FUNCTION_SET_CHAR_FONT_SET            2
#define LCD_INSTR_FUNCTION_SET_CHAR_FONT_SET_FT1        1
#define LCD_INSTR_FUNCTION_SET_CHAR_FONT_SET_FT0        0
#define LCD_INSTR_SET_CGDRAM_ADDRESS                6
#define LCD_INSTR_SET_DDDRAM_ADDRESS                7

#define LCD_INSTR_DDRAM         7 /* Data bit 7: Set DD RAM address */
#define LCD_INSTR_READ_BUSY          7 /* Data bit 7: Read busy flag */

// Set pins
#define lcd_e_delay()   _delay_us(LCD_DELAY_ENABLE_PULSE)
#define lcd_e_high()    LCD_E_PORT  |=  _BV(LCD_E_PIN);
#define lcd_e_low()     LCD_E_PORT  &= ~_BV(LCD_E_PIN);
#define lcd_rw_high()   LCD_RW_PORT |=  _BV(LCD_RW_PIN)
#define lcd_rw_low()    LCD_RW_PORT &= ~_BV(LCD_RW_PIN)
#define lcd_rs_low()    LCD_RS_PORT &= ~_BV(LCD_RS_PIN);
#define lcd_rs_high()   LCD_RS_PORT |= _BV(LCD_RS_PIN);

// Other defines
#define LCD_START_LINE1     0x0
#define LCD_START_LINE2     0x40

#define LCD_DISP_LENGTH     16

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
static uint8_t lcd_read_instr(void);
static uint8_t lcd_wait_busy(void);
static void lcd_send_nibble(uint8_t data);
static void lcd_write_instr(uint8_t data);

/////////////////////////////////////////////////////////////////////////////
// Functions
/////////////////////////////////////////////////////////////////////////////

// Just for debugging
#ifdef DEBUGING
static void setBinaryLeds(uint8_t data)
{
    DDRC |= _BV(PC5);
    DDRC |= _BV(PC4);
    DDRC |= _BV(PC3);
    DDRC |= _BV(PC2);
    DDRC |= _BV(PC1);
    DDRC |= _BV(PC0);
    DDRB |= _BV(PB2);
    DDRB |= _BV(PB0);

    if(data & 0x01)
    {
        PORTC |= _BV(PC5);
    }
    else
    {
        PORTC &= ~_BV(PC5);
    }
    if(data & 0x02)
    {
        PORTC |= _BV(PC4);
    }
    else
    {
        PORTC &= ~_BV(PC4);
    }
    if(data & 0x04)
    {
        PORTC |= _BV(PC3);
    }
    else
    {
        PORTC &= ~_BV(PC3);
    }
    if(data & 0x08)
    {
        PORTC |= _BV(PC2);
    }
    else
    {
        PORTC &= ~_BV(PC2);
    }
    if(data & 0x10)
    {
        PORTC |= _BV(PC1);
    }
    else
    {
        PORTC &= ~_BV(PC1);
    }
    if(data & 0x20)
    {
        PORTC |= _BV(PC0);
    }
    else
    {
        PORTC &= ~_BV(PC0);
    }

    if(data & 0x40)
    {
        PORTB |= _BV(PB2);
    }
    else
    {
        PORTB &= ~_BV(PB2);
    }
    if(data & 0x80)
    {
        PORTB |= _BV(PB0);
    }
    else
    {
        PORTB &= ~_BV(PB0);
    }
}
#endif

/**
 *  \brief Function name is pretty self explanatory: it reads the address
 *         of the DDRAM/CGRAM and the busy flag
 *
 *  \return Current address of DDRAM/CGRAM
 */
static uint8_t lcd_read_instr(void)
{
    uint8_t data;

    lcd_rs_low();
    lcd_rw_high();

    DDR(LCD_DATA4_PORT) &= ~_BV(LCD_DATA4_PIN);         /* configure data pins as input */
    DDR(LCD_DATA5_PORT) &= ~_BV(LCD_DATA5_PIN);         /* configure data pins as input */
    DDR(LCD_DATA6_PORT) &= ~_BV(LCD_DATA6_PIN);         /* configure data pins as input */
    DDR(LCD_DATA7_PORT) &= ~_BV(LCD_DATA7_PIN);         /* configure data pins as input */

    lcd_e_high();
    lcd_e_delay();        
    data = PIN(LCD_DATA4_PORT) << 4;     /* read high nibble first */
    lcd_e_delay();
    lcd_e_low();
    
    lcd_e_delay();                       /* Enable 500ns low       */
    
    lcd_e_high();
    lcd_e_delay();
    data |= PIN(LCD_DATA4_PORT)&0x0F;    /* read low nibble        */
    lcd_e_delay();
    lcd_e_low();

    return data;
}

/**
 *  \brief Wait for busy flag turn low and than return the 
 *         address of the last character written (not the curser pos)
 *
 *  \return Address of the last written character
 */
static uint8_t lcd_wait_busy(void)
{
    uint8_t data;

    do 
    {
        data = lcd_read_instr();
    } while( data & _BV(LCD_INSTR_READ_BUSY));

    _delay_us(LCD_DELAY_BUSY_FLAG);

#ifdef DEBUGING
    setBinaryLeds(data);
#endif

    return lcd_read_instr() & 0x7F;
}

/**
 *  \brief Send 4 bits to the LCD
 *
 *  \param The for bits that should be sent
 */
static void lcd_send_nibble(uint8_t data)
{
    // Set data pins as output
    DDR(LCD_DATA4_PORT) |= _BV(LCD_DATA4_PIN);
    DDR(LCD_DATA5_PORT) |= _BV(LCD_DATA5_PIN);
    DDR(LCD_DATA6_PORT) |= _BV(LCD_DATA6_PIN);
    DDR(LCD_DATA7_PORT) |= _BV(LCD_DATA7_PIN);

    LCD_DATA4_PORT &= 0xF0;
    LCD_DATA4_PORT |= data & 0x0F;

    lcd_e_high();
    lcd_e_delay();
    lcd_e_low();
    lcd_e_delay();
}

/**
 *  \brief Uses the lcd_send_nibble() function to send data to
 *         the instruction register of the LCD
 *  \param The instruction which shall be sent
 */
static void lcd_write_instr(uint8_t data)
{
    lcd_wait_busy();
    lcd_rs_low();
    lcd_rw_low();
    _delay_us(1);
    lcd_send_nibble((data & 0xF0) >> 4);
    lcd_send_nibble(data & 0x0F);
    _delay_us(10);
}

/**
 *  \brief Uses the lcd_send_nibble() function to send data to
 *         the DDRAM/CGRAM of the LCD. 
 *  \param The data which shall be sent
 */
void lcd_write(uint8_t data)
{
    uint8_t pos = LCD_START_LINE1;
    pos = lcd_wait_busy();
    lcd_rs_high();
    lcd_rw_low();
 
    if( pos == 0x10 ) 
    {
        lcd_write_instr(0xC0);    
    }
    else if( pos == 0x50 )
    {
        lcd_write_instr(0x80);
    }

    lcd_send_nibble((data & 0xF0) >> 4);
    lcd_send_nibble(data & 0x0F);
}

void lcd_init(void)
{ 
    LCD_RS_DDR |= _BV(LCD_RS_PIN);
    LCD_RW_DDR |= _BV(LCD_RW_PIN);
    LCD_E_DDR |= _BV(LCD_E_PIN);

    _delay_us(LCD_DELAY_BOOTUP);

    lcd_rs_low();
    lcd_rw_low();

    // Be sure to be in 8-bit mode
    lcd_send_nibble(0x3);
    _delay_us(LCD_DELAY_INIT);
    // Repeat it because the example code says it so...
    lcd_send_nibble(0x3);
    _delay_us(LCD_DELAY_INIT_REP);
    // Repeat it again because the example code says it so...
    lcd_send_nibble(0x3);
    _delay_us(LCD_DELAY_INIT_REP);
    // Now go to 4-bit mode
    lcd_send_nibble(0x2);
    _delay_us(LCD_DELAY_INIT_4BIT);

    // Set two line display and 4 bit mode
    lcd_write_instr(_BV(LCD_INSTR_FUNCTION_SET) | _BV(LCD_INSTR_FUNCTION_SET_TWO_LINE_DISPLAY));
    // Turn display off
    lcd_write_instr(_BV(LCD_INSTR_DISP_ONOFF_CTRL));
    // Increment cursor position (DDRAM address) on write
    lcd_write_instr(_BV(LCD_INSTR_ENTRY_MODE) | _BV(LCD_INSTR_ENTRY_MODE_INC));
    // Set internal power on (DCDC on)
    lcd_write_instr(_BV(LCD_INSTR_SET_INTERNAL_POWER_CURSOR_DISP)
                    | _BV(LCD_INSTR_SET_INTERNAL_POWER_CURSOR_DISP1)
                    | _BV(LCD_INSTR_SET_INTERNAL_POWER_CURSOR_DISP2)
                    | _BV(LCD_INSTR_SET_INTERNAL_POWER_CURSOR_DISP3));
    // Clear display
    lcd_write_instr(_BV(LCD_INSTR_CLR_DISP));
    // Return home
    lcd_write_instr(_BV(LCD_INSTR_RETURN_HOME));
    // Set display on and cursor on and blinking
    lcd_write_instr(_BV(LCD_INSTR_DISP_ONOFF_CTRL)
                    | _BV(LCD_INSTR_DISP_ONOFF_CTRL_DISP_ON)
                    | _BV(LCD_INSTR_DISP_ONOFF_CTRL_CURSOR_ON)
                    | _BV(LCD_INSTR_DISP_ONOFF_CTRL_CURSOR_BLINK));
}

// Clears 
void lcd_clear_display(void)
{
    lcd_write_instr(_BV(LCD_INSTR_CLR_DISP));
}

void lcd_return_home(void)
{
    lcd_write_instr(_BV(LCD_INSTR_RETURN_HOME));
}

void lcd_display_off(void)
{
    lcd_write_instr(_BV(LCD_INSTR_DISP_ONOFF_CTRL));
}

void lcd_display_on(void)
{
    lcd_write_instr(_BV(LCD_INSTR_DISP_ONOFF_CTRL) | _BV(LCD_INSTR_DISP_ONOFF_CTRL_DISP_ON));
}

void lcd_write_ddram_addr(uint8_t data)
{
    if( data <= 0x67 )
    {
        lcd_write_instr((1 << LCD_INSTR_SET_DDDRAM_ADDRESS) + data);
    }
}
