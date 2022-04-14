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
#include "../HAL/HardwareComponents.hpp"

#include <tuple>
enum PidState {pid_locked, pid_unlocked};

class SinglePIDModule : public Module{
public:
	SinglePIDModule();
	virtual ~SinglePIDModule();

	void init() override;
	void work() override;
	virtual uint8_t handle_rpi_input() override;
	PID* pid_loop;
	Oscilloscope* oscilloscope;

	PidState pid_state;

	void initialize(float p, float i, float d, float out_range_min, float out_range_max, bool useTTL, bool locked,
			HardwareComponents err_channel, HardwareComponents setpoint_channel, HardwareComponents out_channel);
	void set_pid(float p, float i, float d);
	void lock();
	void unlock();
	void enable_live_channel(HardwareComponents ch);
	void disable_live_channel(HardwareComponents ch);
	void get_channel_data(HardwareComponents ch);
};

#endif /* MODULES_SINGLEPIDMODULE_HPP_ */
