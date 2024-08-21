// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "event.pb.h"
#include "zenoh.h"

volatile sig_atomic_t should_exit = 0;
void signal_handler(int signum) {
  should_exit = 1;
}

int main(int argc, char **argv) {
  signal(SIGINT, signal_handler);

  // initial msg
  aimrt::protocols::example::ExampleEventMsg msg;
  msg.set_msg("Hello AimRT");
  msg.set_num(2024);
  size_t serialized_size = msg.ByteSizeLong();
  char *serialized_data = new char[serialized_size];
  msg.SerializeToArray(serialized_data, serialized_size);

  // initial configuration
  const char *keyexpr = "aimrt/example/plugin/zenoh_plugin/assistant/native_zenoh_protobuf_channel_publisher";

  z_owned_config_t config;
  z_config_default(&config);

  // open session
  printf("Opening session, please wait...\n\n");
  z_owned_session_t s;
  if (z_open(&s, z_move(config)) < 0) {
    printf("Unable to open session!\n");
    exit(-1);
  }

  // bind a pub to a session
  printf("Declaring Publisher on '%s'...\n", keyexpr);
  z_owned_publisher_t pub;
  z_view_keyexpr_t key;
  z_view_keyexpr_from_str(&key, keyexpr);
  if (z_declare_publisher(&pub, z_loan(s), z_loan(key), NULL) < 0) {
    printf("Unable to declare Publisher for key expression!\n");
    exit(-1);
  }

  // send data
  while (!should_exit) {
    z_publisher_put_options_t options;
    z_publisher_put_options_default(&options);

    time_t now = time(nullptr);
    char timestamp[26];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    printf(">> [%s] Publishing message: %s in %d\n", timestamp, msg.msg().c_str(), msg.num());
    z_owned_bytes_t payload;
    z_bytes_serialize_from_str(&payload, serialized_data);
    z_publisher_put(z_loan(pub), z_move(payload), &options);
    z_sleep_s(1);
  }

  // clear resources
  z_undeclare_publisher(z_move(pub));
  z_close(z_move(s));
  printf("This program has been exied successfully.\n");

  delete[] serialized_data;
  serialized_data = nullptr;

  return 0;
}
