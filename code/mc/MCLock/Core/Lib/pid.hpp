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
		float p, i, d;
		float p_control, i_control, d_control;
		float input_offset, output_offset;

public:
	PID(float p, float i, float d, float input_offset, float output_offset);

	void set_pid(float p,float i ,float d, float input_offset, float output_offset) {
		this->p = p;
		this->i = i;
		this->d = d;
		this->input_offset = input_offset;
		this->output_offset = output_offset;
		this->integral = 0;
	};

	float calculate_output(float setpoint,float mesured,float dt);

};

#endif /* PID_H_ */
