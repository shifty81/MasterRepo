#pragma once

#include <string>

namespace nf
{
    class PropertyInspectorSystem;

    class Inspector
    {
    public:
        void SetPropertyInspectorSystem(PropertyInspectorSystem* system);
        const PropertyInspectorSystem* GetPropertyInspectorSystem() const noexcept;

        bool EditString(const std::string& propertyName, const std::string& value);
        bool EditInt(const std::string& propertyName, int value);
        bool EditFloat(const std::string& propertyName, float value);
        bool EditBool(const std::string& propertyName, bool value);

        std::string BuildSummaryText() const;

    private:
        PropertyInspectorSystem* m_system = nullptr;
    };
}
