#include "parameter_module/parameter_module.h"
#include "aimrt_module_cpp_interface/co/aimrt_context.h"
#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/schedule.h"
#include "aimrt_module_cpp_interface/co/sync_wait.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::example_parameter::parameter_module {

template <typename ArrayType>
std::string ToString(const ArrayType& val) {
  std::stringstream ss;
  for (size_t ii = 0; ii < val.size(); ++ii) {
    if constexpr (sizeof(val[ii]) == 1) {
      ss << static_cast<int>(val[ii]) << ";";
    } else {
      ss << val[ii] << ";";
    }
  }
  return ss.str();
}

bool ParameterModule::Initialize(aimrt::CoreRef core) noexcept {
  // Save aimrt framework handle
  core_ = core;

  try {
    // Get executor handle
    work_executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");
    AIMRT_CHECK_ERROR_THROW(
        work_executor_ && work_executor_.SupportTimerSchedule(),
        "Get executor 'work_thread_pool' failed.");

    parameter_handle_ = core_.GetParameterHandle();
    AIMRT_CHECK_ERROR_THROW(parameter_handle_, "Get parameter failed.");

  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool ParameterModule::Start() noexcept {
  try {
    // Start main loop
    scope_.spawn(co::On(co::AimRTScheduler(work_executor_), SetParameterLoop()));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    scope_.spawn(co::On(co::AimRTScheduler(work_executor_), GetParameterLoop()));
  } catch (const std::exception& e) {
    AIMRT_ERROR("Start failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Start succeeded.");
  return true;
}

void ParameterModule::Shutdown() noexcept {
  try {
    // Wait all coroutine complete
    run_flag_ = false;
    co::SyncWait(scope_.complete());
  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
    return;
  }

  AIMRT_INFO("Shutdown succeeded.");
}

co::Task<void> ParameterModule::SetParameterLoop() {
  try {
    AIMRT_INFO("Start SetParameterLoop.");

    co::AimRTScheduler work_scheduler(work_executor_);

    constexpr uint32_t max_array_len = 16;

    uint32_t count = 0;
    while (run_flag_) {
      count++;
      AIMRT_INFO("SetParameterLoop count : {} -------------------------", count);

      uint32_t array_len = std::min(count, max_array_len);

      std::vector<unsigned char> byte_array(array_len);
      std::vector<bool> bool_array(array_len);

      std::vector<int8_t> int8_array(array_len);
      std::vector<int16_t> int16_array(array_len);
      std::vector<int32_t> int32_array(array_len);
      std::vector<int64_t> int64_array(array_len);

      std::vector<uint8_t> uint8_array(array_len);
      std::vector<uint16_t> uint16_array(array_len);
      std::vector<uint32_t> uint32_array(array_len);
      std::vector<uint64_t> uint64_array(array_len);

      std::vector<float> float_array(array_len);
      std::vector<double> double_array(array_len);

      std::vector<std::string> string_array(array_len);

      for (size_t ii = 0; ii < array_len; ++ii) {
        uint32_t cur_idx = count - ii;

        byte_array[ii] = static_cast<unsigned char>(cur_idx);
        bool_array[ii] = (cur_idx % 2);

        int8_array[ii] = -static_cast<int8_t>(cur_idx);
        int16_array[ii] = -static_cast<int16_t>(cur_idx);
        int32_array[ii] = -static_cast<int32_t>(cur_idx);
        int64_array[ii] = -static_cast<int64_t>(cur_idx);

        uint8_array[ii] = static_cast<uint8_t>(cur_idx);
        uint16_array[ii] = static_cast<uint16_t>(cur_idx);
        uint32_array[ii] = static_cast<uint32_t>(cur_idx);
        uint64_array[ii] = static_cast<uint64_t>(cur_idx);

        float_array[ii] = static_cast<float>(cur_idx) * 3.14;
        double_array[ii] = static_cast<double>(cur_idx) * 3.14;

        string_array[ii] = "str_" + std::to_string(cur_idx);
      }

      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_bool", static_cast<bool>(count % 2)),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_int", -static_cast<int64_t>(count)),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_uint", static_cast<uint64_t>(count)),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_double", static_cast<double>(count) * 3.14),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_string", "count: " + std::to_string(count)),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_byte_array", parameter::ParameterView(byte_array.data(), byte_array.size())),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_bool_array", bool_array),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_int8_array", int8_array),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_int16_array", int16_array),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_int32_array", int32_array),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_int64_array", int64_array),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_uint8_array", uint8_array),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_uint16_array", uint16_array),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_uint32_array", uint32_array),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_uint64_array", uint64_array),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_float_array", float_array),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_double_array", double_array),
          "Set parameter failed");
      AIMRT_CHECK_ERROR_THROW(
          parameter_handle_.SetParameter("test_string_array", string_array),
          "Set parameter failed");

      AIMRT_INFO("SetParameter done");

      co_await co::ScheduleAfter(
          work_scheduler, std::chrono::milliseconds(500));
    }

    AIMRT_INFO("Exit SetParameterLoop.");
  } catch (const std::exception& e) {
    AIMRT_ERROR("Exit SetParameterLoop with exception, {}", e.what());
  }

  co_return;
}

co::Task<void> ParameterModule::GetParameterLoop() {
  try {
    AIMRT_INFO("Start GetParameterLoop.");

    co::AimRTScheduler work_scheduler(work_executor_);

    uint32_t count = 0;
    while (run_flag_) {
      count++;
      AIMRT_INFO("GetParameterLoop count : {} -------------------------", count);

      try {
        auto bool_val = parameter_handle_.GetParameter("test_bool").As<bool>();
        auto int64_val = parameter_handle_.GetParameter("test_int").As<int64_t>();
        auto uint64_val = parameter_handle_.GetParameter("test_uint").As<uint64_t>();
        auto double_val = parameter_handle_.GetParameter("test_double").As<double>();
        auto string_view_val = parameter_handle_.GetParameter("test_string").As<std::string_view>();

        auto test_byte_array_parameter = parameter_handle_.GetParameter("test_byte_array");
        auto char_span_val = test_byte_array_parameter.As<std::span<const signed char>>();
        auto char_vec_val = test_byte_array_parameter.As<std::vector<signed char>>();

        auto test_bool_array_parameter = parameter_handle_.GetParameter("test_bool_array");
        auto bool_span_val = test_bool_array_parameter.As<std::span<const bool>>();
        auto bool_vec_val = test_bool_array_parameter.As<std::vector<bool>>();

        auto test_int8_array_parameter = parameter_handle_.GetParameter("test_int8_array");
        auto int8_span_val = test_int8_array_parameter.As<std::span<const int8_t>>();
        auto int8_vec_val = test_int8_array_parameter.As<std::vector<int8_t>>();

        auto test_int16_array_parameter = parameter_handle_.GetParameter("test_int16_array");
        auto int16_vec_val = test_int16_array_parameter.As<std::vector<int16_t>>();

        auto test_int32_array_parameter = parameter_handle_.GetParameter("test_int32_array");
        auto int32_vec_val = test_int32_array_parameter.As<std::vector<int32_t>>();

        auto test_int64_array_parameter = parameter_handle_.GetParameter("test_int64_array");
        auto int64_span_val = test_int64_array_parameter.As<std::span<const int64_t>>();
        auto int64_vec_val = test_int64_array_parameter.As<std::vector<int64_t>>();

        auto test_uint8_array_parameter = parameter_handle_.GetParameter("test_uint8_array");
        auto uint8_span_val = test_uint8_array_parameter.As<std::span<const uint8_t>>();
        auto uint8_vec_val = test_uint8_array_parameter.As<std::vector<uint8_t>>();

        auto test_uint16_array_parameter = parameter_handle_.GetParameter("test_uint16_array");
        auto uint16_vec_val = test_uint16_array_parameter.As<std::vector<uint16_t>>();

        auto test_uint32_array_parameter = parameter_handle_.GetParameter("test_uint32_array");
        auto uint32_vec_val = test_uint32_array_parameter.As<std::vector<uint32_t>>();

        auto test_uint64_array_parameter = parameter_handle_.GetParameter("test_uint64_array");
        auto uint64_span_val = test_uint64_array_parameter.As<std::span<const uint64_t>>();
        auto uint64_vec_val = test_uint64_array_parameter.As<std::vector<uint64_t>>();

        auto test_float_array_parameter = parameter_handle_.GetParameter("test_float_array");
        auto float_vec_val = test_float_array_parameter.As<std::vector<float>>();

        auto test_double_array_parameter = parameter_handle_.GetParameter("test_double_array");
        auto double_span_val = test_double_array_parameter.As<std::span<const double>>();
        auto double_vec_val = test_double_array_parameter.As<std::vector<double>>();

        auto test_string_array_parameter = parameter_handle_.GetParameter("test_string_array");
        auto string_view_vec_val = test_string_array_parameter.As<std::vector<std::string_view>>();
        auto string_vec_val = test_string_array_parameter.As<std::vector<std::string>>();

        AIMRT_INFO(
            "bool_val: '{}', int64_val: '{}', uint64_val: '{}', double_val: '{}', string_view_val: '{}'\n"
            "char_span_val: '{}', char_vec_val: '{}'\n"
            "bool_span_val: '{}', bool_vec_val: '{}'\n"
            "int8_span_val: '{}', int8_vec_val: '{}'\n"
            "int16_vec_val: '{}'\n"
            "int32_vec_val: '{}'\n"
            "int64_span_val: '{}', int64_vec_val: '{}'\n"
            "uint8_span_val: '{}', uint8_vec_val: '{}'\n"
            "uint16_vec_val: '{}'\n"
            "uint32_vec_val: '{}'\n"
            "uint64_span_val: '{}', uint64_vec_val: '{}'\n"
            "float_vec_val: '{}'\n"
            "double_span_val: '{}', double_vec_val: '{}'\n"
            "string_view_vec_val: '{}', string_vec_val: '{}'",
            bool_val, int64_val, uint64_val, double_val, string_view_val,
            ToString(char_span_val), ToString(char_vec_val),
            ToString(bool_span_val), ToString(bool_vec_val),
            ToString(int8_span_val), ToString(int8_vec_val),
            ToString(int16_vec_val),
            ToString(int32_vec_val),
            ToString(int64_span_val), ToString(int64_vec_val),
            ToString(uint8_span_val), ToString(uint8_vec_val),
            ToString(uint16_vec_val),
            ToString(uint32_vec_val),
            ToString(uint64_span_val), ToString(uint64_vec_val),
            ToString(float_vec_val),
            ToString(double_span_val), ToString(double_vec_val),
            ToString(string_view_vec_val), ToString(string_vec_val));

      } catch (const std::exception& e) {
        AIMRT_ERROR("Get parameter failed with exception, {}", e.what());
      }

      co_await co::ScheduleAfter(
          work_scheduler,
          std::chrono::milliseconds(500));
    }

    AIMRT_INFO("Exit GetParameterLoop.");
  } catch (const std::exception& e) {
    AIMRT_ERROR("Exit GetParameterLoop with exception, {}", e.what());
  }

  co_return;
}

}  // namespace aimrt::examples::example_parameter::parameter_module
