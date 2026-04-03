#include "Editor/Panels/EditorToolbar.h"
#include "Core/Logging/Log.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NODRAWTEXT  // prevent DrawText being macro-expanded to DrawTextA/DrawTextW
#include <windows.h>
#endif

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Colour constants
// ---------------------------------------------------------------------------
static constexpr uint32_t kToolbarBg     = 0x252527FF; // near-black strip
static constexpr uint32_t kBtnBg         = 0x3C3C3CFF; // normal button
static constexpr uint32_t kBtnBgHover    = 0x505053FF; // hovered button
static constexpr uint32_t kBtnBgPlay     = 0x2D6A2DFF; // green for Play
static constexpr uint32_t kBtnBgStop     = 0x6A2D2DFF; // red for Stop
static constexpr uint32_t kBtnBgLaunch   = 0x1E4B7AFF; // blue for Launch
static constexpr uint32_t kTextColor     = 0xCCCCCCFF;
static constexpr uint32_t kTitleColor    = 0xFFFFFFFF;
static constexpr uint32_t kSepColor      = 0x444444FF;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool EditorToolbar::DrawButton(float x, float y, float bw, float bh,
                                const char* label,
                                uint32_t bgColor, uint32_t textColor)
{
    if (!m_Renderer) return false;

    bool hovered = false;
    bool clicked = false;

    if (m_Input) {
        const float mx = m_Input->mouseX;
        const float my = m_Input->mouseY;
        hovered = (mx >= x && mx < x + bw && my >= y && my < y + bh);
        clicked = hovered && m_Input->leftJustPressed;
    }

    const float dpi = m_Renderer->GetDpiScale();
    const uint32_t fill = hovered ? kBtnBgHover : bgColor;
    m_Renderer->DrawRect({x, y, bw, bh}, fill);
    m_Renderer->DrawOutlineRect({x, y, bw, bh}, kSepColor);

    if (label) {
        m_Renderer->DrawText(label, x + 6.f * dpi, y + 4.f * dpi, textColor, 2.f);
    }

    return clicked;
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void EditorToolbar::Draw(float x, float y, float w, float h)
{
    if (!m_Renderer) return;

    const float dpi = m_Renderer->GetDpiScale();

    // Background strip
    m_Renderer->DrawRect({x, y, w, h}, kToolbarBg);
    // Bottom separator line
    m_Renderer->DrawRect({x, y + h - 1.f, w, 1.f}, kSepColor);

    // Title label on the left
    m_Renderer->DrawText("NovaForge", x + 8.f * dpi, y + 6.f * dpi, kTitleColor, 2.f);

    // Buttons — right-aligned group starting from left offset after title
    const float btnH = h - 6.f * dpi;
    const float btnY = y + 3.f * dpi;
    const float btnW = 80.f * dpi;
    const float gap  = 4.f * dpi;
    float bx = x + 160.f * dpi;

    // Play / Reset — restarts the interaction loop
    if (DrawButton(bx, btnY, btnW, btnH, "> Play", kBtnBgPlay, kTextColor)) {
        if (m_Loop) {
            m_Loop->Reset();
            Logger::Log(LogLevel::Info, "EditorToolbar", "Interaction loop reset (Play)");
        }
    }
    bx += btnW + gap;

    // Stop — also resets (placeholder for a future pause state)
    if (DrawButton(bx, btnY, btnW, btnH, "[] Stop", kBtnBgStop, kTextColor)) {
        if (m_Loop) {
            m_Loop->Reset();
            Logger::Log(LogLevel::Info, "EditorToolbar", "Interaction loop reset (Stop)");
        }
    }
    bx += btnW + gap;

#ifdef _WIN32
    // Launch standalone game client in a separate process
    const float launchW = 110.f * dpi;
    if (DrawButton(bx, btnY, launchW, btnH, "Launch Game", kBtnBgLaunch, kTextColor)) {
        Logger::Log(LogLevel::Info, "EditorToolbar", "Launching NovaForgeGame...");

        // Resolve the exe directory from the running editor's own path.
        wchar_t selfPath[MAX_PATH]{};
        GetModuleFileNameW(nullptr, selfPath, MAX_PATH);

        // Replace the editor exe name with the game exe name.
        wchar_t* lastSlash = wcsrchr(selfPath, L'\\');
        if (lastSlash) {
            *(lastSlash + 1) = L'\0';
            wchar_t gamePath[MAX_PATH]{};
            if (swprintf_s(gamePath, MAX_PATH, L"%s%s", selfPath, L"NovaForgeGame.exe") > 0) {
                STARTUPINFOW si{};
                si.cb = sizeof(si);
                PROCESS_INFORMATION pi{};
                if (CreateProcessW(gamePath, nullptr, nullptr, nullptr,
                                   FALSE, 0, nullptr, nullptr, &si, &pi)) {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                } else {
                    Logger::Log(LogLevel::Warning, "EditorToolbar",
                                "CreateProcess failed for NovaForgeGame.exe");
                }
            }
        }
    }
#endif
}

} // namespace NF::Editor
