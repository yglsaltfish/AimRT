#pragma once

#include "zenoh.h"
#include "zenoh_plugin/global.h"
#include "zenoh_plugin/msg_handle_registry.h"

namespace aimrt::plugins::zenoh_plugin {
class ZenohUtil {
 public:
  void ZenohSetCallbacks(std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr_);
  void ZenohSetRole(std::string role);
  bool ZenohRegisterRole(std::string keyexpr);

  bool ZenohPublish(std::string topic, char *serialized_data_ptr, int32_t pkg_size);

  void ZenohInitialize();
  bool ZenohUnsubscribe();
  bool ZenohUnPublish();

 private:
  bool ZenohRegisterSubscribe(std::string keyexpr);
  bool ZenohRegisterPublish(std::string keyexpr);

  std::unordered_map<std::string, z_owned_publisher_t> z_pub_registry_;
  std::unordered_map<std::string, z_owned_subscriber_t> z_sub_registry_;

  z_publisher_put_options_t z_pub_options_;

  z_owned_closure_sample_t z_callback_;

  z_owned_session_t z_session_;
  z_owned_config_t z_config_;

  std::string z_role_;
};

}  // namespace aimrt::plugins::zenoh_plugin