#pragma once

#include "yaml-cpp/yaml.h"

namespace aimrt::runtime::core::util {

YAML::Node DeepCopyYamlNode(const YAML::Node& node);

std::string CheckYamlNodes(
    YAML::Node standard_node, YAML::Node checked_node, const std::string& path, int level = 0);

}  // namespace aimrt::runtime::core::util