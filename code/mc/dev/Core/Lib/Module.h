/*
 * Module.hpp
 *
 *  Created on: Feb 25, 2022
 *      Author: qo
 */

#ifndef MODULES_MODULE_H_
#define MODULES_MODULE_H_

#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"
#include "stm32h7xx_it.h"

#include "../HAL/leds.hpp"
#include "../HAL/rpi.h"
#include "../HAL/spi.hpp"
#include "../Lib/LinearizableDAC.h"
#include <main.h>

#include "../HAL/ADCDevice.hpp"
#include <cstring>

class Module {
public:
  Module();
  virtual ~Module();

  /**RPI methods start**/

  static const uint32_t METHOD_SET_CH_ONE_OUTPUT_LIMITS = 80;
  void set_ch_one_output_limits(RPIDataPackage *read_package);

  static const uint32_t METHOD_SET_CH_TWO_OUTPUT_LIMITS = 81;
  void set_ch_two_output_limits(RPIDataPackage *read_package);

  static const uint32_t METHOD_UNLCAMP_OUTPUT = 90;
  void unclamp_output(RPIDataPackage *read_package);

  /*LINEARIZATION-METHODS START*/
  static const uint32_t METHOD_SET_LINEARIZATION_ONE = 82;
  void set_linearization_one(
      RPIDataPackage *read_package); // set calculated linearization parameters

  static const uint32_t METHOD_SET_LINEARIZATION_TWO = 83;
  void set_linearization_two(
      RPIDataPackage *read_package); // set calculated linearization parameters

  static const uint32_t METHOD_SET_LINEARIZATION_LENGTH_ONE = 84;
  void set_linearization_length_one(
      RPIDataPackage *read_package); // set number of points for linearization
                                     // (must be set befor set linearization)

  static const uint32_t METHOD_SET_LINEARIZATION_LENGTH_TWO = 85;
  void set_linearization_length_two(
      RPIDataPackage *read_package); // set number of points for linearization
                                     // (must be set befor set linearization)

  static const uint32_t METHOD_ENABLE_LINEARIZATION_ONE = 86;
  void enable_linearization_one(RPIDataPackage *read_package);
  static const uint32_t METHOD_ENABLE_LINEARIZATION_TWO = 87;
  void enable_linearization_two(RPIDataPackage *read_package);
  static const uint32_t METHOD_DISABLE_LINEARIZATION_ONE = 88;
  void disable_linearization_one(RPIDataPackage *read_package);
  static const uint32_t METHOD_DISABLE_LINEARIZATION_TWO = 89;
  void disable_linearization_two(RPIDataPackage *read_package);
  /*LINEARIZATION-METHODS END*/

  /**RPI methods END**/

  virtual void initialize_adc_dac(uint8_t ch1_config, uint8_t ch2_config);
  void initialize_rpi();

  void set_ch_output_limit(RPIDataPackage *read_package, LinearizableDAC *dac);

  virtual bool handle_rpi_base_methods(); // handles calls send by the rpi
                                          // corresponding to general methods

public:
  ADC_Device *adc;
  LinearizableDAC *dac_1, *dac_2;
  RPI *rpi;

private:
  void set_linearization(RPIDataPackage *read_package, LinearizableDAC *dac);
};

#endif /* MODULES_MODULE_H_ */
