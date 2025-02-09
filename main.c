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

// Delay times
#define LCD_DELAY_BOOTUP 16000
#define LCD_DELAY_INIT 5000
#define LCD_DELAY_INIT_4BIT 64
#define LCD_DELAY_INIT_REP 64
#define LCD_DELAY_BUSY_FLAG 4
#define LCD_DELAY_ENABLE_PULSE 1

// Pin defintions
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
#define LCD_INSTR_SET_INTERNAL_POWER_CURSOR_DISPL   4
#define LCD_INSTR_SET_INTERNAL_POWER_CURSOR_DISPL1      0
#define LCD_INSTR_SET_INTERNAL_POWER_CURSOR_DISPL2      1
#define LCD_INSTR_SET_INTERNAL_POWER_CURSOR_DISPL3      2
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

// Other defines
#define LCD_RS_INSTR_BF 0 /* instruction (write) or busy flag (read) */
#define LCD_RS_DATA 1

#define LCD_START_LINE1     0x0
#define LCD_START_LINE2     0x40

#define LCD_DISP_LENGTH     16

#define BUTTON_DEBOUNCE_TIME 5000
#define BUTTON1_PIN PB1

static inline void lcd_rs_low(void)
{
    LCD_RS_PORT &= ~_BV(LCD_RS_PIN);
}
static inline void lcd_rs_high(void)
{
    LCD_RS_PORT |= _BV(LCD_RS_PIN);
}

void setBinaryLeds(uint8_t data)
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

uint8_t lcd_read_instr(void)
{
    uint8_t data;

    lcd_rs_low();
    lcd_rw_high();

    DDR(LCD_DATA4_PORT) &= ~_BV(LCD_DATA4_PIN);         /* configure data pins as input */
    DDR(LCD_DATA4_PORT) &= ~_BV(LCD_DATA5_PIN);         /* configure data pins as input */
    DDR(LCD_DATA4_PORT) &= ~_BV(LCD_DATA6_PIN);         /* configure data pins as input */
    DDR(LCD_DATA4_PORT) &= ~_BV(LCD_DATA7_PIN);         /* configure data pins as input */

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
uint8_t lcd_wait_busy(void)
{
    uint8_t data;

    do 
    {
        data = lcd_read_instr();
    } while( data & _BV(LCD_INSTR_READ_BUSY));

    _delay_us(10);

    setBinaryLeds(data);

    return data & 0x7F;
}

void lcd_send_nibble(uint8_t data)
{
    // Set data pins as output
    DDRD |= _BV(LCD_DATA4_PIN);
    DDRD |= _BV(LCD_DATA5_PIN);
    DDRD |= _BV(LCD_DATA6_PIN);
    DDRD |= _BV(LCD_DATA7_PIN);

    LCD_DATA4_PORT &= 0xF0;
    LCD_DATA4_PORT |= data & 0x0F;

    lcd_e_high();
    lcd_e_delay();
    lcd_e_low();
    lcd_e_delay();
}

void lcd_write_instr(uint8_t data)
{
    lcd_wait_busy();
    lcd_rs_low();
    lcd_rw_low();
    _delay_us(1);
    lcd_send_nibble((data & 0xF0) >> 4);
    lcd_send_nibble(data & 0x0F);
    _delay_us(10);
}

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

    lcd_rs_low();
    lcd_rw_low();

    // Be sure to be in 8-bit mode
    lcd_send_nibble(0x3);
    _delay_us(1000);
    lcd_send_nibble(0x3);
    _delay_us(1000);
    lcd_send_nibble(0x3);
    _delay_us(1000);
    // Now go to 4-bit mode
    lcd_send_nibble(0x2);

    lcd_write_instr(0x28);
    lcd_write_instr(0x08);
    lcd_write_instr(0x06);
    lcd_write_instr(0x17);
    lcd_write_instr(0x01);
    lcd_write_instr(0x02);
    lcd_write_instr(0x0F);
}

// Clears 
inline void lcd_clear_display(void)
{
    lcd_write_instr(_BV(LCD_INSTR_CLR_DISP));
}

inline void lcd_return_home(void)
{
    lcd_write_instr(_BV(LCD_INSTR_RETURN_HOME));
}

static inline void lcd_display_off(void)
{
    lcd_write_instr(_BV(LCD_INSTR_DISP_ONOFF_CTRL));
}

static inline void lcd_display_on(void)
{
    lcd_write_instr(0x0F);
}

static inline void lcd_write_ddram_addr(uint8_t data)
{
    if( data <= 0x67 )
    {
        lcd_write_instr((1 << LCD_INSTR_SET_DDDRAM_ADDRESS) + data);
    }
}


int main(void)
{
    uint8_t lastButtonState = 0;
    uint8_t buttonState = 0;
    uint32_t lastButtonTimeStamp = 0;
    uint32_t buttonTimeStamp = 0;
    char c = 'H';
    uint8_t charWritten = 0;
    // Life pulse led
    DDRB |= _BV(PB0);
    PORTB |= _BV(PB0);
    DDRB |= _BV(PB2);
    PORTB |= _BV(PB2);

    DDRB &= ~_BV(PB1);


    lcd_init();

    // Delay needed otherwise malfunction of lcd
    _delay_ms(1000);

    while (1)
    {
        uint8_t tempButton = PINB & _BV(PB1);

        buttonTimeStamp++;

        if(tempButton != lastButtonState)
        {
            lastButtonTimeStamp = buttonTimeStamp;
        }

        if( (buttonTimeStamp - lastButtonTimeStamp) > BUTTON_DEBOUNCE_TIME )
        {
            buttonState = tempButton;
            if( buttonState )
            {
                if(charWritten == 0)
                {
                    charWritten = 1;
                    lcd_write(c);
                    if(c == 'z')
                    {
                        c = 'A';
                    }
                    else
                    {
                        c++;
                    }
                }
            }
            else
            {
                charWritten = 0;
            }
        }

        lastButtonState = tempButton;
    }

    return 0;
}
