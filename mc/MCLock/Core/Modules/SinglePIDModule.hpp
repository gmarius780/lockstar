/*
 * SinglePIDModule.h
 *
 *  Created on: Mar 9, 2022
 *      Author: qo
 */

#ifndef MODULES_SINGLEPIDMODULE_HPP_
#define MODULES_SINGLEPIDMODULE_HPP_

#include "Module.hpp"
#include "../Lib/pid.hpp"

class SinglePIDModule : Module{
public:
	SinglePIDModule();
	virtual ~SinglePIDModule();

	void init();
	void work();
	PID* PIDLoop;
};

#endif /* MODULES_SINGLEPIDMODULE_HPP_ */
