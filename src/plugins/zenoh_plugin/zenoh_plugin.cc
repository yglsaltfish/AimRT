#include "zenoh_plugin/zenoh_plugin.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::zenoh_plugin::ZenohPlugin::Options> {
  using Options = aimrt::plugins::zenoh_plugin::ZenohPlugin::Options;

  static Node encode(const Options &rhs) {
    Node node;
    node["keyexpr"] = rhs.keyexpr;
    return node;
  }

  static bool decode(const Node &node, Options &rhs) {
    if (!node.IsMap()) return false;
    rhs.keyexpr = node["keyexpr"].as<std::string>();
    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::zenoh_plugin {

}  // namespace aimrt::plugins::zenoh_plugin