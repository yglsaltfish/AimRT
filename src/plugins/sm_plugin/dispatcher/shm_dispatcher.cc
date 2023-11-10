
#include <iostream>
#include <thread>

#include "shm_dispatcher.h"

#include "../notifier/notifier_factory.h"

namespace aimrt::plugins::sm_plugin {

constexpr static int ConstDefaultWaitTimeUs = 20;

ShmDisPatcher::ShmDisPatcher() : DisPatcherBase() {}

ShmDisPatcher::~ShmDisPatcher() {
  Shutdown();

  for (auto& [id, segment] : segment_map_) {
    segment = nullptr;
  }

  for (auto& [id, notifier_handler_pair] : listener_handler_map_) {
    notifier_handler_pair.first->Shutdown();
    notifier_handler_pair.first = nullptr;
  }

  segment_map_.clear();
  listener_handler_map_.clear();
}

bool ShmDisPatcher::AddListener(const DisPatcherAttribute& attribute, const ListenerHandler& handler) {
  std::string key = attribute.channel_name + ":" + attribute.msg_type;

  if (segment_map_.count(attribute.channel_id) != 0 || listener_handler_map_.count(attribute.channel_id) != 0) {
    return true;
  }

  auto segment_ptr = std::make_shared<Segment>(key);
  if (segment_ptr == nullptr) {
    std::cout << "create segment failed ..." << std::endl;
    return false;
  }

  NotifierBasePtr notifier_ptr = NotifierFactory::Create(NotifierFactory::NotifierType::SHM);

  if (notifier_ptr == nullptr) {
    std::cout << "create notifier failed ..." << std::endl;
    return false;
  }

  if (!notifier_ptr->Init(key)) {
    std::cout << "init notifier failed ..." << std::endl;
    return false;
  }

  segment_map_.emplace(attribute.channel_id, segment_ptr);
  listener_handler_map_.insert(std::make_pair(attribute.channel_id, std::make_pair(notifier_ptr, handler)));

  // std::cout << "sm dispatcher[" << this << "] add listener for channel [" << key << "] success." << std::endl;

  return true;
}

void ShmDisPatcher::Run(int wait_time_ms) {
  bool is_wait_forever = (wait_time_ms < 0);
  int64_t wait_time_us = wait_time_ms * 1000;

  if (listener_handler_map_.empty()) {
    return;
  }

  while (running_) {
    // 遍历 listener_handler_map_ 中的 NotifierBasePtr
    for (auto& listener_handler_pair : listener_handler_map_) {
      auto& notifier_ptr = listener_handler_pair.second.first;
      auto& handler = listener_handler_pair.second.second;
      Notification notification;
      bool readable = notifier_ptr->Listen(&notification, 0);
      if (readable && notification) {
        uint64_t channel_id = notification.channel_id();
        uint32_t block_index = notification.block_index();
        // uint32_t host_id = notification.host_id(); // todo

        auto& segment = segment_map_[channel_id];

        ReadableBlock readable_block;
        readable_block.index = block_index;
        if (segment->TryAcquireBlockForRead(&readable_block)) {
          handler(readable_block.buf, readable_block.block->msg_size(), readable_block.context);
          segment->ReleaseReadBlock(readable_block);
        }
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
        return;
      }
    }
  }
}

void ShmDisPatcher::Shutdown() {
  running_ = false;
}

}  // namespace aimrt::plugins::sm_plugin