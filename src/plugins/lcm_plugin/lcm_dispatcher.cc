
#include <iostream>
#include <thread>

#include "lcm_dispatcher.h"

namespace aimrt::plugins::lcm_plugin {

constexpr static int ConstDefaultWaitTimeUs = 30;

LcmDispatcher::LcmDispatcher() {}

LcmDispatcher::~LcmDispatcher() {
  Shutdown();
  lcm_map_.clear();
  handler_map_.clear();
}

bool LcmDispatcher::AddListener(const DisPatcherAttribute& attribute, const ListenerHandler& handler) {
  LcmPtr lcm = nullptr;
  if (lcm_map_.count(attribute.url) != 0) {
    lcm = lcm_map_[attribute.url];
  } else {
    lcm = LcmManager::GetInstance().GetLcm(attribute.url);
    if (lcm == nullptr) {
      return false;
    }
    lcm_map_[attribute.url] = lcm;
  }

  handler_map_[attribute.channel_name] = handler;

  lcm->subscribe(attribute.channel_name, &LcmDispatcher::HandleMessage, this);

  // std::cout << "lcm ['" << attribute.url << "'] add listener for channel [" << attribute.channel_name << "] success."
  //           << std::endl;

  return true;
}

void LcmDispatcher::Run(int wait_time_ms) {
  bool is_wait_forever = (wait_time_ms < 0);
  int64_t wait_time_us = wait_time_ms * 1000;

  if (handler_map_.empty()) {
    return;
  }
  while (running_) {
    for (auto& lcm : lcm_map_) {
      lcm.second->handleTimeout(0);
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

void LcmDispatcher::Shutdown() {
  running_ = false;
}

void LcmDispatcher::HandleMessage(const lcm::ReceiveBuffer* rbuf, const std::string& channel) {
  if (handler_map_.count(channel) == 0) {
    return;
  }

  auto& handler = handler_map_[channel];
  handler(rbuf->data, rbuf->data_size);
}

}  // namespace aimrt::plugins::lcm_plugin
