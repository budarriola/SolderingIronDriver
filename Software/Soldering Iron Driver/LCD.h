
/*
16x2 lcd driver
By Bud Arriola
r/w on the lcd must be tied to gnd for this to work
*/

//example usage
/*

#include <avr/io.h>			//io port pin definitions
#define F_CPU 8000000UL		//8Mhz internal Rc osc change to 16000000UL for 16Mhz xtal
#include <util/delay.h>
#include "LCD.h"
	
int main(void)
{
	CLKPR=0b10000000;	//set main clock prescaler to 1:1
	DDRD=0b11111100;	//set lcd as outputs

	intLCD();
	
    while(1)//do forever
    {
		char mystring[]="Testing";
		wrightString(mystring);	//wright Testing on the first line
		
		setCursor(1,0);				//go to the second line
		wrightString("123...");		////wright 123... on the second line
		
		_delay_ms(5000);	//delay 5s
		
		clearDisplay();				//clear the display and go to the first line
		
		_delay_ms(5000);	//delay 5s
		
		wrightString("Back in");		////wright Back in on the first line
		
		setCursor(1,0);				//go to the second line
		wrightString("456...");		////wright 456... on the second line
		
		_delay_ms(10000);	//delay 5s
		
		//your code here
	}
}
*/


#ifndef LCD_H
#define LCD_H

//use these definitions to set pins of lcd *************************************************************************
#define lcd_db7 PD2	//pd2 pin 32 on tqfp
#define lcd_db6 PD3	//pd3 pin 1  on tqfp
#define lcd_db5 PD4	//pd4 pin 2  on tqfp
#define lcd_db4 PD7	//pd7 pin 11 on tqfp
#define lcd_e PD5	//pd5 pin 9 on tqfp
#define lcd_rs PD6	//pd6 pin 10  on tqfp

#define lcd_db7_port PORTD	//port that pin exists on
#define lcd_db6_port PORTD	//port that pin exists on
#define lcd_db5_port PORTD	//port that pin exists on
#define lcd_db4_port PORTD	//port that pin exists on
#define lcd_db3_port PORTD	//port that pin exists on
#define lcd_e_port PORTD	//port that pin exists on
#define lcd_rs_port PORTD	//port that pin exists on
//******************************************************************************************************************
//needed for the delays
#include <util/delay.h>

void sendNibble(uint8_t nibble);
void intLCD();
void wrightString(char *stringtext);
void setCursor(uint8_t row, uint8_t column);
void clearDisplay();

//format input as 0x0z where z is the input nibble
void sendNibble(uint8_t nibble)
{
	//set up nibble to be sent
	if (nibble & 0x08) { lcd_db7_port |= (1<<lcd_db7); }
		else { lcd_db7_port &= ~(1<<lcd_db7); }
	
	if (nibble & 0x04) { lcd_db6_port |= (1<<lcd_db6); }
		else { lcd_db6_port &= ~(1<<lcd_db6); }
			
	if (nibble & 0x02) { lcd_db5_port |= (1<<lcd_db5); }
		else { lcd_db5_port &= ~(1<<lcd_db5); }
		
	if (nibble & 0x01) { lcd_db4_port |= (1<<lcd_db4); }
		else { lcd_db4_port &= ~(1<<lcd_db4); }
	
	//send nibble
	_delay_us(500);
	lcd_e_port &= ~(1<<lcd_e);
	_delay_ms(1);
	lcd_e_port |= (1<<lcd_e);
	_delay_ms(1);
}

//get the lcd in a useful state
void intLCD()
{
	//modified int routine outlined in the hd44780 datasheet for applications where the power supply can not be switched to enable the internal reset circuit (Initializing by Instruction)
	
	_delay_ms(100);
	lcd_rs_port &= ~(1<<lcd_rs);	//set rs=0 to send instructions
	
	sendNibble(0x03);	//function interface is 8 bits long
	_delay_ms(5);
	
	sendNibble(0x03);	//function interface is 8 bits long
	_delay_ms(1);
	
	sendNibble(0x03);	//function interface is 8 bits long
	_delay_ms(1);
	
	sendNibble(0x02);	//function set interface 4 bits long interface is 8 bits long
	_delay_ms(1);
	
	sendNibble(0x02);	//function interface is 4 bits long
	sendNibble(0x08);	//set # of display lines and charter font 8*5
	_delay_ms(1);
	
	sendNibble(0x00);	//function interface is 4 bits long
	sendNibble(0x08);	//display off
	_delay_ms(1);
	
	sendNibble(0x00);	//function interface is 4 bits long
	sendNibble(0x01);	//display clear
	_delay_ms(2);
	
	sendNibble(0x00);	//function interface is 4 bits long
	sendNibble(0x06);	////entry mode set increment address by 1  and shift the cursor to the right at the tine of wright to the dd/cgram, dispay is not shifted
	_delay_ms(1);

	sendNibble(0x00);	//function interface is 4 bits long
	sendNibble(0x0E);	//display/cursor on space mode
	_delay_ms(1);
	
	lcd_rs_port |= (1<<lcd_rs);	//prepare to wright data
}

//wrights a string to the display. does not word wrap
void wrightString(char *stringtext)
{
	lcd_rs_port |= (1<<lcd_rs);	//prepare to wright data
	uint8_t i=0;
	uint8_t charnibble=0;
	for (i=0;i<255;i++)
	{
		if(stringtext[i]=='\0'){break;}		//if end of string break ('\0' is null)
		charnibble=((stringtext[i] & 0xF0)>>4);	//get upper nibble of char and move it into the lower nibble for transfer
		sendNibble(charnibble);				//send upper nibble of char
		charnibble=(stringtext[i] & 0x0F);	//get lower nibble of char
		sendNibble(charnibble);				//send lower nibble of char
	}
}

//row 0 is top, column 0 is left
void setCursor(uint8_t row, uint8_t column)
{
	uint8_t byte=(row*0x40)+column+0x80;
	lcd_rs_port &= ~(1<<lcd_rs);	//set rs=0 to send instructions
	
	sendNibble((byte & 0xF0)>>4);	//set ddram address
	sendNibble(byte & 0x0F);	//send lower nibble of dram address
	
	lcd_rs_port |= (1<<lcd_rs);	//prepare to wright data
}

//clear the display and return to 0,0
void clearDisplay()
{
		lcd_rs_port &= ~(1<<lcd_rs);	//set rs=0 to send instructions
		//clear display
		sendNibble(0x00);	//send clear display command
		sendNibble(0x01);	//send lower nibble of dram address
		_delay_ms(2);
		
		lcd_rs_port |= (1<<lcd_rs);	//prepare to wright data
}
#endif