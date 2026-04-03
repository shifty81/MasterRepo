#pragma once

#include <string>
#include <variant>
#include <vector>

namespace nf
{
    struct PropertyVec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    enum class PropertyType
    {
        Bool = 0,
        Int,
        Float,
        String,
        Vec3
    };

    enum class PropertyWidgetHint
    {
        Default = 0,
        Checkbox,
        NumericField,
        TextField,
        Vec3Field,
        ReadOnlyLabel
    };

    using PropertyValue = std::variant<bool, int, float, std::string, PropertyVec3>;

    struct PropertyEntry
    {
        std::string name;
        PropertyType type = PropertyType::String;
        PropertyWidgetHint widgetHint = PropertyWidgetHint::Default;
        PropertyValue value {};
        PropertyValue originalValue {};
        bool readOnly = false;
        bool dirty = false;
    };

    struct PropertySet
    {
        std::string title = "Properties";
        std::vector<PropertyEntry> entries;
        bool dirty = false;
    };

    class PropertyInspectorSystem
    {
    public:
        void Clear();
        void SetPropertySet(PropertySet propertySet);

        bool HasProperties() const noexcept;
        const PropertySet& GetPropertySet() const noexcept;
        PropertySet& GetMutablePropertySet() noexcept;

        bool ApplyEdit(const std::string& propertyName, const PropertyValue& newValue);
        bool IsDirty() const noexcept;
        void ClearDirty();

        static std::string ToDisplayString(const PropertyEntry& entry);

    private:
        static bool ValuesEqual(const PropertyValue& a, const PropertyValue& b);

    private:
        PropertySet m_propertySet;
        bool m_hasProperties = false;
    };
}
