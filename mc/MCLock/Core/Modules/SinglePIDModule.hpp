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
#include "../Lib/oscilloscope.hpp"

enum PidState {pid_locked, pid_unlocked};

class SinglePIDModule : public Module{
public:
	SinglePIDModule();
	virtual ~SinglePIDModule();

	void init() override;
	void work() override;
	void handle_rpi_input() override;
	PID* pid_loop;
	Oscilloscope* oscilloscope;

	PidState pid_state;
};

#endif /* MODULES_SINGLEPIDMODULE_HPP_ */
