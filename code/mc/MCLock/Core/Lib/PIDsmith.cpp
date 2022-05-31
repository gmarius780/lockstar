#include "PIDsmith.hpp"

PIDsmith::PIDsmith(float p, float i, float d, int modelOrder, float deadtime, float dt) {
	Kp = p;
	Ki = i;
	Kd = d;

	integral = 0.0f;

	this->modelOrder 	= modelOrder;
	this->deadtime 		= deadtime;
	this->dt 			= dt;

	A = new float[modelOrder];
	B = new float[modelOrder];

	inputBufferLength 	= modelOrder + deadtime;
	outputBufferLength	= modelOrder;
	inputBufferPointer 	= 0;
	outputBufferPointer	= 0;

	inputBuffer 	= new float[inputBufferLength];
	outputBuffer 	= new float[outputBufferLength];

	for(int i=0; i<modelOrder; i++) {
		A[i] = 0;
		B[i] = 0;
	}

	for(int i=0; i<outputBufferLength; i++)
		outputBuffer[i] = 0;

	for(int i=0; i<inputBufferLength; i++)
		inputBuffer[i] = 0;
}


float PIDsmith::calcControlOutput(float sp,float pv, float dtNew) {

	this->dt = dtNew;
	// Standard PID error signal
	error = sp-pv;

	// Correct the error signal with predicted output
	error += outputBuffer[(outputBufferPointer+outputBufferLength-deadtime)%outputBufferLength];
	error -= outputBuffer[(outputBufferPointer+outputBufferLength-deadtime)%outputBufferLength];

	// proceed as usual with PID
	integral += error*dt;

	float pControl = Kp*error;
	float iControl = Ki*integral;
	float dControl = Kd*(error-oldError)/dt;

	oldError = error;
	float controlOutput = pControl + iControl + dControl;
	//float controlOutput = sp;
	calcModelOutput(controlOutput);

	return controlOutput;
}


float PIDsmith::calcModelOutput(float input) {

	inputBuffer[inputBufferPointer] = input;

	float modelOutput = 0;

	modelOutput += A[0]*inputBuffer[(inputBufferPointer+inputBufferLength-deadtime-0)%inputBufferLength];
	for(int i=1; i<modelOrder; i++) {
		modelOutput += A[i]*inputBuffer[(inputBufferPointer+inputBufferLength-deadtime-i)%inputBufferLength];
		modelOutput -= B[i]*outputBuffer[(outputBufferPointer+outputBufferLength-i)%outputBufferLength];
	}

	outputBuffer[outputBufferPointer] = modelOutput;

	inputBufferPointer = (inputBufferPointer+1)%inputBufferLength;
	outputBufferPointer = (outputBufferPointer+1)%outputBufferLength;

	return modelOutput;
}

int PIDsmith::setModelParameter(const float* A, const float* B, const int order) {
	if(order != modelOrder)
		return -1;

	for(int i=0; i<modelOrder; i++) {
		this->A[i] = A[i];
		this->B[i] = B[i];
	}

	return 1;
}
