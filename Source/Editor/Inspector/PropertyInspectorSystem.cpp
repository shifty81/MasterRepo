#include "Editor/Inspector/PropertyInspectorSystem.h"

#include <sstream>

namespace nf
{
    namespace
    {
        std::string Vec3ToString(const PropertyVec3& v)
        {
            std::ostringstream oss;
            oss << "(" << v.x << ", " << v.y << ", " << v.z << ")";
            return oss.str();
        }

        bool Vec3Equal(const PropertyVec3& a, const PropertyVec3& b)
        {
            return a.x == b.x && a.y == b.y && a.z == b.z;
        }
    }

    void PropertyInspectorSystem::Clear()
    {
        m_propertySet = {};
        m_hasProperties = false;
    }

    void PropertyInspectorSystem::SetPropertySet(PropertySet propertySet)
    {
        for (auto& entry : propertySet.entries)
        {
            entry.originalValue = entry.value;
            entry.dirty = false;
        }

        propertySet.dirty = false;
        m_propertySet = std::move(propertySet);
        m_hasProperties = true;
    }

    bool PropertyInspectorSystem::HasProperties() const noexcept
    {
        return m_hasProperties;
    }

    const PropertySet& PropertyInspectorSystem::GetPropertySet() const noexcept
    {
        return m_propertySet;
    }

    PropertySet& PropertyInspectorSystem::GetMutablePropertySet() noexcept
    {
        return m_propertySet;
    }

    bool PropertyInspectorSystem::ApplyEdit(const std::string& propertyName, const PropertyValue& newValue)
    {
        if (!m_hasProperties)
        {
            return false;
        }

        for (auto& entry : m_propertySet.entries)
        {
            if (entry.name != propertyName || entry.readOnly)
            {
                continue;
            }

            entry.value = newValue;
            entry.dirty = !ValuesEqual(entry.value, entry.originalValue);

            bool anyDirty = false;
            for (const auto& e : m_propertySet.entries)
            {
                if (e.dirty)
                {
                    anyDirty = true;
                    break;
                }
            }
            m_propertySet.dirty = anyDirty;
            return true;
        }

        return false;
    }

    bool PropertyInspectorSystem::IsDirty() const noexcept
    {
        return m_hasProperties && m_propertySet.dirty;
    }

    void PropertyInspectorSystem::ClearDirty()
    {
        if (!m_hasProperties)
        {
            return;
        }

        m_propertySet.dirty = false;
        for (auto& entry : m_propertySet.entries)
        {
            entry.originalValue = entry.value;
            entry.dirty = false;
        }
    }

    std::string PropertyInspectorSystem::ToDisplayString(const PropertyEntry& entry)
    {
        struct Visitor
        {
            std::string operator()(bool value) const { return value ? "true" : "false"; }
            std::string operator()(int value) const { return std::to_string(value); }
            std::string operator()(float value) const
            {
                std::ostringstream oss;
                oss << value;
                return oss.str();
            }
            std::string operator()(const std::string& value) const { return value; }
            std::string operator()(const PropertyVec3& value) const { return Vec3ToString(value); }
        };

        return std::visit(Visitor{}, entry.value);
    }

    bool PropertyInspectorSystem::ValuesEqual(const PropertyValue& a, const PropertyValue& b)
    {
        if (a.index() != b.index())
        {
            return false;
        }

        if (std::holds_alternative<bool>(a))
        {
            return std::get<bool>(a) == std::get<bool>(b);
        }
        if (std::holds_alternative<int>(a))
        {
            return std::get<int>(a) == std::get<int>(b);
        }
        if (std::holds_alternative<float>(a))
        {
            return std::get<float>(a) == std::get<float>(b);
        }
        if (std::holds_alternative<std::string>(a))
        {
            return std::get<std::string>(a) == std::get<std::string>(b);
        }
        return Vec3Equal(std::get<PropertyVec3>(a), std::get<PropertyVec3>(b));
    }
}
