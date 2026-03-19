#include "MenuManager.h"
#include "UIConstants.h"
#include <algorithm>

namespace atlas::ui {

// Common key codes used for menu keyboard navigation.
static constexpr uint32_t kKeyUp    = 0x26;  // VK_UP / SDLK_UP
static constexpr uint32_t kKeyDown  = 0x28;  // VK_DOWN / SDLK_DOWN
static constexpr uint32_t kKeyLeft  = 0x25;  // VK_LEFT / SDLK_LEFT
static constexpr uint32_t kKeyRight = 0x27;  // VK_RIGHT / SDLK_RIGHT
static constexpr uint32_t kKeyEnter = 0x0D;  // VK_RETURN / SDLK_RETURN
static constexpr uint32_t kKeyEsc   = 0x1B;  // VK_ESCAPE / SDLK_ESCAPE

void MenuManager::Init(UIScreen* screen) {
    m_screen = screen;
    m_openMenuId = 0;
    m_focusedItemId = 0;
    m_contextMenuId = 0;
    m_openSubmenuId = 0;
}

bool MenuManager::IsPointInWidget(const UIWidget* widget, int32_t x, int32_t y) const {
    if (!widget) return false;
    return x >= widget->x && x < widget->x + widget->width &&
           y >= widget->y && y < widget->y + widget->height;
}

bool MenuManager::IsAnyMenuOpen() const {
    return m_openMenuId != 0 || m_contextMenuId != 0;
}

uint32_t MenuManager::GetOpenMenuId() const {
    return m_openMenuId;
}

uint32_t MenuManager::GetFocusedItemId() const {
    return m_focusedItemId;
}

uint32_t MenuManager::GetContextMenuId() const {
    return m_contextMenuId;
}

bool MenuManager::IsContextMenuOpen() const {
    return m_contextMenuId != 0;
}

uint32_t MenuManager::GetOpenSubmenuId() const {
    return m_openSubmenuId;
}

std::vector<uint32_t> MenuManager::GetSelectableItems(uint32_t menuId) const {
    std::vector<uint32_t> items;
    if (!m_screen || menuId == 0) return items;

    for (uint32_t i = 1; i < kMaxWidgetId; ++i) {
        const UIWidget* widget = m_screen->GetWidget(i);
        if (!widget) continue;
        if (widget->type != UIWidgetType::MenuItem) continue;
        if (widget->parentId != menuId) continue;
        if (widget->isSeparator) continue;
        if (widget->isDisabled) continue;
        items.push_back(i);
    }
    return items;
}

void MenuManager::MoveFocus(int direction) {
    uint32_t activeMenu = m_openSubmenuId ? m_openSubmenuId : (m_openMenuId ? m_openMenuId : m_contextMenuId);
    if (activeMenu == 0) return;

    auto items = GetSelectableItems(activeMenu);
    if (items.empty()) return;

    if (m_focusedItemId == 0) {
        // No current focus — pick first or last depending on direction
        m_focusedItemId = (direction > 0) ? items.front() : items.back();
    } else {
        auto it = std::find(items.begin(), items.end(), m_focusedItemId);
        if (it == items.end()) {
            m_focusedItemId = items.front();
        } else {
            if (direction > 0) {
                ++it;
                m_focusedItemId = (it == items.end()) ? items.front() : *it;
            } else {
                if (it == items.begin()) {
                    m_focusedItemId = items.back();
                } else {
                    --it;
                    m_focusedItemId = *it;
                }
            }
        }
    }

    // Update hover state to follow keyboard focus
    for (uint32_t i = 1; i < kMaxWidgetId; ++i) {
        UIWidget* w = m_screen->GetWidgetMutable(i);
        if (!w || w->type != UIWidgetType::MenuItem) continue;
        if (w->parentId == activeMenu) {
            w->isHovered = (i == m_focusedItemId);
        }
    }
}

void MenuManager::SelectFocusedItem() {
    if (m_focusedItemId == 0) return;

    const UIWidget* item = m_screen->GetWidget(m_focusedItemId);
    if (!item || item->isDisabled || item->isSeparator) return;

    // If the focused item has a submenu, open it instead
    if (item->hasSubmenu) {
        OpenSubmenu(m_focusedItemId);
        return;
    }

    uint32_t menuId = m_openSubmenuId ? m_openSubmenuId : (m_openMenuId ? m_openMenuId : m_contextMenuId);

    // Toggle checkable items
    if (item->isCheckable) {
        UIWidget* mutableItem = m_screen->GetWidgetMutable(m_focusedItemId);
        if (mutableItem) {
            mutableItem->isChecked = !mutableItem->isChecked;
        }
    }

    if (m_menuItemCallback) {
        m_menuItemCallback(menuId, m_focusedItemId);
    }

    CloseAllMenus();
}

void MenuManager::OpenSubmenu(uint32_t itemId) {
    if (!m_screen) return;

    const UIWidget* item = m_screen->GetWidget(itemId);
    if (!item || !item->hasSubmenu) return;

    // Close any previously open submenu
    if (m_openSubmenuId != 0 && m_openSubmenuId != itemId) {
        if (UIWidget* old = m_screen->GetWidgetMutable(m_openSubmenuId)) {
            old->isMenuOpen = false;
        }
    }

    // The submenu's children are parented to this item's ID
    // Mark the item as having its submenu open
    if (UIWidget* mutableItem = m_screen->GetWidgetMutable(itemId)) {
        mutableItem->isMenuOpen = true;
    }
    m_openSubmenuId = itemId;
    m_focusedItemId = 0; // Reset focus within submenu
}

void MenuManager::CloseSubmenu() {
    if (m_openSubmenuId == 0) return;
    if (UIWidget* item = m_screen->GetWidgetMutable(m_openSubmenuId)) {
        item->isMenuOpen = false;
    }
    m_openSubmenuId = 0;
}

bool MenuManager::HandleKeyEvent(const UIEvent& event) {
    if (event.type != UIEvent::Type::KeyDown) return false;
    if (!IsAnyMenuOpen()) return false;

    switch (event.keyCode) {
        case kKeyDown:
            MoveFocus(1);
            return true;
        case kKeyUp:
            MoveFocus(-1);
            return true;
        case kKeyEnter:
            SelectFocusedItem();
            return true;
        case kKeyEsc:
            if (m_openSubmenuId != 0) {
                CloseSubmenu();
                // Restore focus to the parent menu item
            } else {
                CloseAllMenus();
            }
            return true;
        case kKeyRight: {
            // If focused item has submenu, open it
            if (m_focusedItemId != 0) {
                const UIWidget* item = m_screen->GetWidget(m_focusedItemId);
                if (item && item->hasSubmenu) {
                    OpenSubmenu(m_focusedItemId);
                    MoveFocus(1); // Focus first item in submenu
                    return true;
                }
            }
            // Otherwise switch to next menu in menu bar
            if (m_openMenuId != 0) {
                // Collect all top-level Menu widgets in order
                std::vector<uint32_t> menus;
                for (uint32_t i = 1; i < kMaxWidgetId; ++i) {
                    const UIWidget* w = m_screen->GetWidget(i);
                    if (w && w->type == UIWidgetType::Menu) {
                        menus.push_back(i);
                    }
                }
                if (menus.size() > 1) {
                    auto it = std::find(menus.begin(), menus.end(), m_openMenuId);
                    if (it != menus.end()) {
                        ++it;
                        uint32_t nextMenu = (it == menus.end()) ? menus.front() : *it;
                        if (UIWidget* old = m_screen->GetWidgetMutable(m_openMenuId)) {
                            old->isMenuOpen = false;
                        }
                        CloseSubmenu();
                        m_openMenuId = nextMenu;
                        if (UIWidget* w = m_screen->GetWidgetMutable(nextMenu)) {
                            w->isMenuOpen = true;
                        }
                        m_focusedItemId = 0;
                        MoveFocus(1);
                    }
                }
            }
            return true;
        }
        case kKeyLeft: {
            // If in a submenu, close it and go back to parent
            if (m_openSubmenuId != 0) {
                uint32_t parentItem = m_openSubmenuId;
                CloseSubmenu();
                m_focusedItemId = parentItem;
                return true;
            }
            // Otherwise switch to previous menu in menu bar
            if (m_openMenuId != 0) {
                std::vector<uint32_t> menus;
                for (uint32_t i = 1; i < kMaxWidgetId; ++i) {
                    const UIWidget* w = m_screen->GetWidget(i);
                    if (w && w->type == UIWidgetType::Menu) {
                        menus.push_back(i);
                    }
                }
                if (menus.size() > 1) {
                    auto it = std::find(menus.begin(), menus.end(), m_openMenuId);
                    if (it != menus.end()) {
                        uint32_t prevMenu = (it == menus.begin()) ? menus.back() : *(it - 1);
                        if (UIWidget* old = m_screen->GetWidgetMutable(m_openMenuId)) {
                            old->isMenuOpen = false;
                        }
                        CloseSubmenu();
                        m_openMenuId = prevMenu;
                        if (UIWidget* w = m_screen->GetWidgetMutable(prevMenu)) {
                            w->isMenuOpen = true;
                        }
                        m_focusedItemId = 0;
                        MoveFocus(1);
                    }
                }
            }
            return true;
        }
        default:
            break;
    }
    return false;
}

bool MenuManager::HandleEvent(const UIEvent& event) {
    if (!m_screen) return false;

    // Handle keyboard events for navigation
    if (event.type == UIEvent::Type::KeyDown || event.type == UIEvent::Type::KeyUp) {
        return HandleKeyEvent(event);
    }

    if (event.type == UIEvent::Type::MouseMove) {
        // Update hover states for all menus and menu items
        for (uint32_t i = 1; i < kMaxWidgetId; ++i) {  // Start at 1 (0 is root)
            UIWidget* widget = m_screen->GetWidgetMutable(i);
            if (!widget) continue;

            if (widget->type == UIWidgetType::Menu) {
                bool wasHovered = widget->isHovered;
                widget->isHovered = IsPointInWidget(widget, event.x, event.y);
                
                // If we hover over a different menu while one is open, switch to it
                if (widget->isHovered && !wasHovered && m_openMenuId != 0 && m_openMenuId != i) {
                    if (UIWidget* oldMenu = m_screen->GetWidgetMutable(m_openMenuId)) {
                        oldMenu->isMenuOpen = false;
                    }
                    CloseSubmenu();
                    m_openMenuId = i;
                    widget->isMenuOpen = true;
                    m_focusedItemId = 0;
                }
            } else if (widget->type == UIWidgetType::MenuItem) {
                // Update hover for items in the currently open dropdown menu
                uint32_t activeMenu = m_openMenuId ? m_openMenuId : m_contextMenuId;
                bool inActiveMenu = (widget->parentId == activeMenu && activeMenu != 0);
                bool inSubmenu = (widget->parentId == m_openSubmenuId && m_openSubmenuId != 0);

                if (inActiveMenu || inSubmenu) {
                    bool nowHovered = IsPointInWidget(widget, event.x, event.y);
                    widget->isHovered = nowHovered;
                    if (nowHovered && !widget->isDisabled && !widget->isSeparator) {
                        m_focusedItemId = i;
                        // If hovering an item with submenu, open it
                        if (widget->hasSubmenu && !widget->isMenuOpen) {
                            OpenSubmenu(i);
                        }
                    }
                } else {
                    widget->isHovered = false;
                }
            }
        }
        return false; // Don't consume mouse move events
    }

    // Handle right-click for context menus
    if (event.type == UIEvent::Type::MouseDown && event.mouseButton == 1) {
        // Right-click: if a context menu is open, close it; otherwise, don't consume
        if (m_contextMenuId != 0) {
            CloseContextMenu();
            return true;
        }
        return false;
    }

    if (event.type == UIEvent::Type::MouseDown && event.mouseButton == 0) {
        // Check if clicking on a menu button
        bool clickedMenu = false;
        for (uint32_t i = 1; i < kMaxWidgetId; ++i) {
            UIWidget* widget = m_screen->GetWidgetMutable(i);
            if (!widget || widget->type != UIWidgetType::Menu) continue;

            if (IsPointInWidget(widget, event.x, event.y)) {
                clickedMenu = true;
                if (m_openMenuId == i) {
                    // Clicking same menu: close it
                    widget->isMenuOpen = false;
                    m_openMenuId = 0;
                    m_focusedItemId = 0;
                    CloseSubmenu();
                } else {
                    // Clicking different menu: close old, open new
                    if (m_openMenuId != 0) {
                        if (UIWidget* oldMenu = m_screen->GetWidgetMutable(m_openMenuId)) {
                            oldMenu->isMenuOpen = false;
                        }
                    }
                    CloseSubmenu();
                    CloseContextMenu();
                    widget->isMenuOpen = true;
                    m_openMenuId = i;
                    m_focusedItemId = 0;
                }
                return true; // Consumed
            }
        }

        // Check if clicking on a menu item in the open menu or submenu
        uint32_t activeMenu = m_openSubmenuId ? m_openSubmenuId : (m_openMenuId ? m_openMenuId : m_contextMenuId);
        if (activeMenu != 0) {
            // Check in submenu first, then in main menu
            for (uint32_t checkMenu : {m_openSubmenuId, m_openMenuId, m_contextMenuId}) {
                if (checkMenu == 0) continue;
                for (uint32_t i = 1; i < kMaxWidgetId; ++i) {
                    const UIWidget* widget = m_screen->GetWidget(i);
                    if (!widget || widget->type != UIWidgetType::MenuItem) continue;
                    if (widget->parentId != checkMenu) continue;
                    if (widget->isSeparator) continue;
                    if (widget->isDisabled) continue;

                    if (IsPointInWidget(widget, event.x, event.y)) {
                        // If the item has a submenu, open it
                        if (widget->hasSubmenu) {
                            OpenSubmenu(i);
                            return true;
                        }

                        // Toggle checkable items
                        UIWidget* mutableWidget = m_screen->GetWidgetMutable(i);
                        if (mutableWidget && mutableWidget->isCheckable) {
                            mutableWidget->isChecked = !mutableWidget->isChecked;
                        }

                        // Invoke callback
                        if (m_menuItemCallback) {
                            m_menuItemCallback(checkMenu, i);
                        }
                        
                        // Close all menus
                        CloseAllMenus();
                        return true; // Consumed
                    }
                }
            }
        }

        // Click outside any menu: close open menus
        if (!clickedMenu && (m_openMenuId != 0 || m_contextMenuId != 0)) {
            // Check if click is inside any dropdown/context area
            bool inDropdown = false;
            uint32_t checkMenus[] = {m_openMenuId, m_contextMenuId, m_openSubmenuId};
            for (uint32_t menuId : checkMenus) {
                if (menuId == 0) continue;
                for (uint32_t i = 1; i < kMaxWidgetId; ++i) {
                    const UIWidget* widget = m_screen->GetWidget(i);
                    if (!widget || widget->type != UIWidgetType::MenuItem) continue;
                    if (widget->parentId != menuId) continue;

                    if (IsPointInWidget(widget, event.x, event.y)) {
                        inDropdown = true;
                        break;
                    }
                }
                if (inDropdown) break;
            }

            if (!inDropdown) {
                CloseAllMenus();
                return false; // Don't consume - let click through
            }
        }
    }

    return false;
}

void MenuManager::Update(int32_t mouseX, int32_t mouseY) {
    if (!m_screen) return;

    // Update hover states based on mouse position
    UIEvent moveEvent;
    moveEvent.type = UIEvent::Type::MouseMove;
    moveEvent.x = mouseX;
    moveEvent.y = mouseY;
    HandleEvent(moveEvent);
}

void MenuManager::CloseAllMenus() {
    if (!m_screen) return;

    CloseSubmenu();

    if (m_openMenuId != 0) {
        if (UIWidget* menu = m_screen->GetWidgetMutable(m_openMenuId)) {
            menu->isMenuOpen = false;
        }
        m_openMenuId = 0;
    }

    CloseContextMenu();

    m_focusedItemId = 0;
}

void MenuManager::SetMenuItemCallback(MenuItemCallback callback) {
    m_menuItemCallback = std::move(callback);
}

void MenuManager::OpenContextMenu(uint32_t contextMenuId, int32_t x, int32_t y) {
    if (!m_screen) return;

    // Close any existing menus
    CloseAllMenus();

    UIWidget* menu = m_screen->GetWidgetMutable(contextMenuId);
    if (!menu) return;

    // Position the context menu at the click point
    menu->x = static_cast<float>(x);
    menu->y = static_cast<float>(y);
    menu->isMenuOpen = true;
    menu->visible = true;
    m_contextMenuId = contextMenuId;
    m_focusedItemId = 0;

    // Position child items relative to the context menu
    float itemY = static_cast<float>(y);
    for (uint32_t i = 1; i < kMaxWidgetId; ++i) {
        UIWidget* item = m_screen->GetWidgetMutable(i);
        if (!item || item->type != UIWidgetType::MenuItem) continue;
        if (item->parentId != contextMenuId) continue;

        item->x = static_cast<float>(x);
        item->y = itemY;
        item->visible = true;
        itemY += item->height;
    }
}

void MenuManager::CloseContextMenu() {
    if (m_contextMenuId == 0 || !m_screen) return;

    if (UIWidget* menu = m_screen->GetWidgetMutable(m_contextMenuId)) {
        menu->isMenuOpen = false;
        menu->visible = false;
    }

    // Hide child items
    for (uint32_t i = 1; i < kMaxWidgetId; ++i) {
        UIWidget* item = m_screen->GetWidgetMutable(i);
        if (!item || item->type != UIWidgetType::MenuItem) continue;
        if (item->parentId != m_contextMenuId) continue;
        item->visible = false;
    }

    m_contextMenuId = 0;
}

} // namespace atlas::ui
