
#pragma once

#include <cinttypes>
#include <string>

namespace aimrt::plugins::sm_plugin {

class ResourceManager {
 public:
  ResourceManager();
  virtual ~ResourceManager();

  bool UpdateMsgSize(const uint32_t msg_size);

  uint32_t managed_size() const;
  uint32_t real_block_size() const;
  uint32_t ceiling_msg_size() const;
  uint32_t context_size() const;
  uint32_t block_count() const;
  uint32_t extra_size() const;

 private:
  uint64_t managed_size_{0};      // 管理的共享内存大小
  uint32_t real_block_size_{0};   // 真正的块大小
  uint32_t ceiling_msg_size_{0};  // 消息大小上限
  uint32_t context_size_{0};      // 消息上下文大小
  uint32_t block_count_{0};       // 共享内存上的块数
  uint32_t extra_size_{0};        // 扩展的共享内存大小
};

}  // namespace aimrt::plugins::sm_plugin