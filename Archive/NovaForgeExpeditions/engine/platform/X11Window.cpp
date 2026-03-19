#include "X11Window.h"
#include "../core/Logger.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include <cstring>

namespace atlas::platform {

/// Translate an X11 KeySym to a platform-independent key code.
static uint32_t TranslateKeySym(unsigned long sym) {
    if (sym == XK_F3) return kKeyF3;
    return static_cast<uint32_t>(sym);
}

/// Translate X11 1-based button index to 0-based platform button.
/// X11: 1=Left, 2=Middle, 3=Right.  Platform: 0=Left, 1=Middle, 2=Right.
/// Buttons outside 1-3 (e.g. high-number buttons) pass through unchanged.
static uint8_t TranslateButton(unsigned int xButton) {
    if (xButton >= 1 && xButton <= 3)
        return static_cast<uint8_t>(xButton - 1);
    return static_cast<uint8_t>(xButton);
}

X11Window::~X11Window() {
    Shutdown();
}

bool X11Window::Init(const PlatformWindowConfig& config) {
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        atlas::Logger::Error("X11: Failed to open display");
        return false;
    }

    int screen = DefaultScreen(m_display);
    ::Window rootWin = RootWindow(m_display, screen);

    int glxAttribs[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    };

    XVisualInfo* vi = glXChooseVisual(m_display, screen, glxAttribs);
    if (!vi) {
        atlas::Logger::Error("X11: No suitable GLX visual found");
        XCloseDisplay(m_display);
        m_display = nullptr;
        return false;
    }

    Colormap cmap = XCreateColormap(m_display, rootWin, vi->visual, AllocNone);

    XSetWindowAttributes swa;
    std::memset(&swa, 0, sizeof(swa));
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                     ButtonPressMask | ButtonReleaseMask |
                     PointerMotionMask | StructureNotifyMask;

    m_window = XCreateWindow(
        m_display, rootWin,
        0, 0,
        static_cast<unsigned int>(config.width),
        static_cast<unsigned int>(config.height),
        0,
        vi->depth,
        InputOutput,
        vi->visual,
        CWColormap | CWEventMask,
        &swa
    );

    XStoreName(m_display, m_window, config.title.c_str());

    Atom wmDelete = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    m_deleteAtom = wmDelete;
    XSetWMProtocols(m_display, m_window, &wmDelete, 1);

    XMapWindow(m_display, m_window);

    m_glContext = glXCreateContext(m_display, vi, nullptr, GL_TRUE);
    if (!m_glContext) {
        atlas::Logger::Error("X11: Failed to create GLX context");
        XFree(vi);
        XDestroyWindow(m_display, m_window);
        XFreeColormap(m_display, cmap);
        XCloseDisplay(m_display);
        m_display = nullptr;
        return false;
    }

    glXMakeCurrent(m_display, m_window, m_glContext);

    XFree(vi);

    m_width = config.width;
    m_height = config.height;
    m_open = true;

    atlas::Logger::Info("X11 window created (" + std::to_string(m_width) + "x" + std::to_string(m_height) + ")");
    return true;
}

void X11Window::Shutdown() {
    if (m_glContext) {
        glXMakeCurrent(m_display, None, nullptr);
        glXDestroyContext(m_display, m_glContext);
        m_glContext = nullptr;
    }
    if (m_window && m_display) {
        XDestroyWindow(m_display, m_window);
        m_window = 0;
    }
    if (m_display) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
    m_open = false;
}

bool X11Window::PollEvent(WindowEvent& event) {
    if (!m_display || !m_open) return false;

    while (XPending(m_display)) {
        XEvent xev;
        XNextEvent(m_display, &xev);

        switch (xev.type) {
            case ClientMessage: {
                if (static_cast<unsigned long>(xev.xclient.data.l[0]) == m_deleteAtom) {
                    event.type = WindowEvent::Type::Close;
                    m_open = false;
                    return true;
                }
                break;
            }
            case ConfigureNotify: {
                if (xev.xconfigure.width != m_width || xev.xconfigure.height != m_height) {
                    m_width = xev.xconfigure.width;
                    m_height = xev.xconfigure.height;
                    event.type = WindowEvent::Type::Resize;
                    event.width = m_width;
                    event.height = m_height;
                    return true;
                }
                break;
            }
            case KeyPress: {
                event.type = WindowEvent::Type::KeyDown;
                KeySym sym = XLookupKeysym(&xev.xkey, 0);
                event.keyCode = TranslateKeySym(sym);
                event.modifiers = 0;
                if (xev.xkey.state & ControlMask) event.modifiers |= kModCtrl;
                if (xev.xkey.state & ShiftMask)   event.modifiers |= kModShift;
                if (xev.xkey.state & Mod1Mask)    event.modifiers |= kModAlt;
                char buf[32] = {};
                int len = XLookupString(&xev.xkey, buf, sizeof(buf) - 1, nullptr, nullptr);
                if (len > 0) {
                    event.textChar = buf[0];
                }
                return true;
            }
            case KeyRelease: {
                event.type = WindowEvent::Type::KeyUp;
                KeySym sym = XLookupKeysym(&xev.xkey, 0);
                event.keyCode = TranslateKeySym(sym);
                event.modifiers = 0;
                if (xev.xkey.state & ControlMask) event.modifiers |= kModCtrl;
                if (xev.xkey.state & ShiftMask)   event.modifiers |= kModShift;
                if (xev.xkey.state & Mod1Mask)    event.modifiers |= kModAlt;
                return true;
            }
            case ButtonPress: {
                unsigned int btn = xev.xbutton.button;
                // X11 buttons 4/5 are scroll wheel:
                // 4 = up (negative delta), 5 = down (positive delta).
                if (btn == 4 || btn == 5) {
                    event.type = WindowEvent::Type::ScrollWheel;
                    event.mouseX = xev.xbutton.x;
                    event.mouseY = xev.xbutton.y;
                    event.scrollDelta = (btn == 5) ? 1.0f : -1.0f;
                    return true;
                }
                event.type = WindowEvent::Type::MouseButtonDown;
                event.mouseButton = TranslateButton(btn);
                event.mouseX = xev.xbutton.x;
                event.mouseY = xev.xbutton.y;
                return true;
            }
            case ButtonRelease: {
                unsigned int btn = xev.xbutton.button;
                // Ignore scroll wheel release events
                if (btn == 4 || btn == 5) break;
                event.type = WindowEvent::Type::MouseButtonUp;
                event.mouseButton = TranslateButton(btn);
                event.mouseX = xev.xbutton.x;
                event.mouseY = xev.xbutton.y;
                return true;
            }
            case MotionNotify: {
                event.type = WindowEvent::Type::MouseMove;
                event.mouseX = xev.xmotion.x;
                event.mouseY = xev.xmotion.y;
                return true;
            }
            default:
                break;
        }
    }

    event.type = WindowEvent::Type::NoEvent;
    return false;
}

void X11Window::SwapBuffers() {
    if (m_display && m_window) {
        glXSwapBuffers(m_display, m_window);
    }
}

} // namespace atlas::platform
