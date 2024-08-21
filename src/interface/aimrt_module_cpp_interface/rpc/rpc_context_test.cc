// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#include <gtest/gtest.h>

#include "aimrt_module_cpp_interface/rpc/rpc_context.h"
#include "util/stl_tool.h"

namespace aimrt::rpc {

TEST(RPC_CONTEXT_TEST, Context) {
  Context ctx;

  ctx.SetTimeout(std::chrono::milliseconds(100));
  EXPECT_EQ(ctx.Timeout(), std::chrono::milliseconds(100));

  EXPECT_EQ(ctx.GetMetaKeys().size(), 0);

  ctx.SetMetaValue("key1", "val1");
  ctx.SetMetaValue("key2", "val2");

  EXPECT_EQ(ctx.GetMetaValue("key1"), "val1");
  EXPECT_EQ(ctx.GetMetaValue("key2"), "val2");
  EXPECT_EQ(ctx.GetMetaValue("key3"), "");

  EXPECT_TRUE(aimrt::common::util::CheckContainerEqualNoOrder(
      ctx.GetMetaKeys(),
      std::vector<std::string_view>{"key1", "key2"}));
}

TEST(RPC_CONTEXT_TEST, ContextRef) {
  Context real_ctx;
  ContextRef ctx(real_ctx);

  ctx.SetTimeout(std::chrono::milliseconds(100));
  EXPECT_EQ(ctx.Timeout(), std::chrono::milliseconds(100));

  EXPECT_EQ(ctx.GetMetaKeys().size(), 0);

  ctx.SetMetaValue("key1", "val1");
  ctx.SetMetaValue("key2", "val2");

  EXPECT_EQ(ctx.GetMetaValue("key1"), "val1");
  EXPECT_EQ(ctx.GetMetaValue("key2"), "val2");
  EXPECT_EQ(ctx.GetMetaValue("key3"), "");

  EXPECT_TRUE(aimrt::common::util::CheckContainerEqualNoOrder(
      ctx.GetMetaKeys(),
      std::vector<std::string_view>{"key1", "key2"}));
}

}  // namespace aimrt::rpc
