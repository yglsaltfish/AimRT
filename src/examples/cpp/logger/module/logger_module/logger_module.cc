// Copyright (c) 2023, AgiBot Inc.
// All rights reserved

#include "logger_module/logger_module.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::cpp::logger::logger_module {

bool LoggerModule::Initialize(aimrt::CoreRef core) {
  // Save aimrt framework handle
  core_ = core;

  AIMRT_HL_INFO(core_.GetLogger(), "Init succeeded.");

  return true;
}

bool LoggerModule::Start() {
  AIMRT_HL_INFO(core_.GetLogger(), "Start succeeded.");

  auto GetLogger = [this]() { return core_.GetLogger(); };

  std::string s = "abc";
  int n = 123;

  AIMRT_TRACE("Test trace log, s = {}, n = {}.", s, n);
  AIMRT_DEBUG("Test debug log, s = {}, n = {}.", s, n);
  AIMRT_INFO("Test info log, s = {}, n = {}.", s, n);
  AIMRT_WARN("Test warn log, s = {}, n = {}.", s, n);
  AIMRT_ERROR("Test error log, s = {}, n = {}.", s, n);
  AIMRT_FATAL("Test fatal log, s = {}, n = {}.", s, n);

  return true;
}

void LoggerModule::Shutdown() {
  AIMRT_HL_INFO(core_.GetLogger(), "Shutdown succeeded.");
}

}  // namespace aimrt::examples::cpp::logger::logger_module
