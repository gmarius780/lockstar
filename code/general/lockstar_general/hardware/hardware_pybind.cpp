#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "HAL/HardwareComponents.hpp"

namespace py = pybind11;

PYBIND11_MODULE(hardware, m) {

    py::enum_<HardwareComponents>(m, "HardwareComponents")
        .value("analog_in_one", HardwareComponents::analog_in_one)
        .value("analog_in_two", HardwareComponents::analog_in_two)
        .value("analog_out_one", HardwareComponents::analog_out_one)
        .value("analog_out_two", HardwareComponents::analog_out_two)
        .export_values();
}