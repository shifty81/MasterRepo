#pragma once
#include <string>
#include <vector>

namespace Runtime::World {

struct SceneDesc {
    std::string name;
    std::string path;
    bool        isLoaded = false;
    bool        isDirty  = false;
};

class SceneManager {
public:
    bool LoadScene(const std::string& path);
    bool SaveScene(const std::string& path);
    bool NewScene(const std::string& name);
    bool UnloadScene();

    bool             IsLoaded() const;
    const SceneDesc& GetCurrentScene() const;
    void             MarkDirty(bool dirty = true);

    std::vector<SceneDesc> GetRecentScenes() const;
    void AddToRecent(const SceneDesc& desc);
    void ClearRecent();

    bool SerializeToString(std::string& out) const;
    bool DeserializeFromString(const std::string& data);

private:
    SceneDesc              m_current;
    std::vector<SceneDesc> m_recentScenes;
    static constexpr int   MaxRecent = 10;
};

} // namespace Runtime::World
