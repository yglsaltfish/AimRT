#pragma once

#include <functional>

#include "aimrt_module_cpp_interface/channel/channel_context.h"
#include "aimrt_module_cpp_interface/util/type_support.h"

namespace aimrt::runtime::core::channel {

struct FrameworkFilterData {
  std::string_view topic_name;
  std::string_view msg_type;
  std::string_view pkg_path;
  std::string_view module_name;
  aimrt::util::TypeSupportRef type_support_ref;
};

using FrameworkAsyncChannelHandle =
    std::function<void(const FrameworkFilterData&, aimrt::channel::ContextRef, const void*)>;
using FrameworkAsyncChannelFilter =
    std::function<void(const FrameworkFilterData&, aimrt::channel::ContextRef, const void*, FrameworkAsyncChannelHandle&&)>;

class FrameworkAsyncFilterManager {
 public:
  FrameworkAsyncFilterManager()
      : final_filter_(
            [](const FrameworkFilterData& filter_data,
               aimrt::channel::ContextRef ctx_ref,
               const void* msg,
               FrameworkAsyncChannelHandle&& h) {
              h(filter_data, ctx_ref, msg);
            }) {}
  ~FrameworkAsyncFilterManager() = default;

  FrameworkAsyncFilterManager(const FrameworkAsyncFilterManager&) = delete;
  FrameworkAsyncFilterManager& operator=(const FrameworkAsyncFilterManager&) = delete;

  void RegisterFilter(FrameworkAsyncChannelFilter&& filter) {
    final_filter_ =
        [final_filter{std::move(final_filter_)},
         cur_filter{std::move(filter)}](
            const FrameworkFilterData& filter_data,
            aimrt::channel::ContextRef ctx_ref,
            const void* msg,
            FrameworkAsyncChannelHandle&& h) {
          cur_filter(
              filter_data,
              ctx_ref,
              msg,
              [final_filter{std::move(final_filter)}, h{std::move(h)}](
                  const FrameworkFilterData& filter_data,
                  aimrt::channel::ContextRef ctx_ref,
                  const void* msg) mutable {
                final_filter(filter_data, ctx_ref, msg, std::move(h));
              });
        };
  }

  void InvokeChannel(
      FrameworkAsyncChannelHandle&& h,
      const FrameworkFilterData& filter_data,
      aimrt::channel::ContextRef ctx_ref,
      const void* msg) const {
    return final_filter_(filter_data, ctx_ref, msg, std::move(h));
  }

  void Clear() {
    final_filter_ = FrameworkAsyncChannelFilter();
  }

 private:
  FrameworkAsyncChannelFilter final_filter_;
};

}  // namespace aimrt::runtime::core::channel