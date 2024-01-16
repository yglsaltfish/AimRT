#include "python_runtime/export_channel.h"
#include "python_runtime/export_configurator.h"
#include "python_runtime/export_core.h"
#include "python_runtime/export_core_runtime.h"
#include "python_runtime/export_executor.h"
#include "python_runtime/export_logger.h"
#include "python_runtime/export_module_base.h"
#include "python_runtime/export_type_support_base.h"

#include "pybind11/pybind11.h"

using namespace aimrt::runtime::python_runtime;

PYBIND11_MODULE(aimrt_py, m) {
  m.doc() = "AimRT Python Runtime Module";

  ExportCoreOptions(m);
  ExportCore(m);

  ExportTypeSupportBase(m);

  ExportModuleInfo(m);
  ExportModuleBase(m);

  ExportCoreRef(m);

  ExportConfiguratorRef(m);

  ExportLoggerRef(m);

  ExportExecutorManagerRef(m);
  ExportExecutorRef(m);

  ExportChannelContextRef(m);
  ExportPublisherRef(m);
  ExportSubscriberRef(m);
  ExportChannelHandleRef(m);
}
