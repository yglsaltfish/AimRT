
#pragma once

#include <atomic>

namespace aimrt::plugins::sm_plugin {

class Block {
  friend class Segment;

 public:
  Block();
  virtual ~Block();

  uint32_t msg_size() const;
  void SetMsgSize(uint32_t msg_size);

 private:
  bool TryLockForWrite();
  bool TryLockForRead();
  void ReleaseWriteLock();
  void ReleaseReadLock();

 private:
  std::atomic<int32_t> lock_{0};
  uint32_t msg_size_{0};
  // pid_t pid_{0};  // todo: record pid
};

}  // namespace aimrt::plugins::sm_plugin