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

  EXPECT_STREQ(
      ctx.ToString().c_str(),
      "Client context, timeout: 100ms, meta: {{\"key2\":\"val2\"},{\"key1\":\"val1\"}}");
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

  EXPECT_STREQ(
      ctx.ToString().c_str(),
      "Client context, timeout: 100ms, meta: {{\"key2\":\"val2\"},{\"key1\":\"val1\"}}");
}

TEST(RPC_CONTEXT_TEST, MergeContextMeta) {
  Context ctx;
  ctx.SetMetaValue("test_prefix::key_1", "val_1");
  ctx.SetMetaValue("test_prefix::key_2", "val_2");
  ctx.SetMetaValue("test_prefix::key_3", "val_3");

  ctx.SetMetaValue("test_prefix_2::key_1", "val_1");
  ctx.SetMetaValue("test_prefix_2::key_2", "val_2");
  ctx.SetMetaValue("test_prefix_2::key_3", "val_3");

  ctx.SetMetaValue("key_1", "val_1");
  ctx.SetMetaValue("key_2", "val_2");
  ctx.SetMetaValue("key_3", "val_3");

  Context c1;
  MergeContextMeta(c1, ctx);
  EXPECT_TRUE(aimrt::common::util::CheckContainerEqualNoOrder(
      c1.GetMetaKeys(),
      std::vector<std::string_view>{
          "test_prefix::key_1",
          "test_prefix::key_2",
          "test_prefix::key_3",
          "test_prefix_2::key_1",
          "test_prefix_2::key_2",
          "test_prefix_2::key_3",
          "key_1",
          "key_2",
          "key_3"}));

  Context c2;
  MergeContextMeta(c2, ctx, {"test_prefix::"});
  EXPECT_TRUE(aimrt::common::util::CheckContainerEqualNoOrder(
      c2.GetMetaKeys(),
      std::vector<std::string_view>{
          "test_prefix_2::key_1",
          "test_prefix_2::key_2",
          "test_prefix_2::key_3",
          "key_1",
          "key_2",
          "key_3"}));

  Context c3;
  MergeContextMeta(c3, ctx, {"test_prefix::", "test_prefix_2::"});
  EXPECT_TRUE(aimrt::common::util::CheckContainerEqualNoOrder(
      c3.GetMetaKeys(),
      std::vector<std::string_view>{
          "key_1",
          "key_2",
          "key_3"}));
}

}  // namespace aimrt::rpc
