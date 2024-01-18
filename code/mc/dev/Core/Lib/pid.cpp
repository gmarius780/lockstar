/*
 * dac.cpp
 *
 *  Created on: Mar 11, 2016
 *      Author: philip
 */

#include "pid.hpp"

PID::PID(float p, float i, float d, float input_offset, float output_offset,
         float i_threshold, bool intensity_mode) {
  this->integral = 0;
  this->p = p;
  this->i = i;
  this->d = d;
  this->error = 0;
  this->old_error = 0;
  this->diff_error = 0;
  this->p_control = 0;
  this->i_control = 0;
  this->d_control = 0;
  this->input_offset = input_offset;
  this->output_offset = output_offset;
  this->intensity_mode = intensity_mode;
  this->i_threshold = i_threshold;

  if (intensity_mode == true) {
    this->calculate_output_func_pointer = &PID::calculate_output_for_intensity;
  } else {
    this->calculate_output_func_pointer = &PID::calculate_output_normal;
  }
}

void PID::enable_intensity_mode() {
  intensity_mode = true;
  this->calculate_output_func_pointer = &PID::calculate_output_for_intensity;
}

void PID::disable_intensity_mode() {
  intensity_mode = false;
  this->calculate_output_func_pointer = &PID::calculate_output_normal;
}

__attribute__((section(".itcmram"))) float
PID::calculate_output(float setpoint, float measured, float dt) {
  return (this->*calculate_output_func_pointer)(setpoint, measured, dt);
}

/*
 * Calculates pid outputs the standard way (in contrast to 'for_intensity')
 */
__attribute__((section(".itcmram"))) float
PID::calculate_output_normal(float setpoint, float measured, float dt) {
  error = setpoint - measured + input_offset;

  integral += error * dt;

  p_control = p * error;
  i_control = i * integral;
  // filtered derivative
  diff_error += error * dt;
  d_control = d * (diff_error - old_error) / dt;

  old_error = diff_error;

  return output_offset + p_control + i_control + d_control;
}

/*
 * Calculates PID output normally but for the following change:
 * if the setpoint is below i_threshold, the integral is ignored
 * and the integrator is set to zero. This is useful in case of an
 * intensity stabilization with a non zero photodiode offset
 */
__attribute__((section(".itcmram"))) float
PID::calculate_output_for_intensity(float setpoint, float measured, float dt) {
  error = setpoint - measured + input_offset;

  if (setpoint < i_threshold) {
    integral = 0;
  } else {
    integral += error * dt;
  }

  p_control = p * error;
  i_control = i * integral;
  // filtered derivative
  diff_error += error * dt;
  d_control = d * (diff_error - old_error) / dt;

  old_error = diff_error;

  return output_offset + p_control + i_control + d_control;
}
