/*
 * dac.cpp
 *
 *  Created on: Mar 11, 2016
 *      Author: philip
 */

#include "pid.hpp"


PID::PID(float p, float i, float d) {
	this->integral = 0;
	this->p = p;
	this->i = i;
	this->d = d;
	this->error = 0;
	this->old_error = 0;
	this->diff_error = 0;
	this->p_control = 0;
	this->i_control = 0;
	this->d_control = 0;
}

__attribute__((section("sram_func")))
float PID::calculate_output(float setpoint,float mesured,float dt) {
	error = setpoint - mesured;

	integral += error*dt;

	p_control = p*error;
	i_control = i*integral;
	// filtered derivative
	diff_error += error*dt*0.05;
	d_control = d*(diff_error-old_error)/dt;

	old_error = diff_error;

	return p_control + i_control + d_control;

}
