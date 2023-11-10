
#include "shm_factory.h"

#include "posix_shm.h"
#include "xsi_shm.h"

namespace aimrt::plugins::sm_plugin {

SharedMemoryBasePtr SharedMemoryFactory::Create(SharedMemoryType type) {
  switch (type) {
    case SharedMemoryType::XSI:
      return std::make_shared<XsiSharedMemory>();
    case SharedMemoryType::POSIX:
      return std::make_shared<PosixSharedMemory>();
    default:
      return nullptr;
  }
}

}  // namespace aimrt::plugins::sm_plugin
