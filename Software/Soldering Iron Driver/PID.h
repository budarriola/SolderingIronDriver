//http://www.codeproject.com/Articles/36459/PID-process-control-a-Cruise-Control-example

#ifndef PID_H
#define PID_H

#define pid_p .0255			//.025
#define pid_i .000175			//.00017
#define pid_d .1			//.1
#define cooldown_i_Setpoint 1164		//1164=50degreesf  23uv per degree
#define cooldown_i_350FSetpower 58823	//amount of i there would be at a set point of 350F
#define cooldown_i_450FSetpower 61764	//amount of i there would be at a set point of 450F
#define cooldown_i_550FSetpower 79411	//amount of i there would be at a set point of 550F
#define cooldown_i_650FSetpower 97058	//amount of i there would be at a set point of 650F
#define cooldown_i_750FSetpower 120588	//amount of i there would be at a set point of 750F
#define cooldown_i_800FSetpower 132352	//amount of i there would be at a set point of 800F

float volatile integral = 0;
float volatile derivative = 0;
float volatile lasterror = 0;
float volatile pid_output=0;
float volatile lastsetpoint=0;
float volatile pid_error =0;

float volatile h2integral = 0;
float volatile h2derivative = 0;
float volatile h2lasterror = 0;
float volatile h2pid_output=0;
float volatile h2lastsetpoint=0;
float volatile h2pid_error =0;
uint8_t coolingflag=0;
uint8_t h2coolingflag=0;

uint8_t calculateOutput(float currentvalue, float setpoint);
uint8_t h2calculateOutput(float currentvalue, float setpoint);

uint8_t calculateOutput(float currentvalue, float setpoint)
{

	if (setpoint!=0)
	{
		if (cooldown_i_Setpoint<currentvalue-setpoint)	//if iron needs to cool down take i away to be sure that iron turns off
		{
			integral=0;
			coolingflag=1;
		}else {											//if the heater is just coming off of a cooling cycle set some integral to be sure that it does not undershoot
			if (coolingflag==1)
			{
				coolingflag=0;
				
				if (setpoint<400*23)
				{
					integral=cooldown_i_350FSetpower;
				}else if (setpoint<500*23)
				{
					integral=cooldown_i_450FSetpower;
				}else if (setpoint<600*23)
				{
					integral=cooldown_i_550FSetpower;
				}else if (setpoint<700*23)
				{
					integral=cooldown_i_650FSetpower;
				}else if (setpoint<800*23)
				{
					integral=cooldown_i_750FSetpower;
				}else
				{
					integral=cooldown_i_800FSetpower;
				}
				
			}
		}
	
	
		pid_error = setpoint-currentvalue;
		integral = integral+pid_error;
		derivative = pid_error - lasterror;
		lasterror=pid_error;
	}else
	{
		integral = 0;
		derivative = 0;
		lasterror = 0;
		pid_error=0;
	}
	
	pid_output=(pid_error*pid_p) + (integral*pid_i) + (derivative*pid_d);
	
	
	if (pid_output>100){pid_output=100;}
	if (pid_output<0){pid_output=0;}
		
	return pid_output;
}

uint8_t h2calculateOutput(float currentvalue, float setpoint)
{

	if (setpoint!=0)
	{
		if (cooldown_i_Setpoint<currentvalue-setpoint)	//if iron needs to cool down take i away to be sure that iron turns off
		{
			h2integral=0;
			h2coolingflag=1;
		}else {							//if the heater is just coming off of a cooling cycle set some integral to be sure that it does not undershoot
			if (h2coolingflag==1)
			{
				h2coolingflag=0;

				if (setpoint<400*23)
				{
					h2integral=cooldown_i_350FSetpower;
				}else if (setpoint<500*23)
				{
					h2integral=cooldown_i_450FSetpower;
				}else if (setpoint<600*23)
				{
					h2integral=cooldown_i_550FSetpower;
				}else if (setpoint<700*23)
				{
					h2integral=cooldown_i_650FSetpower;
				}else if (setpoint<800*23)
				{
					h2integral=cooldown_i_750FSetpower;
				}else
				{
					h2integral=cooldown_i_800FSetpower;
				}
				
			}
		}
		
		h2pid_error = setpoint-currentvalue;
		h2integral = h2integral+h2pid_error;
		h2derivative = h2pid_error - h2lasterror;
		h2lasterror=h2pid_error;
			
	}else
	{
		h2integral = 0;
		h2derivative = 0;
		h2lasterror = 0;
		h2pid_error=0;
	}

	h2pid_output=(h2pid_error*pid_p) + (h2integral*pid_i) + (h2derivative*pid_d);
	

	if (h2pid_output>100){h2pid_output=100;}	//chop output to 100%
	if (h2pid_output<0){h2pid_output=0;}	//chop output to 0%
	
	return h2pid_output;
}



#endif