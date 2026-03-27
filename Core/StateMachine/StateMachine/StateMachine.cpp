// StateMachine is header-only (template). This .cpp is a minimal translation unit
// that ensures the vtable for StateMachineBase is instantiated in this object file.
#include "Core/StateMachine/StateMachine/StateMachine.h"

namespace Core {
// ~StateMachineBase is defined as `= default` in the header (virtual).
// Nothing more needed here.
} // namespace Core
