/*
 * Soldering_Iron_Driver_cplusplus.cpp
 *
 * Created: 1/24/2014 5:36:29 PM
 *  Author: Bud Arriola
 
 
 
 
 
 
 --todo***********************
 tune pid
 create cal menu
 create celcius option
 make it save last set temp every 30 seckonds if it has changed
 make case
 add standby code


 *****************************
 

 
 
 ioport hardware deffinitions-----------------------------------
 
 oxing dector = pcint0 pb0 pin 12
 
 Temp1 = adc6 pin 19
 Temp2 = adc7 pin 22
 
 serial txd = pd1 pin 31
 serial rxd = pd0 pin 30
 */
#define AC_DC_Select 0	//if ==1 AC if ==0 DC
#define Sw1Mask 0x10		//pb4 pin 16	
#define Sw2Mask 0x08		//pb3 pin 15	
#define Sw_optMask 0x20	//pb5 pin 17	
#define Sw3Mask 0x08		//pc3 pin 26	
#define Sw4Mask 0x04		//pc2 pin 25	

#define Temp1 6 //pin 19
#define Temp2 7 //pin 22
 
#define Heater1 PB1	//pb1 pin 13
#define Heater2 PB2	//pb2 pin 14

#define Led_D2 PC1	//pc1 pin 24
#define Led_D3 PC0	//pc0 pin 23
/*
#define lcd_db7 PD2	//pd2 pin 32
#define lcd_db6 PD3	//pd3 pin 1
#define lcd_db5 PD4	//pd4 pin 2
#define lcd_db4 PD7	//pd7 pin 11
#define lcd_e PD5	//pd5 pin 9
#define lcd_rs PD6	//pd6 pin 10
*/
 //-------------------------------------------------------
  


//glolbal includes and deffineitions
#define F_CPU 8000000UL		//8Mhz internal Rc osc or 16mhz xtal with a /2 prescaler
#include <avr/io.h>			//inport pin deffinitions
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h> 

//global variables
uint8_t heater1onoff=0;
uint8_t heater2onoff=0;

uint16_t volatile numofreadings=0;	//stores current number of adc readings that have been taken
float volatile rawADCtemp1=0;	//the current avraged value of temp 1
float volatile rawADCtemp2=0;	//the current avraged value of temp 2

uint8_t volatile ADCtempnew=0;	//the current avraged value of temp 1
uint16_t volatile ADCtemp1avg=0;	//the avraged value of temp 1 flaged by ADCtempnew
uint16_t volatile ADCtemp2avg=0;	//the avraged value of temp 2 flaged by ADCtempnew

uint16_t volatile settempheater1=0;  
uint16_t volatile settempheater2=0;	
uint16_t EEMEM eepromsettempheater1;
uint16_t EEMEM eepromsettempheater2;

uint8_t heater1percent=0;
uint8_t heater2percent=0;

#define heater1_f_offset_temp -4	//if the display is higher than reality then subtract if the display is less than real temp then add in ferenhight
#define heater2_f_offset_temp -4	//if the display is higher than reality then subtract if the display is less than real temp then add in ferenhight
#define heater1_adcgain 111.23		//gain of opamp stages
#define heater2_adcgain 111.23		//gain of opamp stages
#define heater1_adcreff 2.5			//reffrence voltage used by adc
#define heater2_adcreff 2.5

char lcdtext[24];	//holds charters for output to lcd


//glolbal includes and deffineitions
#include "LCD.h"
#include "Serial.h"
#include "Thermocouple.h"
#include "HeaterCtrl.h"
#include "PID.h"




//ADC conversion complete ISR
ISR(ADC_vect)//125khz it takes 13 cycles for the adc to compleat and there are 2 channels (((125000/13)/2)/2) = 4806 samples per seckond
{

	
	updateHeaterPWM(heater1percent,heater2percent);
	
	if(Temp1==(ADMUX & 0x0F))	//if the convertion is from temp1
	{
		rawADCtemp1=(rawADCtemp1+ADC);	//read the new adc value and avrage it into rawADCtemp1
		ADMUX=Temp2;	//set temp2 to be read flag
		numofreadings++;
		

	}
	else    //The reading is from temp2
	{
		rawADCtemp2=(rawADCtemp2+ADC);	//read the new adc value and avrage it into rawADCtemp2
		ADMUX=Temp1;	//set temp2 to be read flag
		if(numofreadings==2400)	//do this at 4hz adc operates at 125khz it takes 13 cycles for the adc to compleat and there are 2 channels (((125000/13)/2)/2) = 4806 samples per seckond
		{
			//clear the values for the next go around
			ADCtemp1avg=(rawADCtemp1/2400);
			ADCtemp2avg=(rawADCtemp2/2400);
			rawADCtemp1=0;
			rawADCtemp2=0;
			numofreadings=0;
			ADCtempnew=1;
			
			//pid calculations here---------------------------------------------------------------------------------
			uint16_t sendtemp1=tempTouV(settempheater1);
			uint16_t sendtemp2=tempTouV(settempheater2);
			
			if (!heater1onoff){sendtemp1=0;}	//if heater is off tell pid to coolit
			if (!heater2onoff){sendtemp2=0;}	//if heater is off tell pid to coolit
				
			heater1percent=calculateOutput(adctouV(ADCtemp1avg,10,heater1_adcgain,heater1_adcreff), sendtemp1);//sets the counter for a spicific duty cycle 100=100% 0=0%	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! check this !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			heater2percent=h2calculateOutput(adctouV(ADCtemp2avg,10,heater2_adcgain,heater2_adcreff), sendtemp2);//sets the counter for a spicific duty cycle 100=100% 0=0%	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! check this !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

			

		}
	}
	
	ADCSRA |= 1<<ADSC;		// Start Conversion
}
	












int main(void)
{
	//CLKPR=0b10000000;	//set main clock prescaler to 1:1 for internal rc oslator
	CLKPR=0x80;	//set system clock prescaler to run at 8mhz form 16mhz xtal so after development the xtal may be omitted
	CLKPR=0x02;//set system clock prescaler to run at 8mhz form 16mhz xtal so after development the xtal may be omitted
	
	
	//config simple io-----------------------------------------------------------------
	DDRB=0b00000110;	//set heaters as outputs
	DDRC=0b00000011;	//set leds as outputs
	DDRD=0b11111100;	//set lcd as outputs
	PORTB=0b00111000;	//add soft pull ups to switches 1 and 2
	PORTC=0b00001100;	//add soft pull ups to switches 3 and 4
	intSerial();
	intLCD();




	if (settempheater1==0)
	{
		settempheater1=eeprom_read_word(&eepromsettempheater1);	//reads heater set value from eeprom into ram
		if (settempheater1<1 || settempheater1>850)	//if there was not a valid temp then put the defualt
		{
			settempheater1=600;		//sets defualt heater value in ram
			eeprom_update_word (&eepromsettempheater1,settempheater1);	//put settempheater1 into eepromsettempheater1
		}
	}

	if (settempheater2==0)
	{
		settempheater2=eeprom_read_word(&eepromsettempheater2);	//reads heater set value from eeprom into ram
		if (settempheater2<1 || settempheater2>850)	//if there was not a valid temp then put the defualt
		{
			settempheater2=600;		//sets defualt heater value in ram
			eeprom_update_word (&eepromsettempheater2,settempheater2);	//put settempheater2 into eepromsettempheater2
		}
	}






	//http://www.avr-tutorials.com/adc/utilizing-avr-adc-interrupt-feature
	PRR &= (0<<PRADC); //make sure the adc is not disabled by power management
	ADCSRA |= ((1<<ADPS2)|(1<<ADPS1)|(1<<ADEN))|(1<<ADIE);    //Prescaler = 64 = 125Khz clock source using 8Mhz internal, turn adc on, ADC Interrupt Enable
	//ADMUX &= ~((1<<REFS1) | (1<<REFS0));	//(00=AREF, Internal Vref turned off, externally connected voltage)
	//ADMUX |= (1<<REFS1) | (1<<REFS0);		//(11=Internal 1.1V Voltage Reference with external capacitor at AREF pin)
	sei();	//enable global interupts
	ADMUX=Temp1;	//set temp1 to be read first
	ADCSRA |= (1<<ADSC);	//ADC Start Conversion
	








	
	uint16_t sw1count=0;	//takes care of debounceing buttons and alt functions
	uint16_t sw2count=0;	//takes care of debounceing buttons and alt functions
	uint16_t sw3count=0;	//takes care of debounceing buttons and alt functions
	uint16_t sw4count=0;	//takes care of debounceing buttons and alt functions
	uint8_t selectediron=1;		//holds witch iron is selected on the lcd defualt to iron 1
	
	//wright persistant text to the lcd
	setCursor(0,0);
	wrightString(">S");
	setCursor(1,0);
	wrightString(" S");
	setCursor(0,6);
	wrightString("A");
	setCursor(1,6);
	wrightString("A");
	setCursor(0,10);
	wrightString("F");
	setCursor(0,15);
	wrightString("%");
	setCursor(1,10);
	wrightString("F");
	setCursor(1,15);
	wrightString("%");
	setCursor(0,2);
	wrightString(dtostrf(settempheater1,3,0,lcdtext));	//turn temp into a string);
	setCursor(1,2);
	wrightString(dtostrf(settempheater2,3,0,lcdtext));	//turn temp into a string);	
	
	//serial pid tuning stuff for use with plx-daq addon form parelax
	//http://robottini.altervista.org/arduino-and-real-time-charts-in-excel?doing_wp_cron=1382484593.4194939136505126953125
	//http://classic.parallax.com/downloads/plx-daq
	serialWrightString("CLEARDATA");
	serialWriteChar(10);	//send new line (10)
	serialWrightString("LABEL,Time,Heater 1 Temp, Heater 1 Power, Heater 2 Temp, Heater 2 Power,Heater 1 Error,Heater 1 I,Heater 1 D,Heater 2 Error,Heater 2 I,Heater 2 D");	//set headers in the graph
	serialWriteChar(10);	//send new line (10)
	
    while(1)
    {
		
		if(ADCtempnew)	//if there is a new filterd adc temp update lcd
		{
			ADCtempnew=0;//clear flag
			
			serialWrightString("DATA,TIME,"); //serial graphing stuff
			
			dtostrf(adctoTemp(ADCtemp1avg, 1, heater1_f_offset_temp, 10, heater1_adcgain, heater1_adcreff),3,0,lcdtext);	//turn temp into a string
			setCursor(0,7);
			wrightString(lcdtext);	//output heater 1 temp to lcd
			
			serialWrightString(lcdtext);	//heater 1 temp to serial
			serialWrightString(",");
			
			dtostrf(heater1percent,3,0,lcdtext);
			setCursor(0,12);
			wrightString(lcdtext);			//output current iron power to lcd
			
			serialWrightString(lcdtext);	//heater1  power to serial
			serialWrightString(",");
			
			dtostrf(adctoTemp(ADCtemp2avg, 1, heater2_f_offset_temp, 10, heater2_adcgain, heater2_adcreff),3,0,lcdtext); //turn temp into a string
			setCursor(1,7);
			wrightString(lcdtext);	//output heater 2 temp to bottom left
			
			serialWrightString(lcdtext);	//heater 2 temp to serial
			serialWrightString(",");
			
			
			dtostrf(heater2percent,3,0,lcdtext);
			setCursor(1,12);
			wrightString(lcdtext);			//output current iron power next to temp
			serialWrightString(lcdtext);	//heater 2 power to serial
			serialWrightString(",");
			
			
			//send pid statis over serial
			dtostrf(pid_error * pid_p,6,0,lcdtext);
			serialWrightString(lcdtext);	//heater 1 pid error to serial
			serialWrightString(",");
			
			dtostrf(integral * pid_i,6,0,lcdtext);
			serialWrightString(lcdtext);	//heater 1 pid intrigral to serial
			serialWrightString(",");
			
			dtostrf(derivative * pid_d,6,0,lcdtext);
			serialWrightString(lcdtext);	//heater 1 pid derivative to serial
			serialWrightString(",");
			
			dtostrf(h2pid_error * pid_p,6,0,lcdtext);
			serialWrightString(lcdtext);	//heater 2 pid error to serial
			serialWrightString(",");
			
			dtostrf(h2integral * pid_i,6,0,lcdtext);
			serialWrightString(lcdtext);	//heater 2 pid intrigral to serial
			serialWrightString(",");
			
			dtostrf(h2derivative * pid_d,6,0,lcdtext);
			serialWrightString(lcdtext);	//heater 2 pid derivative to serial

			serialWriteChar(10);	//send new line (10) to end serial string
			
			
		}
		



		//short button press selects the channel (sw3/4)
		//long button press turns the channel on and off (sw3/4)
		//if selected channel is on change temp up and down (sw1/2)
		
		
		//short button press selects the channel (sw3/4)
		//long button press turns the channel on and off (sw3/4)
		//if selected channel is on change temp up and down (sw1/2)
		
		
		//read buttons
		uint8_t inputc,inputb;
		inputc=PINC;
		inputb=PINB;
		
		if (~inputc & Sw4Mask)	//if sw4 is low -----------------------------heater 1 select / on off
		{
			
			if (sw4count<65535){sw4count++;}
				
			if (sw4count==20)
			{
				selectediron=1;
				setCursor(0,0);
				wrightString(">");
				setCursor(1,0);
				wrightString(" ");
			}
			if (sw4count==65000)	//if sw4 is held
			{
				if (heater1onoff==1)	//if the heater is on so turn heater off
				{
					heater1onoff=0;
					PORTC &= ~(1<<Led_D3);
				} 
				else	//the heater is off so turn the heater on
				{
					heater1onoff=1;
					PORTC |= (1<<Led_D3);//set led d3 on
				}
				
			}
			
			
		}else
		{
			sw4count=0;
		}
		
		
		
		
		
		
		if (~inputc & Sw3Mask)	//if sw3 is low ----------------------------------heater 2 select / on off
		{
			
			if (sw3count<65535){sw3count++;}
				
			if (sw3count==20)
			{
				selectediron=2;
				setCursor(0,0);
				wrightString(" ");
				setCursor(1,0);
				wrightString(">");
			}
			if (sw3count==65000)	//if sw3 is held
			{
				if (heater2onoff==1)	//if the heater is on
				{
					heater2onoff=0;	//turn heater off
					PORTC &= ~(1<<Led_D2);

				} 
				else	//the heater is off
				{
					heater2onoff=1;	//turn heater on
					PORTC |= (1<<Led_D2);//set led d2 on
				}
				
			}
			
			
		}else
		{
			sw3count=0;
		}
		
		
		
		
		if (~inputb & Sw1Mask)	//if sw3 is low ------------------------temp down
		{
			
			if (sw1count<65535){sw1count++;}
				
			if (sw1count==20)	//if short press
			{
				//----------short press
				

				
				
				if (selectediron==1 && settempheater1>1)
				{
					//decrece the set temp of heater 1
					settempheater1--;
					setCursor(0,2);
					wrightString(dtostrf(settempheater1,3,0,lcdtext)); //turn temp into a string);
	
				}else if(selectediron==2 && settempheater2>1){
					//decrece the set temp of heater 2
					settempheater2--;
					setCursor(1,2);
					wrightString(dtostrf(settempheater2,3,0,lcdtext));	//turn temp into a string);
					
				}
			}
			
			
			if (sw1count>65000)	//if sw1 is held
			{
				//long press----------------
				if (selectediron==1 && settempheater1>6)
				{
					//decrece the set temp of heater 1
					settempheater1-=5;
					setCursor(0,2);
					wrightString(dtostrf(settempheater1,3,0,lcdtext)); //turn temp into a string);
					
				}else if(selectediron==2 && settempheater2>6){
					//decrece the set temp of heater 2
					settempheater2-=5;
					setCursor(1,2);
					wrightString(dtostrf(settempheater2,3,0,lcdtext)); //turn temp into a string);
					
				}
			}
			
			
		}else
		{
			sw1count=0;
		}
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		if (~inputb & Sw2Mask)	//if sw3 is low ------------------------temp up
		{
			
			if (sw2count<65535){sw2count++;}
				
			if (sw2count==20)	//if short press
			{
				//----------short press
			
				if (selectediron==1 && settempheater1<850)
				{
					//decrece the set temp of heater 1
					settempheater1++;
					setCursor(0,2);
					wrightString(dtostrf(settempheater1,3,0,lcdtext)); //turn temp into a string);
	
				}else if(selectediron==2 && settempheater2<850){
					//decrece the set temp of heater 2
					settempheater2++;
					setCursor(1,2);
					wrightString(dtostrf(settempheater2,3,0,lcdtext));	//turn temp into a string);
					
				}
			}
			
			
			if (sw2count>65000)	//if sw1 is held
			{
				//long press----------------
				if (selectediron==1 && settempheater1<845)
				{
					//decrece the set temp of heater 1
					settempheater1+=5;
					setCursor(0,2);
					wrightString(dtostrf(settempheater1,3,0,lcdtext)); //turn temp into a string);
					
				}else if(selectediron==2 && settempheater2<845){
					//decrece the set temp of heater 2
					settempheater2+=5;
					setCursor(1,2);
					wrightString(dtostrf(settempheater2,3,0,lcdtext)); //turn temp into a string);
					
				}
			}

		}else
		{
			sw2count=0;
		}
		
		
		
		
		
		
		
    }

}



/*
//debug rutine---------------------------------------------
serialWriteChar(10);	//send new line (10)
serialWriteChar(10);	//send new line (10)
serialWriteChar(10);	//send new line (10)
serialWrightString("selectediron==");
dtostrf(selectediron,3,0,lcdtext);
serialWrightString(lcdtext);
serialWriteChar(10);	//send new line (10)
serialWrightString("settempheater1==");
dtostrf(settempheater1,3,0,lcdtext);
serialWrightString(lcdtext);
serialWriteChar(10);	//send new line (10)
serialWriteChar(10);	//send new line (10)
serialWriteChar(10);	//send new line (10)
//end debug rutine---------------------------------------------
*/


/*notes section


http://support.atmel.com/bin/customer.exe?=&action=viewKbEntry&id=368
Set PB6 and PB4, and leave the other pins unchanged:
PORTB |= (1<<PB6) | (1<<PB4);

Clear PB6 and PB4, and leave the other pins unchanged:
PORTB &= ~((1<<PB6) | (1<<PB4));

Check if the PB6 pin is set:
if (PORTB &= (1<<PB6))
{
//Do something, for instance set a value in a variable
}

Check if PB6 and PB4 is set:
if ((PORTB &= ((1<<PB6) | (1<<PB4))) == ((1<<PB6) | (1<<PB4)))
{
//Do something, for instance set a value in a variable
}	
		
http://maxembedded.com/2011/06/10/port-operations-in-avr/
DDRx – Data Direction Register 1=output
PORTx – Pin Output Register/ input pullup
PINx – Pin Input Register
ADMUX = 0b01000111;
ADMUX = 0x47;


http://hekilledmywire.wordpress.com/2011/03/16/using-the-adc-tutorial-part-5/
#include <avr/io.h>
int adc_value;        //Variable used to store the value read from the ADC converter
 
int main(void){
 
DDRB |= (1<<PB5);    ///PB5/digital 13 is an output
 
ADCSRA |= ((1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0));    //Prescaler at 128 so we have an 125Khz clock source
ADMUX |= (1<<REFS0);
ADMUX &= ~(1<<REFS1);                //Avcc(+5v) as voltage reference
ADCSRB &= ~((1<<ADTS2)|(1<<ADTS1)|(1<<ADTS0));    //ADC in free-running mode
ADCSRA |= (1<<ADATE);                //Signal source, in this case is the free-running
ADCSRA |= (1<<ADEN);                //Power up the ADC
ADCSRA |= (1<<ADSC);                //Start converting
 
for(;;){            //The infinite loop
 adc_value = ADCW;    //Read the ADC value, really that's just it
 if(adc_value > 512){
 PORTB |= (1<<PB5);    //If ADC value is above 512 turn led on
 }
 else {
 PORTB &= ~(1<<PB5);    //Else turn led off
 }
}
 
return 0;
}
*/