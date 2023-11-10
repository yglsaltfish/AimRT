#include "core/rpc/context.h"
#include "aimrt_module_cpp_interface/util/string.h"

namespace aimrt::runtime::core::rpc {

static constexpr aimrt_rpc_context_base_ops_t rpc_context_base_ops{
    .get_deadline_ns = [](void* impl) -> uint64_t {
      return static_cast<ContextImpl*>(impl)->DeadlineNs();
    },
    .set_deadline_ns = [](void* impl, uint64_t ddl) {  //
      static_cast<ContextImpl*>(impl)->SetDeadlineNs(ddl);
    },
    .get_meta_val = [](void* impl, aimrt_string_view_t key) -> aimrt_string_view_t {
      return aimrt::ToAimRTStringView(
          static_cast<ContextImpl*>(impl)->GetMetaValue(
              aimrt::ToStdStringView(key)));
    },
    .set_meta_val = [](void* impl, aimrt_string_view_t key, aimrt_string_view_t val) {  //
      static_cast<ContextImpl*>(impl)->SetMetaValue(
          aimrt::ToStdStringView(key), aimrt::ToStdStringView(val));
    }};

ContextImpl::ContextImpl()
    : base_(aimrt_rpc_context_base_t{
          .ops = &rpc_context_base_ops,
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

uint64_t ContextImpl::DeadlineNs() const { return ddl_; }

void ContextImpl::SetDeadlineNs(uint64_t ddl) { ddl_ = ddl; }

}  // namespace aimrt::runtime::core::rpc