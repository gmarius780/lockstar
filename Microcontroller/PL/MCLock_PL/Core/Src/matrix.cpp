/*
 * matrix.cpp
 *
 *  Created on: Jan 11, 2021
 *      Author: philipleindecker
 */

#include "matrix.hpp"

void Matrix::doTimeStepEvolution() {

	timeCounter++;

	channel_1.increaseCounter(timeCounter);
	channel_2.increaseCounter(timeCounter);

	if (timeCounter == timeCounterEnd) {
		channel_1.counter = 0;
		channel_2.counter = 0;
		channel_1.linearizedVoltageOut = 0.0f;
		channel_2.linearizedVoltageOut = 0.0f;
		timeCounter = 0;
		outputActive = false;
	}
}


