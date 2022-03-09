/*
 * TimedModule.h
 *
 *  Created on: Mar 9, 2022
 *      Author: qo
 */

#ifndef MODULES_TIMEDMODULE_HPP_
#define MODULES_TIMEDMODULE_HPP_

class TimedModule {
public:
	TimedModule();
	virtual ~TimedModule();

	void init();
	void work();
};

#endif /* MODULES_TIMEDMODULE_HPP_ */
