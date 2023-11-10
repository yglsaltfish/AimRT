
#pragma once

#include "shm_base.h"

namespace aimrt::plugins::sm_plugin {

class XsiSharedMemory : public SharedMemoryBase {
 public:
  XsiSharedMemory();
  virtual ~XsiSharedMemory();

  void* Create(const std::string_view& name, size_t size) override;
  void* Open(const std::string_view& name) override;
  bool Close() override;
  bool Destroy() override;

 private:
  int shm_id_;
  void* memory_;
};

using XsiSharedMemoryPtr = std::shared_ptr<XsiSharedMemory>;

}  // namespace aimrt::plugins::sm_plugin