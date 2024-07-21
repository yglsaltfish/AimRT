#include <gtest/gtest.h>

#include <future>

#include "aimrt_module_cpp_interface/rpc/rpc_async_filter.h"

namespace aimrt::rpc {

TEST(RPC_FILTER_TEST, AsyncFilterManager_base) {
  AsyncFilterManager m;

  auto rpc_handle = [](ContextRef ctx, const void *req, void *rsp, std::function<void(Status)> &&callback) {
    *static_cast<std::string *>(rsp) = *static_cast<const std::string *>(req);
    callback(Status());
  };

  std::string req = "test req";
  std::string rsp;
  Context ctx;

  std::promise<Status> status_promise;

  m.InvokeRpc(rpc_handle, ctx, &req, &rsp,
              [&status_promise](Status status) mutable {
                status_promise.set_value(status);
              });

  auto status = status_promise.get_future().get();

  EXPECT_TRUE(status);

  EXPECT_EQ(rsp, req);
}

TEST(RPC_FILTER_TEST, AsyncFilterManager_multiple_filters) {
  AsyncFilterManager m;

  // 先注册的在内层
  m.RegisterFilter(
      [](ContextRef ctx,
         const void *req,
         void *rsp,
         std::function<void(Status)> &&callback,
         const AsyncRpcHandle &h) {
        ctx.SetMetaValue("order", std::string(ctx.GetMetaValue("order")) + " -> f1 begin");

        h(ctx, req, rsp, [ctx, callback{std::move(callback)}](Status status) mutable {
          ctx.SetMetaValue("order", std::string(ctx.GetMetaValue("order")) + " -> f1 end");

          callback(status);
        });
      });

  // 后注册的在外层
  m.RegisterFilter(
      [](ContextRef ctx,
         const void *req,
         void *rsp,
         std::function<void(Status)> &&callback,
         const AsyncRpcHandle &h) {
        ctx.SetMetaValue("order", std::string(ctx.GetMetaValue("order")) + " -> f2 begin");

        h(ctx, req, rsp, [ctx, callback{std::move(callback)}](Status status) mutable {
          ctx.SetMetaValue("order", std::string(ctx.GetMetaValue("order")) + " -> f2 end");

          callback(status);
        });
      });

  // rpc handle在最内层
  auto rpc_handle = [](ContextRef ctx, const void *req, void *rsp, std::function<void(Status)> &&callback) {
    ctx.SetMetaValue("order", std::string(ctx.GetMetaValue("order")) + " -> rpc handle");
    callback(Status());
  };

  std::string req;
  std::string rsp;
  Context ctx;

  std::promise<Status> status_promise;

  ctx.SetMetaValue("order", "begin");

  m.InvokeRpc(rpc_handle, ctx, &req, &rsp,
              [&ctx, &status_promise](Status status) mutable {
                ctx.SetMetaValue("order", std::string(ctx.GetMetaValue("order")) + " -> end");

                status_promise.set_value(status);
              });

  auto status = status_promise.get_future().get();
  EXPECT_TRUE(status);

  EXPECT_STREQ(
      ctx.GetMetaValue("order").data(),
      "begin -> f2 begin -> f1 begin -> rpc handle -> f1 end -> f2 end -> end");
}

}  // namespace aimrt::rpc