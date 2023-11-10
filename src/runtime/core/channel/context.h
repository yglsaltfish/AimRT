#pragma once

#include <map>
#include <string>

#include "aimrt_module_c_interface/channel/channel_context_base.h"

namespace aimrt::runtime::core::channel {

class ContextImpl {
 public:
  ContextImpl();
  ~ContextImpl() = default;

  std::string_view GetMetaValue(std::string_view key) const;
  void SetMetaValue(std::string_view key, std::string_view val);

  uint64_t GetMsgTimestampNs() const;
  void SetMsgTimestampNs(uint64_t t);

  const aimrt_channel_context_base_t* NativeHandle() const { return &base_; }

 private:
  uint64_t t_ = 0;
  std::map<std::string, std::string, std::less<> > meta_data_map_;
  const aimrt_channel_context_base_t base_;
};

}  // namespace aimrt::runtime::core::channel
