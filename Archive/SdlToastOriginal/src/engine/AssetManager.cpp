#include "AssetManager.h"
#include "Renderer.h"
#include "Logger.h"
#include <iostream>

AssetManager::~AssetManager() {
    for (auto& pair : m_textures) {
        UnloadTexture(pair.second);
    }
    m_textures.clear();
}

bool AssetManager::Initialize(Renderer* renderer) {
    m_renderer = renderer;
    return true;
}

Texture2D AssetManager::LoadTexture(const std::string& filepath) {
    auto it = m_textures.find(filepath);
    if (it != m_textures.end()) {
        return it->second;
    }

    Texture2D texture = ::LoadTexture(filepath.c_str());
    if (texture.id == 0) {
        Logger::Instance().Error("Failed to load image " + filepath);
        return texture;
    }

    m_textures[filepath] = texture;
    return texture;
}

Texture2D AssetManager::GetTexture(const std::string& id) const {
    auto it = m_textures.find(id);
    if (it != m_textures.end()) {
        return it->second;
    }
    return Texture2D{};
}
