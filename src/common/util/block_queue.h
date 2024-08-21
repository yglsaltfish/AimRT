// Copyright (c) 2023, AgiBot Inc.
// All rights reserved

#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>

namespace aimrt::common::util {

class BlockQueueStoppedException : public std::runtime_error {
 public:
  BlockQueueStoppedException() : std::runtime_error("BlockQueue is stopped") {}
};

template <class T>
class BlockQueue {
 public:
  BlockQueue() = default;
  ~BlockQueue() { Stop(); }

  BlockQueue(const BlockQueue &) = delete;
  BlockQueue &operator=(const BlockQueue &) = delete;

  /// 添加元素
  void Enqueue(const T &item) {
    std::unique_lock<std::mutex> lck(mutex_);
    if (!running_flag_) throw BlockQueueStoppedException();
    queue_.emplace(item);
    cond_.notify_one();
  }

  /// 添加元素
  void Enqueue(T &&item) {
    std::unique_lock<std::mutex> lck(mutex_);
    if (!running_flag_) throw BlockQueueStoppedException();
    queue_.emplace(std::move(item));
    cond_.notify_one();
  }

  /// 阻塞式取出元素
  T Dequeue() {
    std::unique_lock<std::mutex> lck(mutex_);
    cond_.wait(lck, [this] { return !queue_.empty() || !running_flag_; });
    if (!running_flag_) throw BlockQueueStoppedException();
    T item = std::move(queue_.front());
    queue_.pop();
    return item;
  }

  /// 非阻塞式取出元素
  std::optional<T> TryDequeue() {
    std::lock_guard<std::mutex> lck(mutex_);
    if (queue_.empty() || !running_flag_) [[unlikely]]
      return std::nullopt;

    T item = std::move(queue_.front());
    queue_.pop();
    return item;
  }

  void Stop() {
    std::unique_lock<std::mutex> lck(mutex_);
    running_flag_ = false;
    cond_.notify_all();
  }

  size_t Size() const {
    std::lock_guard<std::mutex> lck(mutex_);
    return queue_.size();
  }

  bool IsRunning() const {
    std::lock_guard<std::mutex> lck(mutex_);
    return running_flag_;
  }

 protected:
  mutable std::mutex mutex_;      ///< 同步锁
  std::condition_variable cond_;  ///< 条件锁
  std::queue<T> queue_;           ///< 队列
  bool running_flag_ = true;      ///< 运行标志，为false时，当队列为空阻塞式取出将失败
};
}  // namespace aimrt::common::util
