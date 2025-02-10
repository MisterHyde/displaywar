#ifndef _LCD_W162_X9LG_H
#define _LCD_W162_X9LG_H

void lcd_write(uint8_t data);
void lcd_init(void);
void lcd_clear_display(void);
void lcd_return_home(void);
void lcd_display_off(void);
void lcd_display_on(void);
void lcd_write_ddram_addr(uint8_t data);

#endif /*_LCD_W162_X9LG_H*/
