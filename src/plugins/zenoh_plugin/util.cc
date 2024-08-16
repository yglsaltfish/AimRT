#pragma once

#include "zenoh_plugin/util.h"
#include "zenoh_plugin/msg_handle_registry.h"

namespace aimrt::plugins::zenoh_plugin {

void ZenohUtil::ZenohInitialize() {
  z_config_default(&z_config_);
  if (z_open(&z_session_, z_move(z_config_)) < 0) {
    AIMRT_ERROR("Unable to open zenoh session!");
    return;
  }
}
void ZenohUtil::ZenohSetCallbacks(std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr) {
  msg_handle_registry_ptr_ = msg_handle_registry_ptr;
}
bool ZenohUtil::ZenohRegisterPublish(std::string keyexpr) {
  z_owned_publisher_t z_pub_;
  z_view_keyexpr_t key;
  z_view_keyexpr_from_str(&key, keyexpr.c_str());
  z_publisher_put_options_default(&z_pub_options_);
  if (z_declare_publisher(&z_pub_, z_loan(z_session_), z_loan(key), NULL) < 0) {
    AIMRT_ERROR("Unable to declare Publisher!");
    return false;
  }
  z_pub_registry_.emplace(keyexpr, std::move(z_pub_));
  return true;
}

bool ZenohUtil::ZenohRegisterSubscribe(std::string keyexpr) {
  z_owned_closure_sample_t z_callback_;
  z_closure(
      &z_callback_,
      [](const z_loaned_sample_t *sample, void *arg) -> void {
        auto msg_handle_registry_ptr = static_cast<MsgHandleRegistry *>(arg);
        z_view_string_t key_string;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &key_string);

        std::string topic = "/";
        topic.append(z_string_data(z_loan(key_string)), (int)z_string_len(z_loan(key_string)));
        msg_handle_registry_ptr->HandleServerMsg(topic, sample);
        return;
      },
      NULL,
      msg_handle_registry_ptr_.get());

  z_owned_subscriber_t z_sub_;
  z_view_keyexpr_t key;
  z_view_keyexpr_from_str(&key, keyexpr.c_str());

  if (z_declare_subscriber(&z_sub_, z_loan(z_session_), z_loan(key), z_move(z_callback_), NULL) < 0) {
    AIMRT_ERROR("Unable to declare Subscriber!");
    return false;
  }
  z_sub_registry_.emplace(keyexpr, std::move(z_sub_));
  return true;
}

bool ZenohUtil::ZenohPublish(std::string topic, char *serialized_data_ptr, int32_t pkg_size) {
  z_owned_bytes_t z_payload_;
  auto z_pub_iter = z_pub_registry_.find(topic.substr(1));
  if (z_pub_iter == z_pub_registry_.end()) {
    AIMRT_ERROR("Topic {} not registered for publishing!", topic);
    return false;
  }
  z_bytes_from_buf(&z_payload_, reinterpret_cast<uint8_t *>(serialized_data_ptr), pkg_size, NULL, NULL);
  z_publisher_put(z_loan(z_pub_iter->second), z_move(z_payload_), &z_pub_options_);
  return true;
}

void ZenohUtil::ZenohSetRole(std::string role) {
  z_role_ = role;
}

bool ZenohUtil::ZenohRegisterRole(std::string keyexpr) {
  if (z_role_ == "pub") {
    return ZenohRegisterPublish(keyexpr.substr(1));
  } else if (z_role_ == "sub") {
    return ZenohRegisterSubscribe(keyexpr.substr(1));

  } else {
    AIMRT_ERROR("Invalid Zenoh parameter : role: {}", z_role_);
    return false;
  }
  return true;
}

}  // namespace aimrt::plugins::zenoh_plugin