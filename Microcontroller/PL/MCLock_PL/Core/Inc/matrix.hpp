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
    float time;
    float voltage;
};

class Matrix {
private:
	bool outputActive = false;
	vector<voltageOut> voltageOuts;
	float voltageOutputTime;
	uint32_t voltageOutputCounter;
	uint32_t noVoltageOuts;
	float linearizedVoltageOut;
	float sampleRate = 10000.0f;
	float correction;
	float timeStep;
	float deltaT;
	float timeCorrection = 6.2f/10.4f;
public:
	Matrix() {
		//this->timeStep = 1.0f/sampleRate;
	};

	float getLinarizedVoltageOut();

	void reset() {
		voltageOutputTime = 0;
		voltageOutputCounter = 0;
		linearizedVoltageOut = 0;
		noVoltageOuts = 0;
		voltageOuts.clear();
	}

	void addVoltageOut(voltageOut vo) {
		voltageOuts.push_back(vo);
	}

	/*Getter and Setter*/

	bool isOutputActive() const {
		return outputActive and voltageOuts.size() > 1;
	}

	void setOutputActive(bool outputActive = false) {
		this->outputActive = outputActive;
	}

	void setNoVoltages(uint32_t noVoltages) {
		noVoltageOuts = noVoltages;
	}

	uint32_t getNoVoltages() const {
		return noVoltageOuts;
	}

	void setSampleRate(float sampleRate = 10000.0f) {
		this->sampleRate = sampleRate;
		//this->timeStep = 1.0f/sampleRate;
	}
};


#endif /* INC_MATRIX_HPP_ */
