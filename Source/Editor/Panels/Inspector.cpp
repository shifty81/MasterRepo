#include "Source/Editor/Panels/Inspector.h"

namespace NF::Editor {

void Inspector::Update([[maybe_unused]] float dt) {}

void Inspector::Draw() {
    if (m_SelectedEntity == NullEntity || !m_World) return;
    // Component property widgets would be rendered here.
}

} // namespace NF::Editor
