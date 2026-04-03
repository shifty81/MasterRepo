#include "Editor/Inspector/InspectorPanel.h"
#include "Editor/Inspector/PropertyInspectorSystem.h"

#include <sstream>

namespace nf
{
    void Inspector::SetPropertyInspectorSystem(PropertyInspectorSystem* system)
    {
        m_system = system;
    }

    const PropertyInspectorSystem* Inspector::GetPropertyInspectorSystem() const noexcept
    {
        return m_system;
    }

    bool Inspector::EditString(const std::string& propertyName, const std::string& value)
    {
        return m_system != nullptr && m_system->ApplyEdit(propertyName, value);
    }

    bool Inspector::EditInt(const std::string& propertyName, int value)
    {
        return m_system != nullptr && m_system->ApplyEdit(propertyName, value);
    }

    bool Inspector::EditFloat(const std::string& propertyName, float value)
    {
        return m_system != nullptr && m_system->ApplyEdit(propertyName, value);
    }

    bool Inspector::EditBool(const std::string& propertyName, bool value)
    {
        return m_system != nullptr && m_system->ApplyEdit(propertyName, value);
    }

    std::string Inspector::BuildSummaryText() const
    {
        if (m_system == nullptr || !m_system->HasProperties())
        {
            return "Inspector: No properties";
        }

        const auto& propertySet = m_system->GetPropertySet();
        std::ostringstream oss;
        oss << propertySet.title << (propertySet.dirty ? " [Dirty]" : "");

        for (const auto& entry : propertySet.entries)
        {
            oss << "\n- " << entry.name << ": "
                << PropertyInspectorSystem::ToDisplayString(entry);

            if (entry.dirty)
            {
                oss << " *";
            }
        }

        return oss.str();
    }
}
