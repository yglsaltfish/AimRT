
#pragma once

#include <memory>
#include <string_view>

namespace aimrt::plugins::sm_plugin {

class SharedMemoryBase {
 public:
  virtual ~SharedMemoryBase() {}
  virtual void* Create(std::string_view name, size_t size) = 0;
  virtual void* Open(std::string_view name) = 0;
  virtual bool Close() = 0;
  virtual bool Destroy() = 0;
};

using SharedMemoryBasePtr = std::shared_ptr<SharedMemoryBase>;

}  // namespace aimrt::plugins::sm_plugin