#pragma once

#include "aimrt_module_cpp_interface/core.h"

#include "pybind11/functional.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

namespace aimrt::runtime::python_runtime {

inline void ExportCoreRef(pybind11::object m) {
  pybind11::class_<CoreRef>(m, "CoreRef")
      .def(pybind11::init<>())
      .def("__bool__", &CoreRef::operator bool)
      .def("GetConfigurator", &CoreRef::GetConfigurator)
      .def("GetLogger", &CoreRef::GetLogger)
      .def("GetExecutorManager", &CoreRef::GetExecutorManager)
      .def("GetRpcHandle", &CoreRef::GetRpcHandle)
      .def("GetChannelHandle", &CoreRef::GetChannelHandle)
      .def("GetAllocator", &CoreRef::GetAllocator)
      .def("GetParameterHandle", &CoreRef::GetParameterHandle);
}

}  // namespace aimrt::runtime::python_runtime