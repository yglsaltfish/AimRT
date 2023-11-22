#include "gtest/gtest.h"

#ifdef AIMRT_USE_EXECUTOR

  #include <thread>

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

  auto ret = SyncWait(work());
  EXPECT_EQ(*ret, 1102);
}

void AsyncSendRecv2(
    uint32_t in,
    aimrt::util::Function<void(uint32_t, std::string&&, const std::string&)>&& callback) {
  std::thread t([in, callback{std::move(callback)}]() mutable {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    static const std::string const_str = "xyz";
    std::string str = "abc";
    callback(in + 1, std::move(str), const_str);
  });
  t.detach();
}

TEST(AsyncWrapper, MultipleRet) {
  auto work = []() -> Task<bool> {
    auto [code, str, str_ref] = co_await AsyncWrapper<int, std::string, const std::string&>(
        std::bind(AsyncSendRecv2, 100, std::placeholders::_1));
    co_return (code == 101 && str == "abc" && str_ref == "xyz");
  };

  auto ret = SyncWait(work());
  EXPECT_TRUE(*ret);
}

}  // namespace aimrt::co

#endif
