/*
 * dac.h
 *
 *  Created on: Mar 11, 2016
 *      Author: philip
 */


#ifndef PID_H_
#define PID_H_

#define PID_TRY_SETTINGS 0
#define PID_ACCEPT_SETTINGS 1
#define PID_ABORT 2

class PID
{
private:
	float pre_error, integral;
	unsigned long counter;
	float dt;
public:
	float Kp, Ki, Kd, max, min, pre_output;
	volatile float set_point;
	bool pol;
	PID() { pre_error=0; integral=0; pre_output = 0; min=-10; max=10; dt=1; Kp=0; Kd=0; Ki=0; counter=0; set_point=0.0; };
	float CalculateFeedback(float setpoint, float pv);
	float CalculateFeedback(float pv);
	float CalculateLimitedFeedback(float pv, float limit);
	float Diff(float setpoint, float pv);
	void Reset() { pre_error=0; integral=0; pre_output = 0; counter=0; };
	void SetVars(float* Vars);
	void SetPIDParam(float* Vars);
	void SetVal(float Val) {pre_output = Val;};
	void Setdt(float dt) {this->dt = dt;};

	friend class UserData;
};

#endif /* PID_H_ */
