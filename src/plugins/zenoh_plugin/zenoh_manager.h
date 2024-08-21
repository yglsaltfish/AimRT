// Copyright (c) 2023, AgiBot Inc.
// All rights reserved

#pragma once

#include "zenoh.h"
#include "zenoh_plugin/global.h"
#include "zenoh_plugin/msg_handle_registry.h"

namespace aimrt::plugins::zenoh_plugin {
class ZenohManager {
 public:
  void RegisterMsgHandleRegistry(std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr_);

  bool RegisterSubscriber(std::string url);
  bool RegisterPublisher(std::string url);

  bool Publish(std::string url, char *serialized_data_ptr, int32_t pkg_size);

  void Initialize();
  void Shutdown();

 private:
  std::unordered_map<std::string, z_owned_publisher_t> z_pub_registry_;
  std::unordered_map<std::string, z_owned_subscriber_t> z_sub_registry_;
  std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr_;

  z_publisher_put_options_t z_pub_options_;

  z_owned_session_t z_session_;
  z_owned_config_t z_config_;
};

}  // namespace aimrt::plugins::zenoh_plugin