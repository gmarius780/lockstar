/*
 * dac.cpp
 *
 *  Created on: Mar 11, 2016
 *      Author: philip
 */

#include "pid.hpp"

__attribute__((section("sram_func")))
float PID::CalculateFeedback(float setpoint, float pv)
{
	// Calculate error
	    float error = pol ? (setpoint - pv) : (pv - setpoint);

	    // Proportional term
	    float Pout = Kp * error;

	    float Iout;
	    // Integral term
	    //if(counter>10000) {
	    	integral += error * dt;
	    	Iout = Ki * integral;
	    //}
	    //else
	    //	Iout = 0;
	    //counter++;

	    // Derivative term
	    float derivative = (error - pre_error) / dt;
	    float Dout = Kd * derivative;

	    // Calculate total output
	    float output = Pout + Iout + Dout;

	    output += pre_output;

	    // Restrict to max/min
	    if( output > max )
	        output = max;
	    else if( output < min )
	        output = min;

	    // Save error to previous error
	    pre_error = error;

	    pre_output = output;

	    return output;
}

void PID::SetVars(float* Vars){
	Kp = *Vars;
	Ki = *(Vars+1);
	Kd = *(Vars+2);
	max = *(Vars+3);
	min = *(Vars+4);
}

void PID::SetPIDParam(float* Vars){
	Kp = *Vars;
	Ki = *(Vars+1);
	Kd = *(Vars+2);
	integral=0;
}

float PID::Diff(float setpoint, float pv)
{
	// Calculate error
	    float error = pol ? (setpoint - pv) : (pv - setpoint);

	    // Proportional term
	    float Pout = Kp * error;

	    float Iout;
	    integral += error * dt;
	    Iout = Ki * integral;

	    // Derivative term
	    float derivative = (error - pre_error) / dt;
	    float Dout = Kd * derivative;

	    // Calculate total output
	    float output = Pout + Iout + Dout;

	    return output;
}
