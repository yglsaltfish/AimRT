#pragma once

#include "aimrt_module_cpp_interface/channel/channel_handle.h"
#include "python_runtime/export_type_support_base.h"

#include "pybind11/pybind11.h"

namespace aimrt::runtime::python_runtime {

inline void ExportChannelContextRef(pybind11::object m) {
  using namespace aimrt::channel;

  pybind11::class_<ContextRef>(m, "ChannelContextRef")
      .def(pybind11::init<>())
      .def("__bool__", &ContextRef::operator bool)
      .def("GetMsgTimestamp", &ContextRef::GetMsgTimestamp)
      .def("SetMsgTimestamp", &ContextRef::SetMsgTimestamp)
      .def("GetMetaValue", &ContextRef::GetMetaValue)
      .def("SetMetaValue", &ContextRef::SetMetaValue);
}

inline bool PyRegisterPublishType(
    aimrt::channel::PublisherRef& publisher_ref,
    const PyTypeSupportBase* msg_type_support) {
  return publisher_ref.RegisterPublishType(msg_type_support->NativeHandle());
}

inline void ExportPublisherRef(pybind11::object m) {
  using namespace aimrt::channel;

  pybind11::class_<PublisherRef>(m, "PublisherRef")
      .def(pybind11::init<>())
      .def("__bool__", &PublisherRef::operator bool)
      .def("RegisterPublishType", &PyRegisterPublishType)
      .def("Publish", &PublisherRef::Publish)
      .def("GetContextManager", &PublisherRef::GetContextManager);
}

inline void PySubscriberRefSubscribeWrapper(
    aimrt::channel::SubscriberRef& subscriber,
    const aimrt_type_support_base_t* msg_type_support,
    std::function<void(const aimrt_channel_context_base_t*, const void*, aimrt_function_base_t*)>&& task) {
  subscriber.Subscribe(msg_type_support, std::move(task));
}

inline void ExportSubscriberRef(pybind11::object m) {
  using namespace aimrt::channel;

  pybind11::class_<SubscriberRef>(m, "SubscriberRef")
      .def(pybind11::init<>())
      .def("__bool__", &SubscriberRef::operator bool);
  // .def("Subscribe", &SubscriberRef::Subscribe);
}

inline void ExportChannelHandleRef(pybind11::object m) {
  using namespace aimrt::channel;

  pybind11::class_<ChannelHandleRef>(m, "ChannelHandleRef")
      .def(pybind11::init<>())
      .def("__bool__", &ChannelHandleRef::operator bool)
      .def("GetPublisher", &ChannelHandleRef::GetPublisher)
      .def("GetSubscriber", &ChannelHandleRef::GetSubscriber)
      .def("GetContextManager", &ChannelHandleRef::GetContextManager);
}
}  // namespace aimrt::runtime::python_runtime