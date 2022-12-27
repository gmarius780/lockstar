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


public:
	PID(float p, float i, float d, float input_offset, float output_offset, bool intensity_mode);
	PID(float p, float i, float d, float input_offset, float output_offset) : PID(p, i, d, input_offset, output_offset, false) {}


	void set_pid(float p,float i ,float d, float input_offset, float output_offset) {
		this->p = p;
		this->i = i;
		this->d = d;
		this->input_offset = input_offset;
		this->output_offset = output_offset;
		this->integral = 0;
	};

	void set_pid(float p,float i ,float d, float input_offset, float output_offset, float i_threshold) {
		this->set_pid(p, i, d, input_offset, output_offset);
		this->i_threshold = i_threshold;
	}

	void enable_intensity_mode();
	void disable_intensity_mode();

	/*
	 * calls the function pointed to in calculate_output_func_pointer
	 * This function-pointer construction should allow for a faster pid loop in the case where intensity_mode
	 * is disabled since it requires less if clauses
	 */
	float calculate_output(float setpoint,float mesured,float dt);
private:
		float integral;
		float error;
		float old_error;
		float diff_error;
		float p, i, d;
		float i_threshold; //threshold below which integrator is set to zero (if intensity_mode == true)
		float p_control, i_control, d_control;
		float input_offset, output_offset;

		bool intensity_mode;

		/*
		 * Calculates pid outputs the standard way (in contrast to 'for_intensity')
		 */
		float calculate_output_normal(float setpoint,float mesured,float dt);
		/*
		 * Calculates PID output normally but for the following change:
		 * if the setpoint is below i_threshold, the integral is ignored
		 * and the integrator is set to zero. This is useful in case of an
		 * intensity stabilization with a non zero photodiode offset
		 */
		float calculate_output_for_intensity(float setpoint,float mesured,float dt);
		float(PID::*calculate_output_func_pointer)(float, float, float);
};

#endif /* PID_H_ */
