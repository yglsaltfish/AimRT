#pragma once

#include "aimrt_module_c_interface/util/type_support_base.h"
#include "aimrt_module_cpp_interface/util/string.h"

#include "pybind11/functional.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

namespace aimrt::runtime::python_runtime {

class PyTypeSupportBase {
 public:
  PyTypeSupportBase() : base_(GenBase(this)) {}
  virtual ~PyTypeSupportBase() {}

  std::string type_name;

  virtual void* Create() const = 0;

  virtual void Destroy(void* msg) const = 0;

  virtual void Copy(const void* from, void* to) const = 0;

  virtual void Move(void* from, void* to) const = 0;

  virtual bool Serialize(
      std::string_view serialization_type,
      const void* msg,
      std::vector<uint8_t>& buffer) const = 0;

  virtual bool Deserialize(
      std::string_view serialization_type,
      std::string_view buffer,
      void* msg) const = 0;

  std::vector<std::string_view> serialization_types_supported_vec;

  const aimrt_type_support_base_t* NativeHandle() const { return &base_; }

 private:
  static aimrt_type_support_base_t GenBase(void* impl) {
    return aimrt_type_support_base_t{
        .type_name = [](void* impl) -> aimrt_string_view_t {
          return aimrt::util::ToAimRTStringView(static_cast<PyTypeSupportBase*>(impl)->type_name);
        },
        .create = [](void* impl) -> void* {
          return static_cast<PyTypeSupportBase*>(impl)->Create();
        },
        .destroy = [](void* impl, void* msg) {
          static_cast<PyTypeSupportBase*>(impl)->Destroy(msg);  //
        },
        .copy = [](void* impl, const void* from, void* to) {
          static_cast<PyTypeSupportBase*>(impl)->Copy(from, to);  //
        },
        .move = [](void* impl, void* from, void* to) {
          static_cast<PyTypeSupportBase*>(impl)->Move(from, to);  //
        },
        .serialize = [](void* impl, aimrt_string_view_t serialization_type, const void* msg, aimrt_buffer_array_t* buffer_array) -> bool {
          std::vector<uint8_t> buffer_vec;
          bool ret = static_cast<PyTypeSupportBase*>(impl)->Serialize(
              aimrt::util::ToStdStringView(serialization_type),
              msg,
              buffer_vec);

          if (!ret) return false;

          const aimrt_buffer_array_allocator_t* allocator = buffer_array->allocator;
          auto buffer = allocator->allocate(allocator->impl, buffer_array, buffer_vec.size());
          if (buffer.data == nullptr || buffer.len < buffer_vec.size()) return false;
          memcpy(buffer.data, buffer_vec.data(), buffer_vec.size());

          return true;
        },
        .deserialize = [](void* impl, aimrt_string_view_t serialization_type, aimrt_buffer_array_view_t buffer_array_view, void* msg) -> bool {
          if (buffer_array_view.len == 1) {
            std::string_view str_buf(
                static_cast<const char*>(buffer_array_view.data[0].data),
                buffer_array_view.data[0].len);

            return static_cast<PyTypeSupportBase*>(impl)->Deserialize(
                aimrt::util::ToStdStringView(serialization_type),
                str_buf,
                msg);
          }

          if (buffer_array_view.len > 1) {
            size_t total_size = 0;
            for (size_t ii = 0; ii < buffer_array_view.len; ++ii) {
              total_size += buffer_array_view.data[ii].len;
            }
            std::vector<char> buffer_vec(total_size);
            char* buffer = buffer_vec.data();
            size_t cur_size = 0;
            for (size_t ii = 0; ii < buffer_array_view.len; ++ii) {
              memcpy(buffer + cur_size, buffer_array_view.data[ii].data,
                     buffer_array_view.data[ii].len);
              cur_size += buffer_array_view.data[ii].len;
            }

            std::string_view str_buf(static_cast<const char*>(buffer), total_size);

            return static_cast<PyTypeSupportBase*>(impl)->Deserialize(
                aimrt::util::ToStdStringView(serialization_type),
                str_buf,
                msg);
          }

          return false;
        },
        .serialization_types_supported_num = [](void* impl) -> size_t {
          return static_cast<PyTypeSupportBase*>(impl)->serialization_types_supported_vec.size();
        },
        .serialization_types_supported_list = [](void* impl) -> const aimrt_string_view_t* {
          static std::vector<aimrt_string_view_t> vec = [](void* impl) -> std::vector<aimrt_string_view_t> {
            auto& v = static_cast<PyTypeSupportBase*>(impl)->serialization_types_supported_vec;

            std::vector<aimrt_string_view_t> result;
            for (auto& item : v)
              result.emplace_back(aimrt::util::ToAimRTStringView(item));

            return result;
          }(impl);

          return vec.data();
        },
        .custom_type_support_ptr = [](void* impl) -> const void* {
          return nullptr;
        },
        .impl = impl};
  }

 private:
  aimrt_type_support_base_t base_;
};

class PyTypeSupportBaseAdapter : public PyTypeSupportBase {
 public:
  using PyTypeSupportBase::PyTypeSupportBase;

  void* Create() const override {
    PYBIND11_OVERRIDE_PURE(void*, PyTypeSupportBase, Create);
  }

  void Destroy(void* msg) const override {
    PYBIND11_OVERRIDE_PURE(void, PyTypeSupportBase, Destroy, msg);
  }

  void Copy(const void* from, void* to) const override {
    PYBIND11_OVERRIDE_PURE(void, PyTypeSupportBase, Copy, from, to);
  }

  void Move(void* from, void* to) const override {
    PYBIND11_OVERRIDE_PURE(void, PyTypeSupportBase, Move, from, to);
  }

  bool Serialize(
      std::string_view serialization_type,
      const void* msg,
      std::vector<uint8_t>& buffer) const override {
    PYBIND11_OVERRIDE_PURE(bool, PyTypeSupportBase, Serialize, serialization_type, msg, buffer);
  }

  bool Deserialize(
      std::string_view serialization_type,
      std::string_view buffer,
      void* msg) const override {
    PYBIND11_OVERRIDE_PURE(bool, PyTypeSupportBase, Deserialize, serialization_type, buffer, msg);
  }
};

inline void ExportTypeSupportBase(pybind11::object m) {
  pybind11::class_<PyTypeSupportBase, PyTypeSupportBaseAdapter, std::shared_ptr<PyTypeSupportBase>>(m, "TypeSupportBase")
      .def(pybind11::init<>())
      .def_readwrite("type_name", &PyTypeSupportBase::type_name)
      .def("Create", &PyTypeSupportBase::Create)
      .def("Destroy", &PyTypeSupportBase::Destroy)
      .def("Copy", &PyTypeSupportBase::Copy)
      .def("Move", &PyTypeSupportBase::Move)
      .def("Serialize", &PyTypeSupportBase::Serialize)
      .def("Deserialize", &PyTypeSupportBase::Deserialize)
      .def_readwrite("serialization_types_supported_vec", &PyTypeSupportBase::serialization_types_supported_vec);
}

}  // namespace aimrt::runtime::python_runtime