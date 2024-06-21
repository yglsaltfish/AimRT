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

  EXPECT_TRUE(aimrt::common::util::CheckContainerEqual(
      ctx.GetMetaKeys(),
      std::set<std::string_view>{"key1", "key2"}));

  EXPECT_STREQ(
      ctx.ToString().c_str(),
      "timeout: 100ms, meta: {{key2,val2},{key1,val1}}");
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

  EXPECT_TRUE(aimrt::common::util::CheckContainerEqual(
      ctx.GetMetaKeys(),
      std::set<std::string_view>{"key1", "key2"}));

  EXPECT_STREQ(
      ctx.ToString().c_str(),
      "timeout: 100ms, meta: {{key2,val2},{key1,val1}}");
}

}  // namespace aimrt::rpc
