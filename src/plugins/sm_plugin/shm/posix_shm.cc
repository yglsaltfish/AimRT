#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "posix_shm.h"

namespace aimrt::plugins::sm_plugin {

PosixSharedMemory::PosixSharedMemory() : memory_(nullptr), size_(0) {}

PosixSharedMemory::~PosixSharedMemory() { Close(); }

void* PosixSharedMemory::Create(const std::string_view& name, size_t size) {
  if (name.empty()) {
    throw std::runtime_error("Invalid shared memory name");
  }

  size_ = size;
  name_ = std::to_string(std::hash<std::string_view>{}(name));

  int fd = shm_open(name_.c_str(), O_RDWR | O_CREAT | O_EXCL, 0644);
  if (fd < 0) {
    if (EEXIST == errno) {
      throw std::runtime_error(std::string("shm [" + name_ + "] already exist, please delete it first"));
    } else {
      throw std::runtime_error("Failed to create shared memory");
    }
  }

  if (ftruncate(fd, size) == -1) {
    close(fd);
    throw std::runtime_error("Failed to resize shared memory, errno: " + std::string(strerror(errno)));
  }

  // attach the shared memory segment to our process's address space.
  memory_ = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (memory_ == MAP_FAILED) {
    close(fd);
    Destroy();
    throw std::runtime_error("Failed to map shared memory, errno: " + std::string(strerror(errno)));
  }

  close(fd);
  return memory_;
}

void* PosixSharedMemory::Open(const std::string_view& name) {
  name_ = std::to_string(std::hash<std::string_view>{}(name));
  int fd = shm_open(name_.c_str(), O_RDWR, 0644);
  if (fd == -1) {
    throw std::runtime_error("Failed to open shared memory, errno: " + std::string(strerror(errno)));
  }

  struct stat buffer;
  if (fstat(fd, &buffer) == -1) {
    close(fd);
    throw std::runtime_error("Failed to get shared memory size");
  }

  memory_ = mmap(nullptr, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (memory_ == MAP_FAILED) {
    close(fd);
    throw std::runtime_error("Failed to map shared memory, errno: " + std::string(strerror(errno)));
  }

  size_ = buffer.st_size;

  close(fd);

  return memory_;
}

bool PosixSharedMemory::Close() {
  if (memory_ && (size_ != 0)) {
    if (munmap(memory_, size_) == 0) {
      memory_ = nullptr;
      size_ = 0;
      return true;
    }
  }
  return false;
}

bool PosixSharedMemory::Destroy() {
  Close();
  if (shm_unlink(name_.c_str()) == 0) {
    return true;
  }

  return false;
}

}  // namespace aimrt::plugins::sm_plugin