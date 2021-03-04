/*
 * matrix.hpp
 *
 *  Created on: Jan 11, 2021
 *      Author: philipleindecker
 */

#ifndef INC_MATRIX_HPP_
#define INC_MATRIX_HPP_

#include <stdio.h>
#include <vector>
#include <complex>
using namespace std;

struct voltageOut {
	uint32_t time;
    float voltage;
};

struct pidInteruptIntervall {
	uint32_t start;
	uint32_t end;

	bool timeIsInIntervall(uint32_t time) {
		return (time >= start && time < end);
	}
};

struct matrixChannel {
	vector<voltageOut> voltageOuts;
	uint32_t voltagesOutsNr = 0;
	uint32_t counter = 0;
	float linearizedVoltageOut = 0.0f;
	uint32_t deltaT = 0;
	vector<pidInteruptIntervall> pidInterruptIntervalls;
	uint32_t pidInterruptIntervallsNr = 0;

	matrixChannel() {

	}

	void increaseCounter(uint32_t time) {
		if (counter+1 <= voltagesOutsNr) {
			if (voltageOuts[counter+1].time <= time) {
				counter++;
				if (voltageOuts[counter+1].time == voltageOuts[counter].time && counter+1 <= voltagesOutsNr) {
					counter++;
				}
			}
		}
	}

	float getLinearizedVoltageOut(uint32_t time) {
		// y(x) = [y0 * (x1 - x) + y1 * (x - x0)] / (x1 - x0)
		deltaT = voltageOuts[counter+1].time - voltageOuts[counter].time;
		if (deltaT != 0) {
			linearizedVoltageOut = (voltageOuts[counter].voltage * (voltageOuts[counter+1].time - time) + voltageOuts[counter+1].voltage * (time - voltageOuts[counter].time)) / (deltaT);
		}
		return linearizedVoltageOut;
	}

	float getVoltageOut() {
		return voltageOuts[counter].voltage;
	}

	bool pidInterrupted(uint32_t time) {
		//TODO: Improve this check
		for(int i = 0; i<pidInterruptIntervallsNr; i++) {
			if (pidInterruptIntervalls[i].timeIsInIntervall(time)) {
				return true;
			}
		}
		return false;
	}

	void reset() {
		voltageOuts.clear();
		voltagesOutsNr = 0;
		counter = 0;
		linearizedVoltageOut = 0.0f;
		deltaT = 0;
		pidInterruptIntervalls.clear();
		pidInterruptIntervallsNr = 0;
	}
};

class Matrix {
private:
	bool outputActive = false;
public:
	bool hasMatrixData = false;
	uint32_t timeCounter, timeCounterEnd;
	uint32_t timeCorr = 12500;
	uint32_t timeCorrCounter;
	matrixChannel channel_1, channel_2;

	Matrix() {
		timeCounter = 0;
		timeCounterEnd = 0;
		timeCorrCounter = 0;
	};

	void doTimeStepEvolution();

	void reset() {
		timeCounter = 0;
		channel_1.reset();
		channel_2.reset();
	}

	/*Getter and Setter*/

	void setTimeCounterEnd() {
		if (channel_1.voltagesOutsNr > 0) {
			timeCounterEnd = channel_1.voltageOuts.back().time;
		} else if (channel_2.voltagesOutsNr > 0) {
			timeCounterEnd = channel_2.voltageOuts.back().time;
		}
	}

	bool isOutputActive() const {
		return outputActive;
	}

	bool isChannel_1Active() const {
		return channel_1.voltagesOutsNr > 0;
	}

	bool isChannel_2Active() const {
		return channel_2.voltagesOutsNr > 0;
	}

	void setOutputActive(bool outputActive = false) {
		this->outputActive = outputActive; //&& ((channel_1.voltagesOutsNr > 0) || (channel_2.voltagesOutsNr > 0));
	}
};


#endif /* INC_MATRIX_HPP_ */
