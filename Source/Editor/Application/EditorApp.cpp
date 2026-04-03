#include "Editor/Application/EditorApp.h"
#include "Core/Config/ProjectManifest.h"
#include "Core/Logging/Log.h"
#include <chrono>

#ifdef _WIN32
// Require Windows 10 minimum for per-monitor V2 DPI awareness and GetDpiForWindow.
// Raise _WIN32_WINNT to 0x0A00 if a lower version was already defined.
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0A00
#  undef  _WIN32_WINNT
#  define _WIN32_WINNT 0x0A00
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM
// Windows defines DrawText as a macro (DrawTextA/DrawTextW).  Undefine it so
// calls to NF::UIRenderer::DrawText are not silently rewritten to DrawTextA.
#ifdef DrawText
#  undef DrawText
#endif
#endif

namespace NF::Editor {

#ifdef _WIN32
// ---------------------------------------------------------------------------
// Win32 window procedure
// ---------------------------------------------------------------------------

static LRESULT CALLBACK EditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    // Route all other messages through the EditorApp stored in user data.
    // GWLP_USERDATA is set after CreateWindowExW returns, so it may be null
    // for messages fired during window creation — that is safe.
    EditorApp* app = reinterpret_cast<EditorApp*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (app)
    {
        // WM_DPICHANGED: reposition to the OS-suggested rect and return 0 so
        // DefWindowProc does not override our placement.
        if (msg == WM_DPICHANGED)
        {
            app->DispatchOsEvent(msg,
                static_cast<uintptr_t>(wParam),
                static_cast<intptr_t>(lParam));
            return 0;
        }
        app->DispatchOsEvent(msg,
            static_cast<uintptr_t>(wParam),
            static_cast<intptr_t>(lParam));
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// DispatchOsEvent — updates m_Input and m_DpiScale from raw Win32 messages
// ---------------------------------------------------------------------------

void EditorApp::DispatchOsEvent(unsigned msg, uintptr_t wParam, intptr_t lParam) noexcept
{
    const WPARAM wp = static_cast<WPARAM>(wParam);
    const LPARAM lp = static_cast<LPARAM>(lParam);

    switch (msg)
    {
    case WM_MOUSEMOVE:
    {
        const float newX = static_cast<float>(GET_X_LPARAM(lp));
        const float newY = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.mouseDeltaX = newX - m_Input.mouseX;
        m_Input.mouseDeltaY = newY - m_Input.mouseY;
        m_Input.mouseX = newX;
        m_Input.mouseY = newY;
        break;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        m_Input.mouseX          = static_cast<float>(GET_X_LPARAM(lp));
        m_Input.mouseY          = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.leftDown        = true;
        m_Input.leftJustPressed = true;
        SetCapture(static_cast<HWND>(m_Hwnd));
        break;

    case WM_LBUTTONUP:
        m_Input.mouseX           = static_cast<float>(GET_X_LPARAM(lp));
        m_Input.mouseY           = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.leftDown         = false;
        m_Input.leftJustReleased = true;
        ReleaseCapture();
        break;

    case WM_RBUTTONDOWN:
        m_Input.mouseX           = static_cast<float>(GET_X_LPARAM(lp));
        m_Input.mouseY           = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.rightDown        = true;
        m_Input.rightJustPressed = true;
        SetCapture(static_cast<HWND>(m_Hwnd));
        break;

    case WM_RBUTTONUP:
        m_Input.mouseX  = static_cast<float>(GET_X_LPARAM(lp));
        m_Input.mouseY  = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.rightDown = false;
        ReleaseCapture();
        break;

    case WM_MBUTTONDOWN:
        m_Input.mouseX            = static_cast<float>(GET_X_LPARAM(lp));
        m_Input.mouseY            = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.middleDown        = true;
        m_Input.middleJustPressed = true;
        SetCapture(static_cast<HWND>(m_Hwnd));
        break;

    case WM_MBUTTONUP:
        m_Input.mouseX      = static_cast<float>(GET_X_LPARAM(lp));
        m_Input.mouseY      = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.middleDown  = false;
        ReleaseCapture();
        break;

    case WM_MOUSEWHEEL:
        m_Input.wheelDelta +=
            static_cast<float>(GET_WHEEL_DELTA_WPARAM(wp)) / static_cast<float>(WHEEL_DELTA);
        break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        const unsigned vk = static_cast<unsigned>(wParam) & 0xFFu;
        if (!m_Input.keysDown[vk])
            m_Input.keysJustPressed[vk] = true;
        m_Input.keysDown[vk] = true;
        break;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        const unsigned vk = static_cast<unsigned>(wParam) & 0xFFu;
        m_Input.keysDown[vk] = false;
        break;
    }

    case WM_DPICHANGED:
    {
        m_DpiScale = static_cast<float>(HIWORD(wp)) / 96.f;
        m_UIRenderer.SetDpiScale(m_DpiScale);
        // Reposition window to the DPI-suggested rect.
        const RECT* suggested = reinterpret_cast<const RECT*>(lp);
        SetWindowPos(static_cast<HWND>(m_Hwnd), nullptr,
                     suggested->left, suggested->top,
                     suggested->right  - suggested->left,
                     suggested->bottom - suggested->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        break;
    }

    default:
        break;
    }
}
#endif

bool EditorApp::Init() {
    Logger::Log(LogLevel::Info, "Editor", "[1/6] EditorApp — loading project manifest");

    // Step 1 — load project manifest
    ProjectManifest manifest;
    {
        const std::string manifestPath = "Config/novaforge.project.json";
        if (manifest.LoadFromFile(manifestPath))
        {
            manifest.LogSummary();
        }
        else
        {
            Logger::Log(LogLevel::Warning, "Editor",
                        "Project manifest not found; using defaults");
        }
    }

    Logger::Log(LogLevel::Info, "Editor", "[2/6] EditorApp — creating platform window");

#ifdef _WIN32
    // Enable per-monitor V2 DPI awareness before any window is created so the
    // OS sends WM_DPICHANGED and reports correct DPI via GetDpiForWindow.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = EditorWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"NovaForgeEditor";

    if (!RegisterClassExW(&wc))
    {
        Logger::Log(LogLevel::Error, "Editor", "Failed to register window class");
        return false;
    }

    HWND hwnd = CreateWindowExW(
        0,
        L"NovaForgeEditor",
        L"NovaForge Editor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 720,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        Logger::Log(LogLevel::Error, "Editor", "Failed to create window");
        UnregisterClassW(L"NovaForgeEditor", hInstance);
        return false;
    }

    // Store 'this' in the window's user data so EditorWndProc can route
    // input events back to us without a global variable.
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // Query the monitor DPI now that the window exists.
    {
        UINT dpi = GetDpiForWindow(hwnd);
        m_DpiScale = static_cast<float>(dpi) / 96.f;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    m_Hwnd = hwnd;
#endif

    Logger::Log(LogLevel::Info, "Editor", "[3/6] EditorApp — initialising render device");
    m_RenderDevice = std::make_unique<RenderDevice>();

    // Use OpenGL when available, passing the native window handle so the
    // device can create a rendering context.  Falls back to Null if OpenGL
    // was not compiled in.
#ifdef NF_HAS_OPENGL
    const GraphicsAPI selectedAPI = GraphicsAPI::OpenGL;
#else
    const GraphicsAPI selectedAPI = GraphicsAPI::Null;
#endif
    if (!m_RenderDevice->Init(selectedAPI, m_Hwnd)) {
        Logger::Log(LogLevel::Error, "Editor", "RenderDevice init failed");
        return false;
    }

    Logger::Log(LogLevel::Info, "Editor", "[4/6] EditorApp — loading dev world");
    // Load the dev world via GameWorld (Phase 1: wired editor load path)
    {
        const std::string contentRoot =
            manifest.IsValid() ? manifest.ContentRoot : "Content";
        m_GameWorld.Initialize(contentRoot);
    }
    m_Level.Load(manifest.IsValid() ? manifest.DefaultWorld : "DevWorld");

    Logger::Log(LogLevel::Info, "Editor", "[5/6] EditorApp — setting up viewport");
    // Viewport
    m_Viewport.Init(m_RenderDevice.get());
    m_Viewport.Resize(m_ClientWidth, m_ClientHeight);

    // Set initial OpenGL viewport to match the window client area.
    m_RenderDevice->Resize(m_ClientWidth, m_ClientHeight);

    // Initialise the custom UI renderer
    m_UIRenderer.Init();
    m_UIRenderer.SetViewportSize(static_cast<float>(m_ClientWidth),
                                  static_cast<float>(m_ClientHeight));
    m_UIRenderer.SetDpiScale(m_DpiScale);

    // Initialise the forward renderer and chunk mesh cache (Phase 4).
    m_ForwardRenderer.Init(m_RenderDevice.get());
    m_MeshCache.Init(&m_ForwardRenderer);
    m_MeshCache.RebuildDirty(m_GameWorld.GetChunkMap());

    Logger::Log(LogLevel::Info, "Editor", "[6/6] EditorApp — wiring panels and layout");
    // Wire UIRenderer to all subsystems
    m_DockingSystem.SetUIRenderer(&m_UIRenderer);
    m_SceneOutliner.SetUIRenderer(&m_UIRenderer);
    m_Inspector.SetUIRenderer(&m_UIRenderer);
    m_ContentBrowser.SetUIRenderer(&m_UIRenderer);
    m_ConsolePanel.SetUIRenderer(&m_UIRenderer);
    m_Viewport.SetUIRenderer(&m_UIRenderer);
    m_VoxelInspector.SetUIRenderer(&m_UIRenderer);
    m_HUDPanel.SetUIRenderer(&m_UIRenderer);

    // Wire input state to interactive panels
    m_SceneOutliner.SetInputState(&m_Input);
    m_ConsolePanel.SetInputState(&m_Input);
    m_ContentBrowser.SetInputState(&m_Input);
    m_Viewport.SetInputState(&m_Input);
    m_VoxelInspector.SetInputState(&m_Input);
    m_HUDPanel.SetInputState(&m_Input);

    // Wire voxel layer
    m_VoxelInspector.SetChunkMap(&m_GameWorld.GetChunkMap());

    // Wire Phase 3 interaction loop
    m_InteractionLoop.Init(&m_GameWorld.GetVoxelEditApi());
    m_HUDPanel.SetInteractionLoop(&m_InteractionLoop);

    // Wire toolbar
    m_Toolbar.SetUIRenderer(&m_UIRenderer);
    m_Toolbar.SetInteractionLoop(&m_InteractionLoop);
    m_Toolbar.SetInputState(&m_Input);

    // Initialize command registry and menu bar
    {
        nf::EditorCommandContext cmdContext;
        cmdContext.app = nullptr;  // We don't have an nf::EditorApp, so use nullptr for now
        // Note: The nf:: command context expects nf:: namespace panels, but we have NF::Editor:: panels
        // For now, we'll leave these nullptr and not pass the panel references
        cmdContext.console = nullptr;
        cmdContext.contentBrowser = nullptr;
        cmdContext.inspector = nullptr;
        m_CommandRegistry.SetContext(cmdContext);

        // Register core commands
        RegisterEditorCommands();

        // Build menus
        m_MenuBar.SetCommandRegistry(&m_CommandRegistry);
        m_MenuBar.BuildDefaultMenus();
    }

    // Panels
    m_SceneOutliner.SetWorld(&m_Level.GetWorld());
    m_SceneOutliner.SetOnSelectionChanged([this](EntityId id) {
        m_Inspector.SetSelectedEntity(id, &m_Level.GetWorld());
    });

    // Use manifest content root if available, else default
    m_ContentBrowser.SetRootPath(
        manifest.IsValid() ? manifest.ContentRoot : "Content");

    // Register panels with the docking system — callbacks now forward bounds
    m_DockingSystem.RegisterPanel("SceneOutliner",
        [this](float x, float y, float w, float h) {
            m_SceneOutliner.Draw(x, y, w, h);
        });
    m_DockingSystem.RegisterPanel("Viewport",
        [this](float x, float y, float w, float h) {
            m_Viewport.Draw(x, y, w, h);
        });
    m_DockingSystem.RegisterPanel("Inspector",
        [this](float x, float y, float w, float h) {
            m_Inspector.Draw(x, y, w, h);
        });
    m_DockingSystem.RegisterPanel("ContentBrowser",
        [this](float x, float y, float w, float h) {
            m_ContentBrowser.Draw(x, y, w, h);
        });
    m_DockingSystem.RegisterPanel("Console",
        [this](float x, float y, float w, float h) {
            m_ConsolePanel.Draw(x, y, w, h);
        });
    m_DockingSystem.RegisterPanel("VoxelInspector",
        [this](float x, float y, float w, float h) {
            m_VoxelInspector.Draw(x, y, w, h);
        });
    m_DockingSystem.RegisterPanel("HUD",
        [this](float x, float y, float w, float h) {
            m_HUDPanel.Draw(x, y, w, h);
        });

    // Default layout:
    //   SceneOutliner (20%) | Viewport+Console (56%) | Inspector+ContentBrowser+VoxelInspector (24%)
    m_DockingSystem.SetRootSplit("SceneOutliner", "Viewport", 0.20f);
    m_DockingSystem.SplitPanel("Viewport",  "Inspector",      SplitAxis::Horizontal, 0.70f);
    m_DockingSystem.SplitPanel("Inspector", "ContentBrowser",  SplitAxis::Vertical,  0.40f);
    m_DockingSystem.SplitPanel("ContentBrowser", "VoxelInspector", SplitAxis::Vertical, 0.50f);
    m_DockingSystem.SplitPanel("VoxelInspector", "HUD",            SplitAxis::Vertical, 0.50f);
    m_DockingSystem.SplitPanel("Viewport",  "Console",         SplitAxis::Vertical,  0.75f);

    m_Running = true;
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Init complete");
    return true;
}

void EditorApp::RegisterEditorCommands()
{
    using namespace nf;

    // File.Exit
    m_CommandRegistry.Register(EditorCommand{
        "File.Exit",
        "Exit",
        nullptr,  // always can execute
        [this](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "File.Exit command triggered");
            m_Running = false;
        }
    });

    // World.SaveDevWorld
    m_CommandRegistry.Register(EditorCommand{
        "World.SaveDevWorld",
        "Save World",
        nullptr,
        [this](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Saving dev world...");
            m_GameWorld.SaveWorld("Content/Worlds/DevWorld.nfsv");
            Logger::Log(LogLevel::Info, "Editor", "World saved");
        }
    });

    // World.LoadDevWorld
    m_CommandRegistry.Register(EditorCommand{
        "World.LoadDevWorld",
        "Load World",
        nullptr,
        [this](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Loading dev world...");
            m_GameWorld.LoadWorld("Content/Worlds/DevWorld.nfsv");
            m_MeshCache.RebuildDirty(m_GameWorld.GetChunkMap());
            Logger::Log(LogLevel::Info, "Editor", "World loaded");
        }
    });

    // View.ToggleConsole
    m_CommandRegistry.Register(EditorCommand{
        "View.ToggleConsole",
        "Toggle Console",
        nullptr,
        [](EditorCommandContext& ctx) {
            Logger::Log(LogLevel::Info, "Editor", "Toggle console (placeholder)");
            // TODO: implement panel visibility toggle
        }
    });

    // Help.About
    m_CommandRegistry.Register(EditorCommand{
        "Help.About",
        "About",
        nullptr,
        [](EditorCommandContext& ctx) {
            Logger::Log(LogLevel::Info, "Editor", "NovaForge Editor - Phase 9 Complete (Build with consolidated editor)");
        }
    });

    // World.SaveDevWorldAs
    m_CommandRegistry.Register(EditorCommand{
        "World.SaveDevWorldAs",
        "Save World As...",
        nullptr,
        [this](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Save World As (placeholder - would open file dialog)");
        }
    });

    // File.NewWorld
    m_CommandRegistry.Register(EditorCommand{
        "File.NewWorld",
        "New World",
        nullptr,
        [this](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "New World (placeholder - would create new world)");
        }
    });

    // File.OpenWorld
    m_CommandRegistry.Register(EditorCommand{
        "File.OpenWorld",
        "Open World...",
        nullptr,
        [this](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Open World (placeholder - would open file dialog)");
        }
    });

    // World.ReloadDevWorld
    m_CommandRegistry.Register(EditorCommand{
        "World.ReloadDevWorld",
        "Reload World",
        nullptr,
        [this](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Reloading dev world...");
            m_GameWorld.LoadWorld("Content/Worlds/DevWorld.nfsv");
            m_MeshCache.RebuildDirty(m_GameWorld.GetChunkMap());
            Logger::Log(LogLevel::Info, "Editor", "World reloaded");
        }
    });

    // World.ValidateState
    m_CommandRegistry.Register(EditorCommand{
        "World.ValidateState",
        "Validate World State",
        nullptr,
        [](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "World validation (placeholder)");
        }
    });

    // Tool commands (placeholders for now)
    m_CommandRegistry.Register(EditorCommand{
        "Tools.SelectMode",
        "Select Mode",
        nullptr,
        [](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Switched to Select mode");
        }
    });

    m_CommandRegistry.Register(EditorCommand{
        "Tools.VoxelInspectMode",
        "Voxel Inspect Mode",
        nullptr,
        [](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Switched to Voxel Inspect mode");
        }
    });

    m_CommandRegistry.Register(EditorCommand{
        "Tools.VoxelAddMode",
        "Voxel Add Mode",
        nullptr,
        [](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Switched to Voxel Add mode");
        }
    });

    m_CommandRegistry.Register(EditorCommand{
        "Tools.VoxelRemoveMode",
        "Voxel Remove Mode",
        nullptr,
        [](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Switched to Voxel Remove mode");
        }
    });

    // Help.Controls
    m_CommandRegistry.Register(EditorCommand{
        "Help.Controls",
        "Editor Controls",
        nullptr,
        [](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "=== Editor Controls ===");
            Logger::Log(LogLevel::Info, "Editor", "Viewport: RMB=Rotate, MMB=Pan, Wheel=Zoom");
            Logger::Log(LogLevel::Info, "Editor", "Toolbar: Play=Reset interaction, Launch Game=Open standalone client");
        }
    });

    // Edit commands (placeholders)
    m_CommandRegistry.Register(EditorCommand{
        "Edit.Undo",
        "Undo",
        nullptr,
        [](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Undo (not yet implemented)");
        }
    });

    m_CommandRegistry.Register(EditorCommand{
        "Edit.Redo",
        "Redo",
        nullptr,
        [](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Redo (not yet implemented)");
        }
    });

    // View commands (placeholders)
    m_CommandRegistry.Register(EditorCommand{
        "View.ResetLayout",
        "Reset Layout",
        nullptr,
        [](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Reset layout (placeholder)");
        }
    });

    m_CommandRegistry.Register(EditorCommand{
        "View.ToggleContentBrowser",
        "Toggle Content Browser",
        nullptr,
        [](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Toggle content browser (placeholder)");
        }
    });

    m_CommandRegistry.Register(EditorCommand{
        "View.ToggleInspector",
        "Toggle Inspector",
        nullptr,
        [](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Toggle inspector (placeholder)");
        }
    });

    m_CommandRegistry.Register(EditorCommand{
        "View.ToggleWorldDebug",
        "Toggle World Debug",
        nullptr,
        [](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Toggle world debug (placeholder)");
        }
    });

    Logger::Log(LogLevel::Info, "Editor", "Registered editor commands");
}

void EditorApp::DrawMenuBar(float x, float y, float w, float h)
{
    // Draw menu bar background
    static constexpr uint32_t kMenuBarBg = 0x2D2D30FF;
    static constexpr uint32_t kMenuBg    = 0x1B1B1CFF;
    static constexpr uint32_t kMenuHover = 0x3E3E40FF;
    static constexpr uint32_t kTextColor = 0xFFFFFFFF;
    static constexpr uint32_t kSepColor  = 0x444444FF;

    m_UIRenderer.DrawRect({x, y, w, h}, kMenuBarBg);
    m_UIRenderer.DrawRect({x, y + h - 1.f, w, 1.f}, kSepColor);  // Bottom border

    const float dpi = m_UIRenderer.GetDpiScale();
    float menuX = x + 8.f * dpi;
    const float menuY = y + 4.f * dpi;
    const float menuItemW = 70.f * dpi;
    const float menuItemH = h - 8.f * dpi;

    // Track which menu is open (if any)
    static int openMenuIndex = -1;
    static bool menuWasJustOpened = false;

    const auto& menus = m_MenuBar.GetMenus();

    // Draw each top-level menu item
    for (size_t i = 0; i < menus.size(); ++i) {
        const auto& menu = menus[i];
        const bool isHovered = m_Input.mouseX >= menuX && m_Input.mouseX < menuX + menuItemW &&
                               m_Input.mouseY >= menuY && m_Input.mouseY < menuY + menuItemH;

        // Draw menu button
        if (isHovered || static_cast<int>(i) == openMenuIndex) {
            m_UIRenderer.DrawRect({menuX, menuY, menuItemW, menuItemH}, kMenuHover);
        }
        m_UIRenderer.DrawText(menu.label, menuX + 6.f * dpi, menuY + 2.f * dpi, kTextColor, 1.5f);

        // Handle click
        if (isHovered && m_Input.leftJustPressed && !menuWasJustOpened) {
            if (openMenuIndex == static_cast<int>(i)) {
                openMenuIndex = -1;  // Close if clicking the same menu
            } else {
                openMenuIndex = static_cast<int>(i);
                menuWasJustOpened = true;
            }
        }

        // Draw dropdown if this menu is open
        if (openMenuIndex == static_cast<int>(i)) {
            const float dropdownX = menuX;
            const float dropdownY = y + h;
            const float dropdownW = 180.f * dpi;
            const float itemH = 24.f * dpi;
            const float dropdownH = static_cast<float>(menu.items.size()) * itemH;

            // Draw dropdown background
            m_UIRenderer.DrawRect({dropdownX, dropdownY, dropdownW, dropdownH}, kMenuBg);
            m_UIRenderer.DrawOutlineRect({dropdownX, dropdownY, dropdownW, dropdownH}, kSepColor);

            float itemY = dropdownY;
            for (const auto& item : menu.items) {
                const bool itemHovered = m_Input.mouseX >= dropdownX && m_Input.mouseX < dropdownX + dropdownW &&
                                        m_Input.mouseY >= itemY && m_Input.mouseY < itemY + itemH;

                if (itemHovered) {
                    m_UIRenderer.DrawRect({dropdownX, itemY, dropdownW, itemH}, kMenuHover);
                }

                m_UIRenderer.DrawText(item.label, dropdownX + 8.f * dpi, itemY + 4.f * dpi, kTextColor, 1.5f);

                // Handle item click
                if (itemHovered && m_Input.leftJustPressed) {
                    m_MenuBar.Trigger(item.commandId);
                    openMenuIndex = -1;
                }

                itemY += itemH;
            }

            // Close menu if clicking outside
            if (m_Input.leftJustPressed && !menuWasJustOpened) {
                const bool clickedInside = m_Input.mouseX >= dropdownX && m_Input.mouseX < dropdownX + dropdownW &&
                                          m_Input.mouseY >= dropdownY && m_Input.mouseY < dropdownY + dropdownH;
                if (!clickedInside) {
                    openMenuIndex = -1;
                }
            }
        }

        menuX += menuItemW + 4.f * dpi;
    }

    // Reset the "just opened" flag after one frame
    if (menuWasJustOpened && !m_Input.leftDown) {
        menuWasJustOpened = false;
    }
}

void EditorApp::TickFrame(float dt)
{
    m_RenderDevice->BeginFrame();
    m_RenderDevice->Clear(0.18f, 0.18f, 0.18f, 1.f);
    m_Level.Update(dt);

    // ---- Phase 4: render voxel chunk meshes via ForwardRenderer ----
    m_MeshCache.RebuildDirty(m_GameWorld.GetChunkMap());
    {
        Matrix4x4 view = m_Viewport.GetViewMatrix();
        Matrix4x4 proj = m_Viewport.GetProjectionMatrix();
        m_ForwardRenderer.BeginScene(view, proj);
        m_MeshCache.Render();
        m_ForwardRenderer.EndScene();
    }

    // Begin UI rendering pass (2-D overlay on top of the 3-D scene)
    m_UIRenderer.SetViewportSize(static_cast<float>(m_ClientWidth),
                                  static_cast<float>(m_ClientHeight));
    m_UIRenderer.BeginFrame();

    m_DockingSystem.Update(dt);
    m_SceneOutliner.Update(dt);
    m_Inspector.Update(dt);
    m_ContentBrowser.Update(dt);
    m_ConsolePanel.Update(dt);
    m_Viewport.Update(dt);
    m_VoxelInspector.Update(dt);
    m_HUDPanel.Update(dt);
    m_InteractionLoop.Tick(dt);

    // Menu bar at the very top
    const float menuBarH = 24.f * m_DpiScale;
    DrawMenuBar(0.f, 0.f, static_cast<float>(m_ClientWidth), menuBarH);

    // Toolbar strip below menu bar; docking fills the remaining area below.
    const float toolbarH = m_Toolbar.GetHeight() * m_DpiScale;
    const float topBarH = menuBarH + toolbarH;
    m_Toolbar.Draw(0.f, menuBarH, static_cast<float>(m_ClientWidth), toolbarH);
    m_DockingSystem.Draw(0.f, topBarH,
                         static_cast<float>(m_ClientWidth),
                         static_cast<float>(m_ClientHeight) - topBarH);

    // Flush all batched UI draw calls to the GPU
    m_UIRenderer.EndFrame();

    m_RenderDevice->EndFrame();
}

void EditorApp::Run() {
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Run – entering editor loop");

    using Clock = std::chrono::steady_clock;
    auto lastTime = Clock::now();

#ifdef _WIN32
    MSG msg{};
    while (m_Running)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                m_Running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Propagate window resize to the viewport
        {
            RECT rc{};
            if (GetClientRect(static_cast<HWND>(m_Hwnd), &rc))
            {
                int newW = rc.right  - rc.left;
                int newH = rc.bottom - rc.top;
                if (newW != m_ClientWidth || newH != m_ClientHeight)
                {
                    m_ClientWidth  = newW;
                    m_ClientHeight = newH;
                    m_RenderDevice->Resize(m_ClientWidth, m_ClientHeight);
                    m_Viewport.Resize(m_ClientWidth, m_ClientHeight);
                }
            }
        }

        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        TickFrame(dt);

        // Clear edge-triggered input flags after all systems have read them.
        m_Input.FlushFrameEvents();
    }
#else
    while (m_Running) {
        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        TickFrame(dt);
    }
#endif
}

void EditorApp::Shutdown() {
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Shutdown");
    m_MeshCache.Shutdown();
    m_ForwardRenderer.Shutdown();
    m_GameWorld.Shutdown();
    m_Level.Unload();
    m_UIRenderer.Shutdown();
    m_RenderDevice->Shutdown();

#ifdef _WIN32
    if (m_Hwnd && IsWindow(static_cast<HWND>(m_Hwnd)))
        DestroyWindow(static_cast<HWND>(m_Hwnd));
    m_Hwnd = nullptr;
    UnregisterClassW(L"NovaForgeEditor", GetModuleHandleW(nullptr));
#endif
}

} // namespace NF::Editor
