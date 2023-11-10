
#include <sys/ipc.h>
#include <sys/shm.h>
#include <functional>
#include <limits>
#include <stdexcept>

#include "xsi_shm.h"

namespace aimrt::plugins::sm_plugin {

XsiSharedMemory::XsiSharedMemory() : shm_id_(-1), memory_(nullptr) {}
XsiSharedMemory::~XsiSharedMemory() { Close(); }

void* XsiSharedMemory::Create(const std::string_view& name, size_t size) {
  if (name.empty()) {
    throw std::runtime_error("Invalid shared memory name");
  }

  key_t key = std::hash<std::string_view>{}(name) % std::numeric_limits<key_t>::max();
  if (key == -1) {
    throw std::runtime_error("Failed to create XSI key");
  }

  shm_id_ = shmget(key, size, IPC_CREAT | IPC_EXCL | 0644);

  if (EEXIST == errno) {
    throw std::runtime_error(std::string("shm key [" + std::to_string(key) + "] already exist, please delete it first"));
  }

  if (shm_id_ == -1) {
    throw std::runtime_error("Failed to create XSI shared memory");
  }

  memory_ = shmat(shm_id_, nullptr, 0);
  if (memory_ == reinterpret_cast<void*>(-1)) {
    Destroy();
    throw std::runtime_error("Failed to attach XSI shared memory");
  }

  return memory_;
}

void* XsiSharedMemory::Open(const std::string_view& name) {
  key_t key = std::hash<std::string_view>{}(name) % std::numeric_limits<key_t>::max();
  if (key == -1) {
    throw std::runtime_error("Failed to create XSI key");
  }

  shm_id_ = shmget(key, 0, 0644);
  if (shm_id_ == -1) {
    throw std::runtime_error("Failed to open XSI shared memory");
  }

  // attach the shared memory segment to our process's address space.
  memory_ = shmat(shm_id_, nullptr, 0);
  if (memory_ == reinterpret_cast<void*>(-1)) {
    shm_id_ = -1;
    throw std::runtime_error("Failed to attach XSI shared memory");
  }

  return memory_;
}

bool XsiSharedMemory::Close() {
  if (memory_) {
    if (shmdt(memory_) == 0) {
      memory_ = nullptr;
      return true;
    }
  }
  return false;
}

bool XsiSharedMemory::Destroy() {
  Close();
  if (shm_id_ != -1) {
    if (shmctl(shm_id_, IPC_RMID, nullptr) == 0) {
      shm_id_ = -1;
      return true;
    }
  }
  return false;
}

}  // namespace aimrt::plugins::sm_plugin