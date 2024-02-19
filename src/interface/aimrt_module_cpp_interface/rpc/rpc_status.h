#pragma once

#include <cinttypes>
#include <string>

namespace aimrt::rpc {

class Status {
 public:
  enum class RetCode : uint32_t {
    OK = 0,
    UNKNOWN,
    TIMEOUT,

    // svr side
    SVR_UNKNOWN = 1000,
    SVR_NOT_IMPLEMENTED,
    SVR_NOT_FOUND,
    SVR_INVALID_SERIALIZATION_TYPE,
    SVR_SERIALIZATION_FAILDE,
    SVR_INVALID_DESERIALIZATION_TYPE,
    SVR_DESERIALIZATION_FAILDE,
    SVR_HANDLE_FAILDE,

    // cli side
    CLI_UNKNOWN = 2000,
    CLI_INVALID_ADDR,
    CLI_INVALID_SERIALIZATION_TYPE,
    CLI_SERIALIZATION_FAILDE,
    CLI_INVALID_DESERIALIZATION_TYPE,
    CLI_DESERIALIZATION_FAILDE,
    CLI_NO_BACKEND_TO_HANDLE,
    CLI_SEND_REQ_FAILED,
  };

 public:
  Status() = default;
  ~Status() = default;

  explicit Status(Status::RetCode code) : code_(static_cast<uint32_t>(code)) {}
  explicit Status(uint32_t code) : code_(code) {}

  bool OK() const {
    return code_ == static_cast<uint32_t>(Status::RetCode::OK);
  }

  operator bool() const { return OK(); }

  uint32_t Code() const { return code_; }

  std::string ToString() const {
    return std::string(OK() ? "suc" : "fail") + ", code " + std::to_string(code_);
  }

 private:
  uint32_t code_ = static_cast<uint32_t>(Status::RetCode::OK);
};

}  // namespace aimrt::rpc
