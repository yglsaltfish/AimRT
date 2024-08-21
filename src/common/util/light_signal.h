// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include <condition_variable>
#include <mutex>

namespace aimrt::common::util {

/**
 * @brief 轻量级信号量
 * @note 线程安全的轻量级信号量
 */
class LightSignal {
 public:
  LightSignal() = default;
  ~LightSignal() = default;

  /// 唤醒所有wait
  void Notify() {
    std::lock_guard<std::mutex> lck(mutex_);
    flag_ = true;
    cond_.notify_all();
  }

  /// 无限等待唤醒
  void Wait() {
    std::unique_lock<std::mutex> lck(mutex_);
    if (flag_) return;
    cond_.wait(lck);
  }

  /// 带超时的等待唤醒
  bool WaitFor(std::chrono::nanoseconds timeout) {
    std::unique_lock<std::mutex> lck(mutex_);
    if (flag_) return true;
    if (cond_.wait_for(lck, timeout) == std::cv_status::timeout) return false;
    return true;
  }

  /// 重置信号量
  void Reset() {
    std::lock_guard<std::mutex> lck(mutex_);
    flag_ = false;
  }

 private:
  std::mutex mutex_;
  std::condition_variable cond_;
  bool flag_ = false;
};

}  // namespace aimrt::common::util