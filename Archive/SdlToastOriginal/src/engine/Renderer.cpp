#include "Renderer.h"
#include "Logger.h"
#include <iostream>

Renderer::Renderer()
    : m_cameraX(0)
    , m_cameraY(0)
    , m_width(0)
    , m_height(0)
{
}

Renderer::~Renderer() {
}

bool Renderer::Initialize(int width, int height) {
    m_width = width;
    m_height = height;
    return true;
}

void Renderer::Clear(unsigned char r, unsigned char g, unsigned char b) {
    BeginDrawing();
    ClearBackground(Color{r, g, b, 255});
}

void Renderer::Present() {
    EndDrawing();
}

void Renderer::DrawRect(int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    DrawRectangleLines(x - m_cameraX, y - m_cameraY, w, h, Color{r, g, b, a});
}

void Renderer::FillRect(int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    DrawRectangle(x - m_cameraX, y - m_cameraY, w, h, Color{r, g, b, a});
}

void Renderer::DrawTextureRect(Texture2D texture, int x, int y) {
    if (texture.id == 0) return;
    DrawTexture(texture, x - m_cameraX, y - m_cameraY, WHITE);
}

void Renderer::DrawTextureRect(Texture2D texture, const Rectangle* srcRect, const Rectangle* dstRect) {
    if (texture.id == 0) return;
    Rectangle adjustedDst = *dstRect;
    adjustedDst.x -= m_cameraX;
    adjustedDst.y -= m_cameraY;
    DrawTexturePro(texture, *srcRect, adjustedDst, Vector2{0, 0}, 0.0f, WHITE);
}

void Renderer::DrawGameText(const char* text, int x, int y, int fontSize, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    DrawText(text, x, y, fontSize, Color{r, g, b, a});
}
