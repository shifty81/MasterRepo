#pragma once
#include <string>

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Panel for browsing project content assets.
class ContentBrowser {
public:
    /// @brief Set the root directory to browse.
    /// @param path Filesystem path to the content root.
    void SetRootPath(std::string path) { m_RootPath = std::move(path); }

    /// @brief Set the UIRenderer used for drawing.
    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Advance panel state.
    void Update(float dt);

    /// @brief Draw the content browser panel within the given region.
    void Draw(float x, float y, float w, float h);

    /// @brief Return the path of the currently selected asset, or empty if none.
    [[nodiscard]] const std::string& GetSelectedAsset() const noexcept { return m_SelectedAsset; }

private:
    std::string  m_RootPath;
    std::string  m_SelectedAsset;
    UIRenderer*  m_Renderer{nullptr};
};

} // namespace NF::Editor
