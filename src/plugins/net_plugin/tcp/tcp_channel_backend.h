#pragma once

#include <set>

#include "core/channel/channel_backend_base.h"
#include "net/asio_tcp_cli.h"
#include "net/asio_tcp_svr.h"
#include "net_plugin/msg_handle_registry.h"

namespace aimrt::plugins::net_plugin {

class TcpChannelBackend : public runtime::core::channel::ChannelBackendBase {
 public:
  using TcpMsgHandleRegistry = MsgHandleRegistry<boost::asio::ip::tcp::endpoint>;

  struct Options {
    struct PubTopicOptions {
      std::string topic_name;
      std::vector<std::string> server_url_list;
    };

    std::vector<PubTopicOptions> pub_topics_options;
  };

 public:
  TcpChannelBackend(
      const std::shared_ptr<boost::asio::io_context>& io_ptr,
      const std::shared_ptr<common::net::AsioTcpClientPool>& tcp_cli_pool_ptr,
      const std::shared_ptr<common::net::AsioTcpServer>& tcp_svr_ptr,
      const std::shared_ptr<TcpMsgHandleRegistry>& msg_handle_registry_ptr)
      : io_ptr_(io_ptr),
        tcp_cli_pool_ptr_(tcp_cli_pool_ptr),
        tcp_svr_ptr_(tcp_svr_ptr),
        msg_handle_registry_ptr_(msg_handle_registry_ptr) {}

  ~TcpChannelBackend() override = default;

  std::string_view Name() const override { return "tcp"; }

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
  enum class Status : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<Status> status_ = Status::PreInit;

  const runtime::core::channel::ChannelRegistry* channel_registry_ptr_ = nullptr;

  std::shared_ptr<boost::asio::io_context> io_ptr_;
  std::shared_ptr<common::net::AsioTcpClientPool> tcp_cli_pool_ptr_;
  std::shared_ptr<common::net::AsioTcpServer> tcp_svr_ptr_;
  std::shared_ptr<TcpMsgHandleRegistry> msg_handle_registry_ptr_;

  std::map<std::string,
           std::unique_ptr<std::vector<const runtime::core::channel::SubscribeWrapper*>>>
      tcp_subscribe_wrapper_map_;
};

}  // namespace aimrt::plugins::net_plugin