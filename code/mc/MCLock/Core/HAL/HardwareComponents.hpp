#include <stdint.h>
#include <iosfwd>
#include <cstddef>
#include <cstring>

#ifndef HAL_HARDWARECOMPONENTS_HPP_
#define HAL_HARDWARECOMPONENTS_HPP_

enum struct HardwareComponents: uint8_t{
	analog_in_one = (uint8_t)0,
    analog_in_two = (uint8_t)1,
    analog_out_one = (uint8_t)2,
    analog_out_two = (uint8_t)3};

#endif
