/*
 * misc_func.hpp
 *
 *  Created on: 10 Mar 2020
 *      Author: qo
 */

#ifndef INC_MISC_FUNC_HPP_
#define INC_MISC_FUNC_HPP_


#include <queue>



#include "../Lib/pid.hpp"

struct Waypoint { float type; float value; Waypoint(float minmax, float value){this->type=minmax; this->value=value;};};
#define WAYPOINT_GO_TO 3.0f
#define WAYPOINT_GO_UP 4.0f
#define WAYPOINT_GO_DOWN 5.0f
#define WAYPOINT_TRANSM_LARGER 2.0f
#define WAYPOINT_TRANSM_SMALLER -2.0f
#define WAYPOINT_ERRORS_LARGER 1.0f
#define WAYPOINT_ERRORS_SMALLER -1.0f

#ifdef OLD
#include "../HAL/adc.hpp"
#include "../HAL/dac.hpp"
#include "../HAL/raspberrypi.hpp"
float* RecordTrace(ADC_Dev* ADC_DEV, uint8_t ADC_Channel, DAC_Dev* DAC_DEV, uint16_t Steps);
float* RecordTrace(ADC_Dev* ADC_DEV, bool ReadChannel1, bool ReadChannel2, DAC_Dev* DAC_DEV, float From, float To, uint32_t Steps);
void Move2Voltage(DAC_Dev* DAC_DEV, float Voltage, float MaxStep=0.01);
void Go2Lock(DAC_Dev* DAC_DEV, ADC_Dev* ADC_DEV, std::queue<Waypoint> waypoints, float Step);
void RecordDetailedTrace(ADC_Dev* ADC_DEV, bool ReadChannel1, bool ReadChannel2, DAC_Dev* DAC_DEV, RaspberryPi* RPi, uint32_t From, uint32_t NumberSteps, uint32_t BlockSize);
void TestRCCompensationJump(DAC_Dev* DAC_DEV);
void OptimizePID(ADC_Dev* ADC_DEV, uint8_t ADC_Channel, DAC_Dev* DAC_DEV, RaspberryPi* RPi, PID* PIDLoop);
void SelfTest(ADC_Dev* ADC_DEV, DAC_Dev* DAC_1, DAC_Dev* DAC_2, RaspberryPi* RPi);

void DigitalOutHigh();
void DigitalOutLow();

#endif

#endif /* INC_MISC_FUNC_HPP_ */
