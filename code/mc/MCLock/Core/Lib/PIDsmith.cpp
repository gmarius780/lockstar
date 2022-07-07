#include "PIDsmith.hpp"
#include <stm32f427xx.h>

PIDsmith::PIDsmith(float p, float i, float d, int modelOrder, float deadtime, float dt) {
	Kp = p;
	Ki = i;
	Kd = d;

	integral = 0.0f;
	controlOutput = 0.0f;
	error = 0.0f;
	dError = 0.0f;
	oldError = 0.0f;
	modelOffset = 0.0f;

	this->modelOrder 	= modelOrder;
	this->deadtime 		= deadtime;
	this->dt 			= dt;

	A = new float[modelOrder]();
	B = new float[modelOrder]();

	inputBufferLength 	= modelOrder + deadtime;
	outputBufferLength	= modelOrder + 1; // +1 for debugging
	inputBufferPointer 	= 0;
	outputBufferPointer	= 0;

	inputBuffer 	= new float[inputBufferLength]();
	outputBuffer 	= new float[outputBufferLength]();

	debug = false;
}

__attribute__((section("sram_func")))
float PIDsmith::calcControlOutput(float sp, float pv, float dtNew) {

	this->dt = dtNew;
	// Standard PID error signal
	error = sp-pv;

	// Correct the error signal with predicted output
	error += getModelOutput(deadtime);
	error -= getModelOutput(0);

	// proceed as usual with PID
	float pControl = Kp*error;

	integral += error*dt;
	float iControl = Ki*integral;
	if(iControl < 0 || iControl > 10) {
		integral -= error*dt;
		iControl = Ki*integral;
	}

	// filtered derivative term
	dError += error*dt*0.05;
	float dControl = Kd*(dError-oldError)/dt;


	oldError = dError;
	controlOutput = pControl + iControl + dControl;
	//controlOutput = sp;

	calcModelOutput(controlOutput);

	return controlOutput;
}

__attribute__((section("sram_func")))
void PIDsmith::calcModelOutput(float input) {

	inputBufferPointer = (inputBufferPointer+1)%inputBufferLength;
	outputBufferPointer = (outputBufferPointer+1)%outputBufferLength;

	inputBuffer[inputBufferPointer] = input;

	float modelOutput = 0;

	modelOutput += A[0]*inputBuffer[(inputBufferPointer+inputBufferLength-deadtime)%inputBufferLength];
	for(int i=1; i<modelOrder; i++) {
		modelOutput += A[i]*inputBuffer[(inputBufferPointer+inputBufferLength-deadtime-i)%inputBufferLength];
		modelOutput -= B[i]*outputBuffer[(outputBufferPointer+outputBufferLength-i)%outputBufferLength];
	}

	outputBuffer[outputBufferPointer] = modelOutput;
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

__attribute__((section("sram_func")))
float PIDsmith::getModelOutput(int pointerOffset) {
	float modelOutput = outputBuffer[(outputBufferPointer+outputBufferLength-pointerOffset)%outputBufferLength];
	modelOutput += modelOffset;

	if(modelOutput > 4.28)
		modelOutput = 4.28;
	else if(modelOutput < 0)
		modelOutput = 0;

	return modelOutput;
}
