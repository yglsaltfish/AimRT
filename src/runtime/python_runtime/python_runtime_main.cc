#include "aimrt_module_cpp_interface/module_base.h"
#include "core/aimrt_core.h"

#include "pybind11/functional.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

namespace py = pybind11;

namespace aimrt::runtime::python_runtime {

void ExportModuleInfo(py::object m) {
  py::class_<aimrt::ModuleInfo>(m, "ModuleInfo")
      .def(py::init<>())
      .def_readwrite("name", &aimrt::ModuleInfo::name)
      .def_readwrite("major_version", &aimrt::ModuleInfo::major_version)
      .def_readwrite("minor_version", &aimrt::ModuleInfo::minor_version)
      .def_readwrite("patch_version", &aimrt::ModuleInfo::patch_version)
      .def_readwrite("build_version", &aimrt::ModuleInfo::build_version)
      .def_readwrite("author", &aimrt::ModuleInfo::author)
      .def_readwrite("description", &aimrt::ModuleInfo::description);
}

void ExportCoreOptions(py::object m) {
  using namespace aimrt::runtime::core;
  py::class_<AimRTCore::Options>(m, "CoreOptions")
      .def(py::init<>())
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

  void Start() { core_.Start(); }

  void Shutdown() { core_.Shutdown(); }

  void RegisterModule(const aimrt::ModuleBase* module) {
    AIMRT_HL_CHECK_ERROR_THROW(core_.GetLogger(), module, "module is null!");

    core_.GetModuleManager().RegisterModule(module->NativeHandle());
  }

 private:
  aimrt::runtime::core::AimRTCore core_;
};

void ExportCore(py::object m) {
  py::class_<PyCore>(m, "Core")
      .def(py::init<>())
      .def("Initialize", &PyCore::Initialize)
      .def("Start", &PyCore::Start)
      .def("Shutdown", &PyCore::Shutdown)
      .def("RegisterModule", &PyCore::RegisterModule);
}

}  // namespace aimrt::runtime::python_runtime

PYBIND11_MODULE(aimrt_py, m) {
  m.doc() = "AimRT Python Runtime Module";

  using namespace aimrt::runtime::python_runtime;

  ExportModuleInfo(m);
  ExportCoreOptions(m);
  ExportCore(m);
}
