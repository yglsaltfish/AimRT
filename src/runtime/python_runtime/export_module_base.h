#pragma once

#include "aimrt_module_cpp_interface/module_base.h"

#include "pybind11/pybind11.h"

namespace aimrt::runtime::python_runtime {

inline void ExportModuleInfo(pybind11::object m) {
  using namespace aimrt;

  pybind11::class_<ModuleInfo>(m, "ModuleInfo")
      .def(pybind11::init<>())
      .def_readwrite("name", &ModuleInfo::name)
      .def_readwrite("major_version", &ModuleInfo::major_version)
      .def_readwrite("minor_version", &ModuleInfo::minor_version)
      .def_readwrite("patch_version", &ModuleInfo::patch_version)
      .def_readwrite("build_version", &ModuleInfo::build_version)
      .def_readwrite("author", &ModuleInfo::author)
      .def_readwrite("description", &ModuleInfo::description);
}

class PyModuleBase : public ModuleBase {
 public:
  using ModuleBase::ModuleBase;

  ModuleInfo Info() const noexcept override {
    PYBIND11_OVERRIDE_PURE(ModuleInfo, ModuleBase, Info);
  }

  bool Initialize(CoreRef core) noexcept override {
    PYBIND11_OVERRIDE_PURE(bool, ModuleBase, Initialize, core);
  }

  bool Start() noexcept override {
    PYBIND11_OVERRIDE_PURE(bool, ModuleBase, Start);
  }

  void Shutdown() noexcept override {
    PYBIND11_OVERRIDE_PURE(void, ModuleBase, Shutdown);
  }
};

inline void ExportModuleBase(pybind11::object m) {
  using namespace aimrt;

  pybind11::class_<ModuleBase, PyModuleBase, std::shared_ptr<ModuleBase>>(m, "ModuleBase")
      .def(pybind11::init<>())
      .def("Info", &ModuleBase::Info)
      .def("Initialize", &ModuleBase::Initialize)
      .def("Start", &ModuleBase::Start)
      .def("Shutdown", &ModuleBase::Shutdown);
}

}  // namespace aimrt::runtime::python_runtime
