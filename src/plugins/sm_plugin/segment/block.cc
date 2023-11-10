
#include <iostream>

#include "block.h"

namespace aimrt::plugins::sm_plugin {

constexpr int32_t ConstLockFree = 0;
constexpr int32_t ConstLockWrite = -1;
constexpr int32_t ConstLockReadCeiling = 10;

// // check if the pid process is alive
// static inline bool IsPidAlive(const pid_t pid) {
//   return kill(pid, 0) == 0 || errno != ESRCH;
// }

Block::Block() {}

Block::~Block() {}

uint32_t Block::msg_size() const { return msg_size_; }

void Block::SetMsgSize(uint32_t msg_size) { msg_size_ = msg_size; }

bool Block::TryLockForWrite() {
  int32_t expected = ConstLockFree;
  return lock_.compare_exchange_strong(expected, ConstLockWrite,
                                       std::memory_order_acq_rel,
                                       std::memory_order_relaxed);
  // todo: record pid information to avoid memory locks due to process crashes
}

bool Block::TryLockForRead() {
  int32_t lock = lock_.load(std::memory_order_acquire);
  if (lock < ConstLockFree) {
    std::cout << "block is locked for write ..." << std::endl;
    return false;
  }
  int32_t try_count = 0;
  while (!lock_.compare_exchange_weak(lock, lock + 1, std::memory_order_acq_rel,
                                      std::memory_order_relaxed)) {
    if (++try_count > ConstLockReadCeiling) {
      std::cout << "try lock for read ceiling ..." << std::endl;
      return false;
    }

    lock = lock_.load(std::memory_order_acquire);
    if (lock < ConstLockFree) {
      std::cout << "block is locked for write ..." << std::endl;
      return false;
    }
  }
  return true;
}

void Block::ReleaseWriteLock() {
  if (lock_.load(std::memory_order_acquire) != ConstLockWrite) {
    // std::cout << "block is not locked for write ..." << std::endl;
    return;
  }
  lock_.store(ConstLockFree, std::memory_order_release);
}

void Block::ReleaseReadLock() {
  if (lock_.load(std::memory_order_acquire) <= ConstLockFree) {
    // std::cout << "block is not locked for read ..." << std::endl;
    return;
  }

  lock_.fetch_sub(1, std::memory_order_release);
}

}  // namespace aimrt::plugins::sm_plugin