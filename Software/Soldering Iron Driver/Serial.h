//http://www.evilmadscientist.com/2009/basics-serial-communication-with-avr-microcontrollers/

#ifndef Serial_H
#define Serial_H

//#define F_CPU 16000000	// 16 MHz oscillator.
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define BaudRate 9600
#define MYUBRR ((F_CPU / 16 / BaudRate / 2) - 1)



unsigned char checkTxReady();
void intSerial();
void serialWriteChar(unsigned char DataOut);
void serialWrightInt(uint16_t number);

unsigned char volatile txserialbuffer [50];  //used to buffer serial rx because the hardware buffer only holds 2 characters



//create tx complete interrupt to check if there is new data to be sent
void intSerial()
{
	/*Set baud rate */
	UBRR0H = (unsigned char)(MYUBRR>>8);
	UBRR0L = (unsigned char) MYUBRR;

	/* Enable receiver and transmitter   */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	/* Frame format: 8data, No parity, 1stop bit */
	UCSR0C = (3<<UCSZ00);
}

unsigned char checkTxReady()
{
	return( UCSR0A & 0x20 ) ;		// nonzero if transmit register is ready to receive new data.
}

void serialWriteChar(unsigned char DataOut)
{
	while (checkTxReady() == 0)		// while NOT ready to transmit
	{;;}
	UDR0 = DataOut;
}

void serialWrightInt(uint16_t number)
{
	char buffer[24];
	sprintf(buffer, "%d", number);
	uint8_t i;
	for (i=0;i<255;i++)
	{
		if(buffer[i]=='\0'){break;}		//if end of string break ('\0' is null)
		serialWriteChar(buffer[i]);
	}
}

void serialWrightString(char *stringtext)
{
	uint8_t i;
	for (i=0;i<255;i++)
	{
		if(stringtext[i]=='\0'){break;}		//if end of string break ('\0' is null)
		serialWriteChar(stringtext[i]);
	}
}


#endif