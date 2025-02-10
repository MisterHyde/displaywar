#include <avr/io.h>
#include <util/delay.h>

#include "lcd_w162_x9lg.h"

#define BUTTON_DEBOUNCE_TIME 5000
#define BUTTON1_PIN PB1


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
    lcd_write('F');
    lcd_write('u');
    lcd_write('c');
    lcd_write('k');
    lcd_write('!');

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
