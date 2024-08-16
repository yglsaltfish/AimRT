#pragma once

#include "zenoh_plugin/zenoh_manager.h"
#include "zenoh_plugin/msg_handle_registry.h"
#include "zenoh_plugin/util.h"

namespace aimrt::plugins::zenoh_plugin {

void ZenohManager::Initialize() {
  z_config_default(&z_config_);
  if (z_open(&z_session_, z_move(z_config_)) < 0) {
    AIMRT_ERROR("Unable to open zenoh session!");
    return;
  }
}

void ZenohManager::Shutdown() {
  for (auto ptr : z_pub_registry_) {
    z_undeclare_publisher(z_move(ptr.second));
  }

  for (auto ptr : z_sub_registry_) {
    z_undeclare_subscriber(z_move(ptr.second));
  }

  z_close(z_move(z_session_));
  printf("Zenoh manager shutdown\n");
}

void ZenohManager::SetCallbacks(std::shared_ptr<MsgHandleRegistry> msg_handle_registry_ptr) {
  msg_handle_registry_ptr_ = msg_handle_registry_ptr;
}

bool ZenohManager::RegisterPublisher(std::string url) {
  std::string keyexpr = Url2Keyexpr(url);
  z_view_keyexpr_t key;
  z_view_keyexpr_from_str(&key, keyexpr.c_str());

  z_owned_publisher_t z_pub_;
  z_publisher_put_options_default(&z_pub_options_);

  if (z_declare_publisher(&z_pub_, z_loan(z_session_), z_loan(key), NULL) < 0) {
    AIMRT_ERROR("Unable to declare Publisher!");
    return false;
  }

  z_pub_registry_.emplace(keyexpr, std::move(z_pub_));

  return true;
}

bool ZenohManager::RegisterSubscriber(std::string url) {
  std::string keyexpr = Url2Keyexpr(url);
  z_view_keyexpr_t key;
  z_view_keyexpr_from_str(&key, keyexpr.c_str());

  z_owned_closure_sample_t z_callback_;
  z_closure(
      &z_callback_,
      [](const z_loaned_sample_t *sample, void *arg) -> void {
        z_view_string_t key_string;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &key_string);

        std::string keyexpr = "";
        keyexpr.append(z_string_data(z_loan(key_string)), (int)z_string_len(z_loan(key_string)));

        std::string url = Keyexpr2Url(keyexpr);
        static_cast<MsgHandleRegistry *>(arg)->HandleServerMsg(url, sample);
      },

      NULL,

      msg_handle_registry_ptr_.get());

  z_owned_subscriber_t z_sub_;

  if (z_declare_subscriber(&z_sub_, z_loan(z_session_), z_loan(key), z_move(z_callback_), NULL) < 0) {
    AIMRT_ERROR("Unable to declare Subscriber!");
    return false;
  }

  z_sub_registry_.emplace(keyexpr, std::move(z_sub_));

  return true;
}

bool ZenohManager::Publish(std::string url, char *serialized_data_ptr, int32_t pkg_size) {
  std::string topic = Url2Keyexpr(url);

  auto z_pub_iter = z_pub_registry_.find(topic);
  if (z_pub_iter == z_pub_registry_.end()) {
    AIMRT_ERROR("Url: {} not registered for publishing!", url);
    return false;
  }

  z_owned_bytes_t z_payload_;

  z_bytes_from_buf(&z_payload_, reinterpret_cast<uint8_t *>(serialized_data_ptr), pkg_size, NULL, NULL);
  z_publisher_put(z_loan(z_pub_iter->second), z_move(z_payload_), &z_pub_options_);

  return true;
}

}  // namespace aimrt::plugins::zenoh_plugin