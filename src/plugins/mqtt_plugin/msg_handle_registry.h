#pragma once

#include <string>
#include <unordered_map>

#include "util/string_util.h"

#include "MQTTClient.h"

#include "mqtt_plugin/global.h"

namespace aimrt::plugins::mqtt_plugin {

class MsgHandleRegistry {
 public:
  using MsgHandleFunc = std::function<void(MQTTClient_message* message)>;

  MsgHandleRegistry() = default;
  ~MsgHandleRegistry() = default;

  MsgHandleRegistry(const MsgHandleRegistry&) = delete;
  MsgHandleRegistry& operator=(const MsgHandleRegistry&) = delete;

  template <typename... Args>
    requires std::constructible_from<MsgHandleFunc, Args...>
  void RegisterMsgHandle(std::string_view topic, std::string_view uri, Args&&... args) {
    msg_handle_map_[std::string(topic)].emplace(uri, std::forward<Args>(args)...);
  }

  template <typename... Args>
    requires std::constructible_from<MsgHandleFunc, Args...>
  void RegisterRpcHandle(std::string_view topic, Args&&... args) {
    rpc_handle_map_.emplace(topic, std::forward<Args>(args)...);
  }

  void HandleServerMsg(std::string_view topic,
                       MQTTClient_message* message) const {
    AIMRT_TRACE("Mqtt recv msg, topic: {}", topic);
    try {
      if (aimrt::common::util::StartsWith(topic, "aimrt_rpc_")) {
        auto find_topic_itr = rpc_handle_map_.find(topic);
        if (find_topic_itr == rpc_handle_map_.end()) {
          AIMRT_WARN("Unregisted topic: {}", topic);
          return;
        }

        find_topic_itr->second(message);

      } else {
        // 1 byte uri len : n byte uri : buf.len-1-n byte data
        const void* buf_data = message->payload;
        size_t buf_size = message->payloadlen;

        AIMRT_CHECK_ERROR_THROW(buf_size > 1, "Invalid msg, topic: {}, buf size: {}", topic, buf_size);

        uint8_t uri_size = static_cast<const uint8_t*>(buf_data)[0];
        AIMRT_CHECK_ERROR_THROW(buf_size > uri_size + 1,
                                "Invalid msg, topic: {}, buf size: {}, uri size: {}",
                                topic, buf_size, uri_size);

        std::string_view uri = std::string_view(static_cast<const char*>(buf_data) + 1, uri_size);

        auto find_topic_itr = msg_handle_map_.find(topic);
        if (find_topic_itr == msg_handle_map_.end()) {
          AIMRT_WARN("Unregisted topic: {}", topic);
          return;
        }

        auto find_uri_itr = find_topic_itr->second.find(uri);
        if (find_uri_itr == find_topic_itr->second.end()) {
          AIMRT_WARN("Unregisted uri: {}, topic: {}", uri, topic);
          return;
        }

        find_uri_itr->second(message);
      }

    } catch (const std::exception& e) {
      AIMRT_ERROR("Handle msg failed, topic: {}, exception info: {}", topic, e.what());
      return;
    }
  }

 private:
  using UriMsgHandleMap = std::unordered_map<std::string, MsgHandleFunc, aimrt::common::util::StringHash, std::equal_to<>>;
  std::unordered_map<std::string, UriMsgHandleMap, aimrt::common::util::StringHash, std::equal_to<>> msg_handle_map_;

  UriMsgHandleMap rpc_handle_map_;
};

}  // namespace aimrt::plugins::mqtt_plugin
