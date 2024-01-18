#pragma once

#include "aimrt_module_cpp_interface/channel/channel_handle.h"
#include "python_runtime/export_type_support.h"

#include "pybind11/pybind11.h"

namespace aimrt::runtime::python_runtime {

inline bool PyRegisterPublishType(
    aimrt::channel::PublisherRef& publisher_ref,
    std::shared_ptr<const PyTypeSupport> msg_type_support) {
  static std::vector<std::shared_ptr<const PyTypeSupport>> py_ts_vec;
  py_ts_vec.emplace_back(msg_type_support);

  return publisher_ref.RegisterPublishType(msg_type_support->NativeHandle());
}

inline void PyPublish(
    aimrt::channel::PublisherRef& publisher_ref,
    std::string_view msg_type,
    std::string_view serialization_type,
    const std::string& msg_buf) {
  auto ctx_ptr = publisher_ref.GetContextManager().NewContextSharedPtr();
  auto ctx_ref = aimrt::channel::ContextRef(ctx_ptr);
  ctx_ref.SetSerializationType(serialization_type);
  publisher_ref.Publish(msg_type, ctx_ref, static_cast<const void*>(&msg_buf));
}

inline void ExportPublisherRef(pybind11::object m) {
  using namespace aimrt::channel;

  pybind11::class_<PublisherRef>(m, "PublisherRef")
      .def(pybind11::init<>())
      .def("__bool__", &PublisherRef::operator bool)
      .def("RegisterPublishType", &PyRegisterPublishType)
      .def("Publish", &PyPublish);
}

inline bool PySubscribe(
    aimrt::channel::SubscriberRef& subscriber_ref,
    std::shared_ptr<const PyTypeSupport> msg_type_support,
    std::function<void(std::string_view, const std::string&)>&& callback) {
  static std::vector<std::shared_ptr<const PyTypeSupport>> py_ts_vec;
  py_ts_vec.emplace_back(msg_type_support);

  return subscriber_ref.Subscribe(
      msg_type_support->NativeHandle(),
      [callback{std::move(callback)}](
          const aimrt_channel_context_base_t* ctx_ptr,
          const void* msg_ptr,
          aimrt_function_base_t* release_callback_base) {
        aimrt::util::Function<aimrt_function_subscriber_release_callback_ops_t> release_callback(release_callback_base);
        const std::string& msg_buf = *static_cast<const std::string*>(msg_ptr);
        auto ctx_ref = aimrt::channel::ContextRef(ctx_ptr);
        callback(ctx_ref.GetSerializationType(), msg_buf);
        release_callback();
      });
}

inline void ExportSubscriberRef(pybind11::object m) {
  using namespace aimrt::channel;

  pybind11::class_<SubscriberRef>(m, "SubscriberRef")
      .def(pybind11::init<>())
      .def("__bool__", &SubscriberRef::operator bool)
      .def("Subscribe", &PySubscribe);
}

inline void ExportChannelHandleRef(pybind11::object m) {
  using namespace aimrt::channel;

  pybind11::class_<ChannelHandleRef>(m, "ChannelHandleRef")
      .def(pybind11::init<>())
      .def("__bool__", &ChannelHandleRef::operator bool)
      .def("GetPublisher", &ChannelHandleRef::GetPublisher)
      .def("GetSubscriber", &ChannelHandleRef::GetSubscriber);
}
}  // namespace aimrt::runtime::python_runtime