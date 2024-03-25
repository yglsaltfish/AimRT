#pragma once

#include <set>

#include "core/channel/channel_backend_base.h"
#include "net_plugin/util/asio_http_cli.h"
#include "net_plugin/util/asio_http_svr.h"

namespace aimrt::plugins::net_plugin {

class HttpChannelBackend : public runtime::core::channel::ChannelBackendBase {
 public:
  struct Options {
    struct PubTopicOptions {
      std::string topic_name;
      std::vector<std::string> server_url_list;
    };

    std::vector<PubTopicOptions> pub_topics_options;
  };

 public:
  HttpChannelBackend(
      const std::shared_ptr<boost::asio::io_context>& io_ptr,
      const std::shared_ptr<AsioHttpClientPool>& http_cli_pool_ptr,
      const std::shared_ptr<AsioHttpServer>& http_svr_ptr)
      : io_ptr_(io_ptr),
        http_cli_pool_ptr_(http_cli_pool_ptr),
        http_svr_ptr_(http_svr_ptr) {}

  ~HttpChannelBackend() override = default;

  std::string_view Name() const override { return "http"; }

  void Initialize(YAML::Node options_node,
                  const runtime::core::channel::ChannelRegistry* channel_registry_ptr,
                  runtime::core::channel::ContextManager* context_manager_ptr) override;
  void Start() override;
  void Shutdown() override;

  bool RegisterPublishType(
      const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept override;
  bool Subscribe(const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept override;
  void Publish(const runtime::core::channel::PublishWrapper& publish_wrapper) noexcept override;

 private:
  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<State> state_ = State::PreInit;

  const runtime::core::channel::ChannelRegistry* channel_registry_ptr_ = nullptr;
  runtime::core::channel::ContextManager* context_manager_ptr_ = nullptr;

  std::shared_ptr<boost::asio::io_context> io_ptr_;
  std::shared_ptr<AsioHttpClientPool> http_cli_pool_ptr_;
  std::shared_ptr<AsioHttpServer> http_svr_ptr_;

  std::unordered_map<
      std::string,
      std::unique_ptr<std::vector<const runtime::core::channel::SubscribeWrapper*>>>
      http_subscribe_wrapper_map_;
};

}  // namespace aimrt::plugins::net_plugin