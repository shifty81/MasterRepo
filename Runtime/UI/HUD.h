#pragma once
#include <string>
#include <vector>
#include <algorithm>

namespace Runtime::UI {

struct HUDElement {
    std::string id;
    std::string type;    // "healthbar", "ammo", "minimap", "crosshair", …
    float       x       = 0.0f;
    float       y       = 0.0f;
    float       width   = 100.0f;
    float       height  = 20.0f;
    bool        visible = true;
};

class HUD {
public:
    void AddElement(const HUDElement& elem);
    void RemoveElement(const std::string& id);

    HUDElement*       GetElement(const std::string& id);
    const HUDElement* GetElement(const std::string& id) const;

    void SetVisible(const std::string& id, bool visible);

    void Update(float dt);

    // Integration point: called by the renderer each frame.
    void Render();

    const std::vector<HUDElement>& GetElements() const { return m_elements; }

private:
    std::vector<HUDElement> m_elements;
};

} // namespace Runtime::UI
