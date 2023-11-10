

#pragma once

#include "shm_base.h"

namespace aimrt::plugins::sm_plugin {

class PosixSharedMemory : public SharedMemoryBase {
 public:
  PosixSharedMemory();
  virtual ~PosixSharedMemory();

  void* Create(const std::string_view& name, size_t size) override;
  void* Open(const std::string_view& name) override;
  bool Close() override;
  bool Destroy() override;

 private:
  void* memory_;
  size_t size_;
  std::string name_;
};

using PosixSharedMemoryPtr = std::shared_ptr<PosixSharedMemory>;

}  // namespace aimrt::plugins::sm_plugin