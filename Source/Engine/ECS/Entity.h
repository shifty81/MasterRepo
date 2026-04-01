#pragma once
#include "Engine/ECS/World.h"

namespace NF {

/// @brief Lightweight handle that pairs an entity identifier with the World
///        that owns it, providing a convenient object-oriented interface.
struct Entity {
    /// @brief Numeric identifier of this entity within OwnerWorld.
    EntityId Id{NullEntity};

    /// @brief The World that manages the lifetime of this entity.
    World* OwnerWorld{nullptr};

    /// @brief Returns true if this handle refers to a valid, live entity.
    [[nodiscard]] bool IsValid() const noexcept {
        return OwnerWorld != nullptr && Id != NullEntity;
    }

    /// @brief Attach a component to this entity.
    template<typename T>
    void AddComponent(T comp) {
        OwnerWorld->AddComponent<T>(Id, std::move(comp));
    }

    /// @brief Retrieve a reference to the component of type T.
    template<typename T>
    T& GetComponent() { return OwnerWorld->GetComponent<T>(Id); }

    /// @brief Check whether this entity has a component of type T.
    template<typename T>
    bool HasComponent() const { return OwnerWorld->HasComponent<T>(Id); }
};

} // namespace NF
