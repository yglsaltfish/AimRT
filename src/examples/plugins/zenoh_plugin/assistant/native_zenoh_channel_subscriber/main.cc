#include <signal.h>
#include <stdio.h>
#include <ctime>
#include "zenoh.h"

volatile sig_atomic_t should_exit = 0;
void signal_handler(int signum) {
  should_exit = 1;
}

// callback
void data_handler(const z_loaned_sample_t *sample, void *arg) {
  z_view_string_t key_string;
  z_keyexpr_as_view_string(z_sample_keyexpr(sample), &key_string);

  z_owned_string_t payload_string;
  z_bytes_deserialize_into_string(z_sample_payload(sample), &payload_string);

  time_t now = time(nullptr);
  char timestamp[26];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
  printf(">> [%s] Subscribed message: '%.*s'\n", timestamp, (int)z_string_len(z_loan(payload_string)), z_string_data(z_loan(payload_string)));
  z_drop(z_move(payload_string));
}

int main(int argc, char **argv) {
  signal(SIGINT, signal_handler);

  // initial configuration
  const char *keyexpr = "aimrt/example/plugin/zenoh_plugin/assistant/native_zenoh_channel_publisher";
  z_view_keyexpr_t key;
  z_view_keyexpr_from_str(&key, keyexpr);

  z_owned_config_t config;
  z_config_default(&config);

  // open session
  printf("Opening session, please wait...\n\n");
  z_owned_session_t s;
  if (z_open(&s, z_move(config)) < 0) {
    printf("Unable to open session!\n");
    exit(-1);
  }

  // bind a sub to a session and register a callback,this is s asynchronous operation
  z_owned_closure_sample_t callback;
  z_closure(&callback, data_handler, NULL, NULL);
  printf("Declaring Subscriber on '%s'...\n", keyexpr);
  z_owned_subscriber_t sub;
  if (z_declare_subscriber(&sub, z_loan(s), z_loan(key), z_move(callback), NULL) < 0) {
    printf("Unable to declare subscriber.\n");
    exit(-1);
  }

  // main loop
  printf("Enter 'Ctrl + C' to quit...\n");
  while (!should_exit) {
    z_sleep_s(1);
  }

  // clear resourse
  z_undeclare_subscriber(z_move(sub));
  z_close(z_move(s));
  printf("This program has been exied successfully.\n");
  return 0;
}
