#include "core/channel/context.h"
#include "aimrt_module_cpp_interface/util/string.h"

namespace aimrt::runtime::core::channel {

static constexpr aimrt_channel_context_base_ops_t channel_context_base_ops{
    .get_msg_timestamp_ns = [](void* impl) -> uint64_t {
      return static_cast<ContextImpl*>(impl)->GetMsgTimestampNs();
    },
    .set_msg_timestamp_ns = [](void* impl, uint64_t ddl) {  //
      static_cast<ContextImpl*>(impl)->SetMsgTimestampNs(ddl);
    },
    .get_meta_val = [](void* impl, aimrt_string_view_t key) -> aimrt_string_view_t {
      return aimrt::util::ToAimRTStringView(
          static_cast<ContextImpl*>(impl)->GetMetaValue(aimrt::util::ToStdStringView(key)));
    },
    .set_meta_val = [](void* impl, aimrt_string_view_t key, aimrt_string_view_t val) {  //
      static_cast<ContextImpl*>(impl)->SetMetaValue(
          aimrt::util::ToStdStringView(key), aimrt::util::ToStdStringView(val));
    }};

ContextImpl::ContextImpl()
    : base_(aimrt_channel_context_base_t{
          .ops = &channel_context_base_ops,
          .impl = this}) {}

std::string_view ContextImpl::GetMetaValue(std::string_view key) const {
  auto finditr = meta_data_map_.find(key);
  if (finditr != meta_data_map_.end()) return finditr->second;
  return "";
}

void ContextImpl::SetMetaValue(std::string_view key, std::string_view val) {
  auto finditr = meta_data_map_.find(key);
  if (finditr != meta_data_map_.end()) {
    finditr->second = std::string(val);
    return;
  }

  meta_data_map_.emplace(key, val);
}

uint64_t ContextImpl::GetMsgTimestampNs() const { return t_; }

void ContextImpl::SetMsgTimestampNs(uint64_t t) { t_ = t; }

}  // namespace aimrt::runtime::core::channel