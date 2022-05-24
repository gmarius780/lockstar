#include "PIDnew.hpp"

PIDnew::PIDnew(float p, float i, float d) {
			this->integral = 0;
			this->Kp = p;
			this->Ki = i;
			this->Kd = d;
			this->dt = 1;
			this->error = 0;
			this->oldError = 0;
		}

float PIDnew::calcControlOutput(float sp, float pv, float dtNew) {
	dt = dtNew;
	error = sp - pv;

	integral += error*dt;

	float pControl = Kp*error;
	float iControl = Ki*integral;
	float dControl = Kd*(error-oldError)/dt;

	oldError = error;

	return pControl + iControl + dControl;

}
