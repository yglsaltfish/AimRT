// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#include "zenoh_plugin/zenoh_manager.h"

namespace aimrt::plugins::zenoh_plugin {

void ZenohManager::Initialize(std::string &native_cfg_file_path) {
  if (!native_cfg_file_path.empty() && native_cfg_file_path.c_str() != nullptr) {
    if (zc_config_from_file(&z_config_, native_cfg_file_path.c_str()) != Z_OK) {
      AIMRT_ERROR("Unable to load configuration file: {}", native_cfg_file_path);
      return;
    }
    PrintZenohCgf(z_config_);
  } else {
    z_config_default(&z_config_);
  }

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

  msg_handle_vec_.clear();
  z_pub_registry_.clear();
  z_sub_registry_.clear();

  z_close(z_move(z_session_));
}

void ZenohManager::RegisterPublisher(const std::string &keyexpr) {
  z_view_keyexpr_t key;
  z_view_keyexpr_from_str(&key, keyexpr.c_str());

  z_owned_publisher_t z_pub;
  z_publisher_put_options_default(&z_pub_options_);

  if (z_declare_publisher(&z_pub, z_loan(z_session_), z_loan(key), NULL) < 0) {
    AIMRT_ERROR("Unable to declare Publisher!");
    return;
  }

  z_pub_registry_.emplace(keyexpr, z_pub);
  AIMRT_TRACE("Publisher with keyexpr: {} registered successfully.", keyexpr.c_str());

  return;
}

void ZenohManager::RegisterSubscriber(const std::string &keyexpr, MsgHandleFunc handle) {
  z_view_keyexpr_t key;
  z_view_keyexpr_from_str(&key, keyexpr.c_str());
  z_owned_closure_sample_t z_callback;
  auto function_ptr = std::make_shared<MsgHandleFunc>(std::move(handle));

  z_closure(
      &z_callback,
      [](const z_loaned_sample_t *sample, void *arg) { (*reinterpret_cast<MsgHandleFunc *>(arg))(sample); },

      nullptr,

      function_ptr.get());

  msg_handle_vec_.emplace_back(std::move(function_ptr));

  z_owned_subscriber_t z_sub;

  if (z_declare_subscriber(&z_sub, z_loan(z_session_), z_loan(key), z_move(z_callback), NULL) < 0) {
    AIMRT_ERROR("Unable to declare Subscriber!");
    return;
  }

  z_sub_registry_.emplace(keyexpr, z_sub);
  AIMRT_TRACE("Subscriber with keyexpr: {} registered successfully.", keyexpr.c_str());

  return;
}

void ZenohManager::RegisterServicer(const std::string &keyexpr, MsgQueryFunc handle) {
  z_view_keyexpr_t key;
  z_view_keyexpr_from_str(&key, keyexpr.c_str());
  z_owned_closure_query_t z_callback;
  auto function_ptr = std::make_shared<MsgQueryFunc>(std::move(handle));

  z_closure(
      &z_callback,
      [](const z_loaned_query_t *query, void *arg) { (*reinterpret_cast<MsgQueryFunc *>(arg))(query); },

      nullptr,

      function_ptr.get());

  msg_query_vec_.emplace_back(std::move(function_ptr));

  z_owned_queryable_t z_qable;
  if (z_declare_queryable(&z_qable, z_loan(z_session_), z_loan(key), z_move(z_callback), NULL) < 0) {
    AIMRT_ERROR("Unable to declare Server!");
    return;
  }
  z_query_reply_options_default(&z_reply_options_);
  z_srv_registry_.emplace(keyexpr, std::move(z_qable));
  AIMRT_TRACE("Server with keyexpr: {} registered successfully.", keyexpr.c_str());
  return;
}

void ZenohManager::RegisterClient(const std::string &keyexpr, MsgReplyFunc handle) {
  z_get_options_default(&z_get_options_);

  auto function_ptr = std::make_shared<MsgReplyFunc>(std::move(handle));
  msg_reply_registry_.emplace(keyexpr, std::move(function_ptr));
}

void ZenohManager::Publish(const std::string &topic, char *serialized_data_ptr, uint64_t serialized_data_len) {
  auto z_pub_iter = z_pub_registry_.find(topic);
  if (z_pub_iter == z_pub_registry_.end()) {
    AIMRT_ERROR("Url: {} is not registered!", topic);
    return;
  }

  z_owned_bytes_t z_payload;

  z_bytes_from_buf(&z_payload, reinterpret_cast<uint8_t *>(serialized_data_ptr), serialized_data_len, NULL, NULL);
  z_publisher_put(z_loan(z_pub_iter->second), z_move(z_payload), &z_pub_options_);

  return;
}

bool ZenohManager::Query(const std::string &keyexpr, char *serialized_data_ptr,
                         uint64_t serialized_data_len) {
  auto z_rep_iter = msg_reply_registry_.find(keyexpr);
  if (z_rep_iter == msg_reply_registry_.end()) [[unlikely]] {
    AIMRT_ERROR("Url: {} is not registered!", keyexpr);
    return false;
  }

  z_view_keyexpr_t z_get_key_;
  z_view_keyexpr_from_str(&z_get_key_, keyexpr.c_str());

  // recreate the closure object (the closure and opts are forcibly removed after each successful get)
  z_owned_closure_reply_t z_reply_closure_;
  z_closure(
      &z_reply_closure_,
      [](const z_loaned_reply_t *reply, void *arg) {
        if (z_reply_is_ok(reply)) {
          (*reinterpret_cast<MsgReplyFunc *>(arg))(reply);
        } else {
          AIMRT_ERROR("time out");
        }
      },

      NULL,

      z_rep_iter->second.get());

  z_owned_bytes_t z_get_payload;
  z_bytes_from_buf(&z_get_payload, reinterpret_cast<uint8_t *>(serialized_data_ptr),
                   serialized_data_len,
                   NULL,
                   NULL);
  z_get_options_.payload = &z_get_payload;
  auto z_result = z_get(z_loan(z_session_), z_loan(z_get_key_), "", &z_reply_closure_, &z_get_options_);
  z_drop(z_move(z_get_payload));

  if (z_result != Z_OK) [[unlikely]] {
    AIMRT_WARN("Client get request failed, error code: {}", z_result);
    return false;
  }

  return true;
}

void ZenohManager::Reply(const std::string &keyexpr, char *serialized_data_ptr,
                         uint64_t serialized_data_len, const z_loaned_query_t *query) {
  z_owned_bytes_t reply_payload;
  z_bytes_from_buf(&reply_payload, reinterpret_cast<uint8_t *>(serialized_data_ptr),
                   serialized_data_len,
                   NULL,
                   NULL);

  z_view_keyexpr_t reply_keyexpr_;
  z_view_keyexpr_from_str(&reply_keyexpr_, keyexpr.data());

  z_query_reply(query, z_loan(reply_keyexpr_), z_move(reply_payload), &z_reply_options_);
}

}  // namespace aimrt::plugins::zenoh_plugin