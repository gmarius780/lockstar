/*
 * dac.h
 *
 *  Created on: Mar 11, 2016
 *      Author: philip
 */


#ifndef PID_H_
#define PID_H_


class PID
{
private:
		float integral;
		float error;
		float old_error;
		float diff_error;
		float p;
		float i;
		float d;
		float p_control;
		float i_control;
		float d_control;

public:
	PID(float p, float i, float d);

	void set_pid(float p,float i ,float d) { this->p = p; this->i = i; this->d = d; };
	float calculate_output(float setpoint,float mesured,float dt);

};

#endif /* PID_H_ */
