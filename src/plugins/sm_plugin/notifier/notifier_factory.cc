
#include "notifier_factory.h"

#include "shm_notifier.h"

namespace aimrt::plugins::sm_plugin {

NotifierBasePtr NotifierFactory::Create(NotifierType type) {
  switch (type) {
    case NotifierType::SHM:
      return std::make_shared<ShmNotifier>();
    default:
      return nullptr;
  }
  return nullptr;
}

}  // namespace aimrt::plugins::sm_plugin
