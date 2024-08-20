
#include "zenoh_plugin/util.h"

namespace aimrt::plugins::zenoh_plugin {

// note:keyexpr cantnot begin with "/", but ulr can.
std::string Url2Keyexpr(const std::string& url) {
  return url.substr(1);
}
std::string Keyexpr2Url(const std::string& keyexpr) {
  return "/" + keyexpr;
}

}  // namespace aimrt::plugins::zenoh_plugin