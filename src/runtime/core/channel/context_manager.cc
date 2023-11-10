#include "core/channel/context_manager.h"

namespace aimrt::runtime::core::channel {

// todo: use object pool
ContextImpl* ContextManager::NewContext() { return new ContextImpl(); }

void ContextManager::DeleteContext(ContextImpl* ctx_ptr) { delete ctx_ptr; }

}  // namespace aimrt::runtime::core::channel
