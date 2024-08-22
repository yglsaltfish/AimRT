// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include <string>
#include <unordered_map>

#include "util/string_util.h"

#include "zenoh.h"

#include "zenoh_plugin/global.h"

namespace aimrt::plugins::zenoh_plugin {

class MsgHandleRegistry {
 public:
  // zenoh中函数回调用于处理sample（即数据单元, 包含负载、资源标识符、时间戳等资源）
  using MsgHandleFunc = std::function<void(const z_loaned_sample_t* message)>;

  MsgHandleRegistry() = default;
  ~MsgHandleRegistry() = default;

  MsgHandleRegistry(const MsgHandleRegistry&) = delete;
  MsgHandleRegistry& operator=(const MsgHandleRegistry&) = delete;

  // 确保注册的回调函数满足MshandleFunc构造要求，并与topic(url)进行绑定 注册到map中，便于后续查询和管理
  template <typename... Args>
    requires std::constructible_from<MsgHandleFunc, Args...>
  void RegisterMsgHandle(std::string_view url, Args&&... args) {
    msg_handle_map_.emplace(url, std::forward<Args>(args)...);
  }

  // 根据主题找到对应的处理函数，用于处理收到的messgae
  void HandleServerMsg(std::string_view url, const z_loaned_sample_t* message) const {
    if (shutdown_flag_.load()) [[unlikely]]
      return;

    AIMRT_TRACE("Zenoh recv msg, topic: {}", url);

    try {
      auto find_topic_itr = msg_handle_map_.find(url);
      if (find_topic_itr == msg_handle_map_.end()) [[unlikely]] {
        AIMRT_WARN("Unregisted topic: {}", url);
        return;
      }

      find_topic_itr->second(message);

    } catch (const std::exception& e) {
      AIMRT_ERROR("Handle msg failed, topic: {}, exception info: {}", url, e.what());
      return;
    }
  }

  void Shutdown() {
    if (std::atomic_exchange(&shutdown_flag_, true)) return;
  }

 private:
  std::atomic_bool shutdown_flag_ = false;

  using UriMsgHandleMap = std::unordered_map<std::string, MsgHandleFunc, aimrt::common::util::StringHash, std::equal_to<>>;
  UriMsgHandleMap msg_handle_map_;
};

}  // namespace aimrt::plugins::zenoh_plugin