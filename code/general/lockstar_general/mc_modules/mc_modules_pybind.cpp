#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "Modules/SinglePIDModuleDP.hpp"

namespace py = pybind11;

PYBIND11_MODULE(mc_modules, m) {

    py::enum_<SinglePIDMethods>(m, "SinglePIDMethods")
        .value("INITIALIZE", SinglePIDMethods::INITIALIZE)
        .value("SET_PID", SinglePIDMethods::SET_PID)
        .export_values();
	
	py::class_<SinglePIDModuleDP>(m, "SinglePIDModuleDP")
    .def("write_initialize_call", &SinglePIDModuleDP::write_initialize_call)
    .def("size_initialize_call", &SinglePIDModuleDP::size_initialize_call);
}