#pragma once

#include "aimrt_module_cpp_interface/module_base.h"
#include "core/aimrt_core.h"

#include "pybind11/pybind11.h"

namespace aimrt::runtime::python_runtime {

inline void ExportCoreOptions(pybind11::object m) {
  using namespace aimrt::runtime::core;

  pybind11::class_<AimRTCore::Options>(m, "CoreOptions")
      .def(pybind11::init<>())
      .def_readwrite("cfg_file_path", &AimRTCore::Options::cfg_file_path)
      .def_readwrite("dump_cfg_file", &AimRTCore::Options::dump_cfg_file)
      .def_readwrite("dump_cfg_file_path", &AimRTCore::Options::dump_cfg_file_path)
      .def_readwrite("register_signal", &AimRTCore::Options::register_signal)
      .def_readwrite("auto_set_to_global", &AimRTCore::Options::auto_set_to_global);
}

class PyCore {
 public:
  void Initialize(const aimrt::runtime::core::AimRTCore::Options& options) {
    core_.Initialize(options);
  }

  void Start() {
    // 阻塞之前需要释放gil锁
    pybind11::gil_scoped_release release;

    core_.Start();

    pybind11::gil_scoped_acquire acquire;
  }

  void Shutdown() { core_.Shutdown(); }

  void RegisterModule(const aimrt::ModuleBase* module) {
    AIMRT_HL_CHECK_ERROR_THROW(core_.GetLogger(), module, "module is null!");
    core_.GetModuleManager().RegisterModule(module->NativeHandle());
  }

 private:
  aimrt::runtime::core::AimRTCore core_;
};

inline void ExportCore(pybind11::object m) {
  pybind11::class_<PyCore>(m, "Core")
      .def(pybind11::init<>())
      .def("Initialize", &PyCore::Initialize)
      .def("Start", &PyCore::Start)
      .def("Shutdown", &PyCore::Shutdown)
      .def("RegisterModule", &PyCore::RegisterModule);
}

}  // namespace aimrt::runtime::python_runtime
