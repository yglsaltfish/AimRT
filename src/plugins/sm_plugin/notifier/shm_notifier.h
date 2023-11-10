
#pragma once

#include "notifier_base.h"

#include "../shm/shm_base.h"

namespace aimrt::plugins::sm_plugin {

class ShmNotifier : public NotifierBase {
 public:
  ShmNotifier();
  virtual ~ShmNotifier();

  bool Init(const std::string& name) override;
  bool Shutdown() override;
  bool Notify(const Notification& info) override;

  // wait_time_ms < 0: block until notified
  bool Listen(Notification* info, int wait_time_ms = -1) override;

 private:
  bool Create();
  bool Open();
  bool Close();
  bool Destroy();

 private:
  std::string name_;
  void* notifier_manager_{nullptr};
  std::atomic<bool> is_shutdown_{true};
  std::atomic<uint64_t> next_seq_{0};
  SharedMemoryBasePtr shm_impl_{nullptr};
};

}  // namespace aimrt::plugins::sm_plugin