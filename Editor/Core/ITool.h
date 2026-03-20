#pragma once
// ITool interface for editor tools — migrated from atlas::tools
namespace Editor {

class ITool {
public:
    virtual ~ITool() = default;
    virtual std::string Name()  const = 0;
    virtual void Activate()           = 0;
    virtual void Deactivate()         = 0;
    virtual void Update(float dt)     = 0;
    virtual bool IsActive()     const = 0;
};

} // namespace Editor
