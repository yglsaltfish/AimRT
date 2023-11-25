#include "gtest/gtest.h"

#include "aimrt_module_cpp_interface/co/async_wrapper.h"
#include "aimrt_module_cpp_interface/co/sync_wait.h"
#include "aimrt_module_cpp_interface/co/task.h"

namespace aimrt::co {

void AsyncSendRecv(uint32_t in, aimrt::util::Function<void(uint32_t)>&& callback) {
  std::thread t([in, callback{std::move(callback)}]() mutable {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    callback(in + 1);
  });
  t.detach();
}

void AsyncSendRecv2(
    uint32_t in,
    aimrt::util::Function<void(uint32_t, std::string&&, const std::string&)>&& callback) {
  std::thread t([in, callback{std::move(callback)}]() mutable {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::string str = "abc";
    static const std::string const_str = "xyz";
    callback(in + 1, std::move(str), const_str);
  });
  t.detach();
}

void AsyncSendRecv3(uint32_t in, aimrt::util::Function<void()>&& callback) {
  std::thread t([in, callback{std::move(callback)}]() mutable {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    callback();
  });
  t.detach();
}

TEST(AsyncWrapper, SingleRet) {
  auto work = []() -> Task<int> {
    int ret1 = co_await AsyncWrapper<uint32_t>(
        [](aimrt::util::Function<void(uint32_t)>&& callback) {
          AsyncSendRecv(100, std::move(callback));
        });

    int ret2 = co_await AsyncWrapper<uint32_t>(
        std::bind(AsyncSendRecv, 1000, std::placeholders::_1));
    co_return ret1 + ret2;
  };

  auto [ret] = SyncWait(work()).value();
  EXPECT_EQ(ret, 1102);
}

TEST(AsyncWrapper, MultipleRet) {
  auto work = []() -> Task<bool> {
    auto [code, str, str_ref] = co_await AsyncWrapper<uint32_t, std::string, const std::string&>(
        std::bind(AsyncSendRecv2, 100, std::placeholders::_1));
    co_return (code == 101 && str == "abc" && str_ref == "xyz");
  };

  auto [ret] = SyncWait(work()).value();
  EXPECT_TRUE(ret);
}

TEST(AsyncWrapper, VoidRet) {
  auto work = []() -> Task<bool> {
    co_await AsyncWrapper(std::bind(AsyncSendRecv3, 100, std::placeholders::_1));
    co_return true;
  };

  auto [ret] = SyncWait(work()).value();
  EXPECT_TRUE(ret);
}

}  // namespace aimrt::co
