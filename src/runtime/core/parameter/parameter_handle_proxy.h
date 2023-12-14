#pragma once

#include "aimrt_module_c_interface/parameter/parameter_handle_base.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "core/parameter/parameter_handle.h"

namespace aimrt::runtime::core::parameter {

class ParameterHandleProxy {
 public:
  explicit ParameterHandleProxy(ParameterHandle& parameter_handle)
      : base_(GenBase(&parameter_handle)) {}
  ~ParameterHandleProxy() = default;

  ParameterHandleProxy(const ParameterHandleProxy&) = delete;
  ParameterHandleProxy& operator=(const ParameterHandleProxy&) = delete;

  const aimrt_parameter_handle_base_t* NativeHandle() const { return &base_; }

 private:
  static aimrt_parameter_handle_base_t GenBase(void* impl) {
    return aimrt_parameter_handle_base_t{
        .get_parameter = [](void* impl, aimrt_string_view_t name) -> aimrt_parameter_view_holder_t {
          auto ptr = static_cast<ParameterHandle*>(impl)->GetParameter(aimrt::util::ToStdStringView(name));
          if (!ptr->IsArray()) {
            return aimrt_parameter_view_holder_t{
                .parameter_view = ptr->GetView(),
                .release_callback = nullptr};
          }

          auto* f = new aimrt::util::Function<aimrt_function_parameter_ref_release_callback_ops_t>();
          (*f) = [ptr, f]() { delete f; };

          return aimrt_parameter_view_holder_t{
              .parameter_view = ptr->GetView(),
              .release_callback = f->NativeHandle()};
        },
        .set_parameter = [](void* impl, aimrt_string_view_t name, aimrt_parameter_view_t parameter) -> bool {
          auto ptr = std::make_shared<Parameter>();
          if (!ptr->SetData(parameter)) [[unlikely]]
            return false;

          static_cast<ParameterHandle*>(impl)->SetParameter(aimrt::util::ToStdStringView(name), ptr);

          return true;
        },
        .impl = impl};
  }

 private:
  const aimrt_parameter_handle_base_t base_;
};

}  // namespace aimrt::runtime::core::parameter
