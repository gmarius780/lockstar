/*
 * ScopeModule.h
 *
 * Subclasses of this Module inherit 'Scope-functionality': They can 'stream' there inputs and outputs to the backend
 *
 *  Created on: Dec 28, 2022
 *      Author: marius
 */

#ifndef MODULES_SCOPEMODULE_H_
#define MODULES_SCOPEMODULE_H_

#include "../Lib/Scope.h"
#include "Module.h"

class ScopeModule: public Module {
public:
	ScopeModule();
	virtual ~ScopeModule();

	/**RPI methods start**/

	/**Scope methods start**/

	/**
	 * Setup the scope. Check comments in Scope.h
	 * This method should not be called as in frequently as possible, since
	 * it allocates new memory for each execution
	 */
	static const uint32_t METHOD_SETUP_SCOPE = 100;
	void setup_scope(RPIDataPackage* read_package);

	static const uint32_t METHOD_ENABLE_SCOPE = 101;
	void enable_scope(RPIDataPackage* read_package);

	static const uint32_t METHOD_DISABLE_SCOPE = 102;
	void disable_scope(RPIDataPackage* read_package);

	/**
	 * Returns the last batch of recorded data. check comments in Scope.h
	 */
	static const uint32_t METHOD_GET_SCOPE_DATA = 103;
	void get_scope_data(RPIDataPackage* read_package);

	static const uint32_t METHOD_SET_SCOPE_SAMPLING_RATE = 104;
	void set_scope_sampling_rate(RPIDataPackage* read_package);
	/**Scope methods end**/

	/**RPI methods END**/

	void scope_timer_interrupt();

	bool handle_rpi_base_methods() override;

	void initialize_adc_dac(uint8_t ch1_config, uint8_t ch2_config) override;


	Scope *scope;
};

#endif /* MODULES_SCOPEMODULE_H_ */
