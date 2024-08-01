#pragma once

#include <functional>

#include "core/rpc/rpc_invoke_wrapper.h"
#include "util/exception.h"

namespace aimrt::runtime::core::rpc {

using FrameworkAsyncRpcHandle = std::function<void(const std::shared_ptr<InvokeWrapper>&)>;
using FrameworkAsyncRpcFilter = std::function<void(const std::shared_ptr<InvokeWrapper>&, FrameworkAsyncRpcHandle&&)>;

class FrameworkAsyncFilterCollector {
 public:
  FrameworkAsyncFilterCollector()
      : final_filter_(
            [](const std::shared_ptr<InvokeWrapper>& invoke_wrapper, FrameworkAsyncRpcHandle&& h) {
              h(invoke_wrapper);
            }) {}
  ~FrameworkAsyncFilterCollector() = default;

  FrameworkAsyncFilterCollector(const FrameworkAsyncFilterCollector&) = delete;
  FrameworkAsyncFilterCollector& operator=(const FrameworkAsyncFilterCollector&) = delete;

  void RegisterFilter(const FrameworkAsyncRpcFilter& filter) {
    final_filter_ =
        [final_filter{std::move(final_filter_)}, &filter](
            const std::shared_ptr<InvokeWrapper>& invoke_wrapper, FrameworkAsyncRpcHandle&& h) {
          filter(
              invoke_wrapper,
              [&final_filter, h{std::move(h)}](const std::shared_ptr<InvokeWrapper>& invoke_wrapper) mutable {
                final_filter(invoke_wrapper, std::move(h));
              });
        };
  }

  void InvokeRpc(
      FrameworkAsyncRpcHandle&& h, const std::shared_ptr<InvokeWrapper>& invoke_wrapper) const {
    return final_filter_(invoke_wrapper, std::move(h));
  }

  void Clear() {
    final_filter_ = FrameworkAsyncRpcFilter();
  }

 private:
  FrameworkAsyncRpcFilter final_filter_;
};

class FrameworkAsyncFilterManager {
 public:
  FrameworkAsyncFilterManager() = default;
  ~FrameworkAsyncFilterManager() = default;

  FrameworkAsyncFilterManager(const FrameworkAsyncFilterManager&) = delete;
  FrameworkAsyncFilterManager& operator=(const FrameworkAsyncFilterManager&) = delete;

  void RegisterFilter(std::string_view name, FrameworkAsyncRpcFilter&& filter) {
    auto emplace_ret = filter_map_.emplace(name, std::move(filter));
    AIMRT_ASSERT(emplace_ret.second, "Register filter {} failed.", name);
  }

  std::vector<std::string> GetAllFiltersName() const {
    std::vector<std::string> result;
    for (const auto& itr : filter_map_)
      result.emplace_back(itr.first);

    return result;
  }

  void CreateFilterCollector(
      std::string_view func_name, const std::vector<std::string>& filter_name_vec) {
    if (filter_name_vec.empty()) [[unlikely]]
      return;

    auto collector_ptr = std::make_unique<FrameworkAsyncFilterCollector>();

    for (auto& name : filter_name_vec) {
      auto find_itr = filter_map_.find(name);
      AIMRT_ASSERT(find_itr != filter_map_.end(), "Can not find filter: {}", name);

      collector_ptr->RegisterFilter(find_itr->second);
    }

    auto emplace_ret = filter_collector_map_.emplace(func_name, std::move(collector_ptr));

    AIMRT_ASSERT(emplace_ret.second, "Func {} set filter collector failed.", func_name);

    filter_names_map_.emplace(func_name, filter_name_vec);
  }

  const FrameworkAsyncFilterCollector& GetFilterCollector(std::string_view func_name) const {
    auto find_itr = filter_collector_map_.find(func_name);
    if (find_itr != filter_collector_map_.end()) {
      return *(find_itr->second);
    }

    return default_filter_collector_;
  }

  std::vector<std::string> GetFilterNameVec(std::string_view func_name) const {
    auto find_itr = filter_names_map_.find(func_name);
    if (find_itr != filter_names_map_.end()) {
      return find_itr->second;
    }

    return {};
  }

  void Clear() {
    filter_collector_map_.clear();
    filter_map_.clear();
  }

 private:
  // filter name - filter
  std::unordered_map<std::string, FrameworkAsyncRpcFilter> filter_map_;

  // func name - filter collector
  using FilterCollectorMap = std::unordered_map<
      std::string,
      std::unique_ptr<FrameworkAsyncFilterCollector>,
      aimrt::common::util::StringHash,
      std::equal_to<>>;
  FilterCollectorMap filter_collector_map_;

  // func name - filter name vec
  using FilterNameMap = std::unordered_map<
      std::string,
      std::vector<std::string>,
      aimrt::common::util::StringHash,
      std::equal_to<>>;
  FilterNameMap filter_names_map_;

  FrameworkAsyncFilterCollector default_filter_collector_;
};

}  // namespace aimrt::runtime::core::rpc