#include "parameter_plugin/rpc_service.h"
#include "aimrt_module_cpp_interface/parameter/parameter_handle.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"
#include "parameter_plugin/global.h"

namespace aimrt::plugins::parameter_plugin {

void ParameterServiceImpl::SetPbParameter(
    const std::shared_ptr<aimrt::runtime::core::parameter::Parameter>& aimrt_parameter,
    ::aimrt::protocols::parameter_plugin::ParameterValue* pb_parameter) {
  aimrt::parameter::ParameterView parameter_view(aimrt_parameter->GetView());

  switch (parameter_view.Type()) {
    case aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_NULL:
      break;
    case aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL:
      pb_parameter->set_bool_value(parameter_view.As<bool>());
      break;
    case aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER:
      pb_parameter->set_int_value(parameter_view.As<int64_t>());
      break;
    case aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER:
      pb_parameter->set_uint_value(parameter_view.As<uint64_t>());
      break;
    case aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE:
      pb_parameter->set_double_value(parameter_view.As<double>());
      break;
    case aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING:
      pb_parameter->set_allocated_string_value(
          new std::string(parameter_view.As<std::string_view>()));
      break;
    case aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BYTE_ARRAY: {
      auto buf = parameter_view.As<std::span<const int8_t>>();
      pb_parameter->set_allocated_bytes_value(
          new std::string(reinterpret_cast<const char*>(buf.data()), buf.size()));
      break;
    }
    case aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_BOOL_ARRAY: {
      auto buf = parameter_view.As<std::span<const bool>>();
      auto* bool_array = pb_parameter->mutable_bool_array();
      for (auto item : buf) {
        bool_array->add_items(item);
      }
      break;
    }
    case aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_INTEGER_ARRAY: {
      const auto& array = parameter_view.As<std::span<const int64_t>>();
      auto* pb_array = pb_parameter->mutable_int_array();
      for (auto item : array) pb_array->add_items(item);
      break;
    }
    case aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY: {
      const auto& array = parameter_view.As<std::span<const uint64_t>>();
      auto* pb_array = pb_parameter->mutable_uint_array();
      for (auto item : array) pb_array->add_items(item);
      break;
    }
    case aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_DOUBLE_ARRAY: {
      const auto& array = parameter_view.As<std::span<const double>>();
      auto* pb_array = pb_parameter->mutable_double_array();
      for (auto item : array) pb_array->add_items(item);
      break;
    }
    case aimrt_parameter_type_t::AIMRT_PARAMETER_TYPE_STRING_ARRAY: {
      auto array = parameter_view.As<std::span<const aimrt_string_view_t>>();
      auto pb_array = pb_parameter->mutable_string_array();
      for (auto itm : array) pb_array->add_items(itm.str, itm.len);
      break;
    }
    default:
      pb_parameter->set_allocated_null_value(
          new ::aimrt::protocols::parameter_plugin::NullValue{});
      break;
  }
}

std::shared_ptr<aimrt::runtime::core::parameter::Parameter> ParameterServiceImpl::GetPbParameter(
    const ::aimrt::protocols::parameter_plugin::ParameterValue& pb_parameter) {
  using namespace protocols::parameter_plugin;

  auto parameter_ptr = std::make_shared<aimrt::runtime::core::parameter::Parameter>();

  switch (pb_parameter.kind_case()) {
    case ParameterValue::kNullValue:
      break;
    case ParameterValue::kBoolValue:
      parameter_ptr->SetData(
          aimrt::parameter::ParameterView(pb_parameter.bool_value()).NativeHandle());
      break;
    case ParameterValue::kIntValue:
      parameter_ptr->SetData(
          aimrt::parameter::ParameterView(pb_parameter.int_value()).NativeHandle());
      break;
    case ParameterValue::kUintValue:
      parameter_ptr->SetData(
          aimrt::parameter::ParameterView(pb_parameter.uint_value()).NativeHandle());
      break;
    case ParameterValue::kDoubleValue:
      parameter_ptr->SetData(
          aimrt::parameter::ParameterView(pb_parameter.double_value()).NativeHandle());
      break;
    case ParameterValue::kStringValue:
      parameter_ptr->SetData(
          aimrt::parameter::ParameterView(pb_parameter.string_value()).NativeHandle());
      break;
    case ParameterValue::kBytesValue: {
      auto buf = pb_parameter.bytes_value();
      parameter_ptr->SetData(
          aimrt::parameter::ParameterView(buf.c_str(), buf.size()).NativeHandle());
      break;
    }
    case ParameterValue::kBoolArray: {
      auto buf = pb_parameter.bool_array().items();
      parameter_ptr->SetData(
          aimrt::parameter::ParameterView(std::span<const bool>(buf.data(), buf.size())).NativeHandle());
      break;
    }
    case ParameterValue::kIntArray: {
      auto buf = pb_parameter.int_array().items();
      parameter_ptr->SetData(
          aimrt::parameter::ParameterView(std::span<const int64_t>(buf.data(), buf.size())).NativeHandle());
      break;
    }
    case ParameterValue::kUintArray: {
      auto buf = pb_parameter.uint_array().items();
      parameter_ptr->SetData(
          aimrt::parameter::ParameterView(std::span<const uint64_t>(buf.data(), buf.size())).NativeHandle());
      break;
    }
    case ParameterValue::kDoubleArray: {
      auto buf = pb_parameter.double_array().items();
      parameter_ptr->SetData(
          aimrt::parameter::ParameterView(std::span<const double>(buf.data(), buf.size())).NativeHandle());
      break;
    }
    case ParameterValue::kStringArray: {
      auto buf = pb_parameter.string_array().items();
      std::vector<aimrt_string_view_t> str_array;
      str_array.reserve(buf.size());
      for (auto itr = buf.begin(); itr != buf.end(); ++itr) {
        str_array.emplace_back(util::ToAimRTStringView(*itr));
      }
      parameter_ptr->SetData(aimrt::parameter::ParameterView(str_array).NativeHandle());
      break;
    }
    default:
      break;
  }

  return parameter_ptr;
}

aimrt::co::Task<aimrt::rpc::Status> ParameterServiceImpl::Set(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::parameter_plugin::SetParameterReq& req,
    ::aimrt::protocols::parameter_plugin::SetParameterRsp& rsp) {
  assert(parameter_manager_ptr_);

  auto* parameter_handle_ptr = parameter_manager_ptr_->GetParameterHandle(req.module_name());
  if (parameter_handle_ptr == nullptr) {
    SetErrorCode(ErrorCode::INVALID_MODULE_NAME, rsp);
    co_return aimrt::rpc::Status();
  }

  if (!req.has_parameter_value()) [[unlikely]] {
    parameter_handle_ptr->SetParameter(
        req.parameter_name(),
        std::shared_ptr<aimrt::runtime::core::parameter::Parameter>());
    co_return aimrt::rpc::Status();
  }

  parameter_handle_ptr->SetParameter(
      req.parameter_name(), GetPbParameter(req.parameter_value()));

  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> ParameterServiceImpl::Get(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::parameter_plugin::GetParameterReq& req,
    ::aimrt::protocols::parameter_plugin::GetParameterRsp& rsp) {
  assert(parameter_manager_ptr_);

  auto* parameter_handle_ptr = parameter_manager_ptr_->GetParameterHandle(req.module_name());
  if (parameter_handle_ptr == nullptr) {
    SetErrorCode(ErrorCode::INVALID_MODULE_NAME, rsp);
    co_return aimrt::rpc::Status();
  }

  auto parameter_ptr = parameter_handle_ptr->GetParameter(req.parameter_name());
  if (!parameter_ptr) [[unlikely]] {
    rsp.mutable_parameter_value()->set_allocated_null_value(
        new ::aimrt::protocols::parameter_plugin::NullValue{});
    co_return aimrt::rpc::Status();
  }

  SetPbParameter(parameter_ptr, rsp.mutable_parameter_value());

  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> ParameterServiceImpl::List(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::parameter_plugin::ListParameterReq& req,
    ::aimrt::protocols::parameter_plugin::ListParameterRsp& rsp) {
  assert(parameter_manager_ptr_);

  auto* parameter_handle_ptr = parameter_manager_ptr_->GetParameterHandle(req.module_name());
  if (parameter_handle_ptr == nullptr) {
    SetErrorCode(ErrorCode::INVALID_MODULE_NAME, rsp);
    co_return aimrt::rpc::Status();
  }

  auto parameter_names = parameter_handle_ptr->ListParameter();
  rsp.mutable_parameter_names()->Assign(parameter_names.begin(), parameter_names.end());

  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> ParameterServiceImpl::Dump(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::parameter_plugin::DumpParameterReq& req,
    ::aimrt::protocols::parameter_plugin::DumpParameterRsp& rsp) {
  assert(parameter_manager_ptr_);

  for (auto itr = req.module_names().begin(); itr != req.module_names().end(); ++itr) {
    std::string_view module_name(*itr);
    auto* parameter_handle_ptr = parameter_manager_ptr_->GetParameterHandle(module_name);

    // 检查module name合法性
    if (parameter_handle_ptr == nullptr) {
      SetErrorCode(ErrorCode::INVALID_MODULE_NAME, rsp);
      co_return aimrt::rpc::Status();
    }

    auto& pb_parameter_map = (*rsp.mutable_module_parameter_map())[module_name];

    const auto& parameter_names = parameter_handle_ptr->ListParameter();

    for (auto& parameter_name : parameter_names) {
      auto parameter_ptr = parameter_handle_ptr->GetParameter(parameter_name);
      if (!parameter_ptr) [[unlikely]] {
        continue;
      }

      SetPbParameter(parameter_ptr, &(*pb_parameter_map.mutable_value())[parameter_name]);
    }
  }

  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> ParameterServiceImpl::Load(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::parameter_plugin::LoadParameterReq& req,
    ::aimrt::protocols::parameter_plugin::LoadParameterRsp& rsp) {
  for (auto module_itr = req.module_parameter_map().begin();
       module_itr != req.module_parameter_map().end();
       ++module_itr) {
    std::string_view module_name(module_itr->first);
    auto* parameter_handle_ptr = parameter_manager_ptr_->GetParameterHandle(module_name);

    // 检查module name合法性
    if (parameter_handle_ptr == nullptr) {
      SetErrorCode(ErrorCode::INVALID_MODULE_NAME, rsp);
      co_return aimrt::rpc::Status();
    }

    const auto& pb_parameter_map = module_itr->second.value();
    for (auto parameter_itr = pb_parameter_map.begin();
         parameter_itr != pb_parameter_map.end();
         ++parameter_itr) {
      parameter_handle_ptr->SetParameter(
          parameter_itr->first, GetPbParameter(parameter_itr->second));
    }
  }

  co_return aimrt::rpc::Status();
}

}  // namespace aimrt::plugins::parameter_plugin
