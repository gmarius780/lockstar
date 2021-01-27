/*
 * matrix.cpp
 *
 *  Created on: Jan 11, 2021
 *      Author: philipleindecker
 */

#include "matrix.hpp"

float Matrix::getLinarizedVoltageOut() {

	//TODO: Watch out for edge cases & clean up
	if (voltageOutputCounter == noVoltageOuts) {
		voltageOutputCounter = 0;
		voltageOutputTime = 0.0;
		outputActive = false;
	}
	// y(x) = [y0 * (x1 - x) + y1 * (x - x0)] / (x1 - x0)

	deltaT = voltageOuts[voltageOutputCounter+1].time - voltageOuts[voltageOutputCounter].time;

	if (deltaT != 0.0f) {
		linearizedVoltageOut = (voltageOuts[voltageOutputCounter].voltage * (voltageOuts[voltageOutputCounter+1].time - voltageOutputTime) + voltageOuts[voltageOutputCounter+1].voltage * (voltageOutputTime - voltageOuts[voltageOutputCounter].time)) / (deltaT);
	}

	if (voltageOuts[voltageOutputCounter+1].time <= voltageOutputTime) {
		voltageOutputCounter++;

		if (voltageOuts[voltageOutputCounter+1].time == voltageOuts[voltageOutputCounter].time && voltageOutputCounter+1 <= noVoltageOuts) {
			voltageOutputCounter++;
		}
	}

	voltageOutputTime += 1.0f/sampleRate; //+ timeCorrection/sampleRate;//timeStep;

	return linearizedVoltageOut;
}


