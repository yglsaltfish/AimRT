
#pragma once

#include <memory>
#include <unordered_map>

#include <lcm/lcm-cpp.hpp>

namespace aimrt::plugins::lcm_plugin {

using LcmPtr = std::shared_ptr<lcm::LCM>;

class LcmManager {
 public:
  static LcmManager& GetInstance() {
    static LcmManager instance;
    return instance;
  }

  LcmPtr GetLcm(std::string url = "") {
    if (lcm_map_.count(url) == 0) {
      return CreateLcm(url);
    }

    return lcm_map_[url];
  }

  std::string GetDefaultLcmUrl() {
    return "udpm://239.255.76.67:7667?ttl=0";
  }

 private:
  LcmManager() = default;
  LcmManager(const LcmManager&) = delete;
  LcmManager& operator=(const LcmManager&) = delete;
  LcmPtr CreateLcm(std::string url) {
    auto lcm = std::make_shared<lcm::LCM>(url);
    if (!lcm->good()) {
      return nullptr;
    }
    lcm_map_[url] = lcm;
    return lcm;
  }

 private:
  std::unordered_map<std::string, LcmPtr> lcm_map_;
};

}  // namespace aimrt::plugins::lcm_plugin
