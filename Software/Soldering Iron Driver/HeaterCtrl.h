/*
 * HeaterCtrl.h
 *
 * Created: 4/27/2014 9:27:18 AM
 *  Author: Bud
 */ 


#ifndef HEATERCTRL_H_
#define HEATERCTRL_H_

	uint16_t volatile softpwmcounter=0;
	uint32_t volatile twentieth_seconds;

	
	
	void updateHeaterPWM(uint8_t h1percent, uint8_t h2percent)
	{
		uint16_t volatile heater1pwmcounts=0;
		uint16_t volatile heater2pwmcounts=0;
		heater1pwmcounts = 2.4*h1percent;
		heater2pwmcounts = 2.4*h2percent;
		
		if (!AC_DC_Select)	//if ==1 AC if ==0 DC
		{
			//pwm stuff--------------------------
	
			if (!heater1onoff)	//if heater is off tell pwm to coolit
			{
	
				PORTB &= ~(1<<Heater1); //turn heater off
		
			}else{		//if the heater is on we may need to apply power
				if (softpwmcounter<=heater1pwmcounts)
				{
					PORTB |= (1<<Heater1);//set heater on
				}else
				{
					PORTB &= ~(1<<Heater1); //turn heater off
				}
			}
	
	
			if (!heater2onoff)	//if heater is off tell pwm to coolit
			{
		
				PORTB &= ~(1<<Heater2); //turn heater off
		
			}else{
				if ( softpwmcounter >= (240 - heater2pwmcounts) )	//uses back side of pwm cycle to reeduce peek current when possable
				{
					PORTB |= (1<<Heater2);//set heater on
				}else
				{
					PORTB &= ~(1<<Heater2); //turn heater off
				}
			}
	
	
			if (softpwmcounter<240)//240 counts max because the adc is running at 125khz with a prescaler of 64 and each convertion takes 13 cycles giveing us a pwm of roufly 20hz
			{
				softpwmcounter++;
			}else
			{
				softpwmcounter=0;
				twentieth_seconds++;
				if (twentieth_seconds%400==0)	//every thirty seckonds	check to see if the heater temp has changed and if so update it in eeprom	****************************************************************
				{
					eeprom_update_word (&eepromsettempheater1,settempheater1);	//put settempheater1 into eepromsettempheater1 if it has changed
					eeprom_update_word (&eepromsettempheater2,settempheater2);	//put settempheater2 into eepromsettempheater2 if it has changed
				}
				/*
				if (twentieth_seconds%36000==0)	//every thirty minits check to see if the heater has been used****************************************************************
				{
					//heater standby code here-----------------`--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
				}
				*/
			}
			//end pwm stuff----------------------
		}else
		{
		//AC Heater Pwm Update------------------------------------------------------
		}
	}
	
	
	
	
	
	


#endif /* HEATERCTRL_H_ */