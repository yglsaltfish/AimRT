
#include <iostream>
#include <thread>

#include "../shm/shm_factory.h"
#include "shm_notifier.h"

namespace aimrt::plugins::sm_plugin {

constexpr static int ConstDefaultWaitTimeUs = 20;
constexpr static uint32_t ConstNotificationNum = 4096;

struct NotifierManager {
  std::atomic<uint32_t> reference{0};
  std::atomic<uint64_t> next_seq = {0};
  uint64_t seqs[ConstNotificationNum] = {0};
  Notification infos[ConstNotificationNum];
};

ShmNotifier::ShmNotifier() : NotifierBase() {
  shm_impl_ = SharedMemoryFactory::Create(SharedMemoryFactory::SharedMemoryType::POSIX);
}

ShmNotifier::~ShmNotifier() { Shutdown(); }

bool ShmNotifier::Init(const std::string& name) {
  if (name.empty()) {
    throw std::runtime_error("shm notifier name is empty");
  }

  name_ = name + ":notifier";

  if (!Open()) {
    if (!Create()) {
      return false;
    }
  }

  NotifierManager* notifier_manager = static_cast<NotifierManager*>(notifier_manager_);
  next_seq_ = notifier_manager->next_seq.load();
  is_shutdown_ = false;

  return true;
}

bool ShmNotifier::Shutdown() {
  if (is_shutdown_) {
    return true;
  }

  is_shutdown_ = true;

  return Destroy();
}

bool ShmNotifier::Notify(const Notification& info) {
  if (is_shutdown_) {
    return false;
  }

  if (notifier_manager_ == nullptr) {
    return false;
  }

  NotifierManager* notifier_manager = static_cast<NotifierManager*>(notifier_manager_);

  uint64_t seq = notifier_manager->next_seq.load();
  uint64_t index = seq % ConstNotificationNum;
  notifier_manager->seqs[index] = seq;
  notifier_manager->infos[index] = info;
  notifier_manager->next_seq.store(seq + 1, std::memory_order_release);

  return true;
}

bool ShmNotifier::Listen(Notification* info, int wait_time_ms) {
  if (is_shutdown_) {
    return false;
  }

  if (info == nullptr) {
    return false;
  }

  if (notifier_manager_ == nullptr) {
    return false;
  }

  bool is_wait_forever = (wait_time_ms < 0);
  int64_t wait_time_us = wait_time_ms * 1000;

  NotifierManager* notifier_manager = static_cast<NotifierManager*>(notifier_manager_);

  while (!is_shutdown_.load()) {
    uint64_t seq = notifier_manager->next_seq.load(std::memory_order_acquire);
    if (seq != next_seq_.load(std::memory_order_acquire)) {
      uint64_t index = next_seq_ % ConstNotificationNum;
      uint64_t actual_seq = notifier_manager->seqs[index];
      if (actual_seq >= next_seq_) {
        *info = notifier_manager->infos[index];
        next_seq_.store(actual_seq + 1);
        return true;
      } else {
        std::cout << "seq[" << next_seq_ << "] is writing, can not read now ..." << std::endl;
      }
    }

    if (is_wait_forever) {
      std::this_thread::sleep_for(std::chrono::microseconds(ConstDefaultWaitTimeUs));
      continue;
    } else {
      if (wait_time_us > 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(ConstDefaultWaitTimeUs));
        wait_time_us -= ConstDefaultWaitTimeUs;
      } else {
        return false;
      }
    }
  }

  return false;
}

bool ShmNotifier::Create() {
  // try create only
  void* memory = nullptr;
  try {
    memory = shm_impl_->Create(name_, sizeof(NotifierManager));
  } catch (...) {
    return false;
  }

  if (memory == nullptr) {
    return false;
  }

  // create notifier manager
  notifier_manager_ = new (memory) NotifierManager();
  if (notifier_manager_ == nullptr) {
    std::cout << "create notifier manager failed ..." << std::endl;
    Destroy();
    return false;
  }

  NotifierManager* notifier_manager = static_cast<NotifierManager*>(notifier_manager_);
  notifier_manager->reference.fetch_add(1);

  return true;
}

bool ShmNotifier::Open() {
  // try open only
  void* memory = nullptr;
  try {
    memory = shm_impl_->Open(name_);
  } catch (...) {
    return false;
  }

  if (memory == nullptr) {
    return false;
  }

  // get notifier manager
  notifier_manager_ = static_cast<NotifierManager*>(memory);
  if (notifier_manager_ == nullptr) {
    std::cout << "open notifier manager failed ..." << std::endl;
    Close();
    return false;
  }

  NotifierManager* notifier_manager = static_cast<NotifierManager*>(notifier_manager_);
  notifier_manager->reference.fetch_add(1);

  return true;
}

bool ShmNotifier::Close() {
  if (shm_impl_ == nullptr) {
    return false;
  }

  if (!shm_impl_->Close()) {
    return false;
  }
  return true;
}

bool ShmNotifier::Destroy() {
  if (shm_impl_ == nullptr) {
    return false;
  }

  if (notifier_manager_ == nullptr) {
    return true;
  }

  NotifierManager* notifier_manager = static_cast<NotifierManager*>(notifier_manager_);
  try {
    uint32_t current_reference_count = notifier_manager->reference.load();
    do {
      if (current_reference_count == 0) {
        return true;
      }
    } while (!notifier_manager->reference.compare_exchange_weak(
        current_reference_count, current_reference_count - 1));

    if (notifier_manager->reference.load(std::memory_order_acquire) == 0) {
      return shm_impl_->Destroy();
    }
  } catch (...) {
    return false;
  }

  notifier_manager_ = nullptr;

  return true;
}

}  // namespace aimrt::plugins::sm_plugin