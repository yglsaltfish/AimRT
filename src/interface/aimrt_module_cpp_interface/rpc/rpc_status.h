#pragma once

#include <cinttypes>
#include <string>

#include "aimrt_module_c_interface/rpc/rpc_status_base.h"

namespace aimrt::rpc {

class Status {
 public:
  Status() = default;
  ~Status() = default;

  explicit Status(aimrt_rpc_status_code_t code) : code_(code) {}
  explicit Status(uint32_t code) : code_(code) {}

  bool OK() const {
    return code_ == aimrt_rpc_status_code_t::AIMRT_RPC_STATUS_OK;
  }

  operator bool() const { return OK(); }

  uint32_t Code() const { return code_; }

  std::string ToString() const {
    return std::string(OK() ? "suc" : "fail") + ", code " + std::to_string(code_);
  }

 private:
  uint32_t code_ = aimrt_rpc_status_code_t::AIMRT_RPC_STATUS_OK;
};

}  // namespace aimrt::rpc
