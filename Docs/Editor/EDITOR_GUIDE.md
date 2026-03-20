# Editor Guide

This guide covers the Atlas Editor — the primary development environment for
the game engine. The editor uses a fully custom UI (no ImGui) built on the
`UI::` widget framework.

---

## Panel System and Docking

The editor uses a **docking system** (`Editor::DockingSystem`) that allows
panels to be attached to any screen edge or floated as independent windows.

- **Dock a panel:** drag its title bar to a dock zone indicator (edge or center).
- **Float a panel:** drag it away from all dock zones.
- **Close a panel:** click the ✕ button in the title bar.
- **Reopen a panel:** use the **View** menu or keyboard shortcut.
- **Save layout:** `File → Save Layout` persists the current panel arrangement.

Registered panels:

| Panel | Default Location | Shortcut |
|-------|-----------------|---------|
| Scene Outliner | Left dock | `Ctrl+1` |
| Inspector | Right dock | `Ctrl+2` |
| Content Browser | Bottom dock | `Ctrl+3` |
| Console | Bottom dock (tabbed) | `` Ctrl+` `` |
| Viewport | Center | — |
| Profiler | Bottom dock (tabbed) | `Ctrl+P` |
| Node Editor | Floating | `Ctrl+N` |
| Builder Editor | Floating | `Ctrl+B` |

---

## Viewport Controls

| Action | Input |
|--------|-------|
| Orbit camera | Right-click + drag |
| Pan camera | Middle-click + drag |
| Zoom | Scroll wheel |
| Focus selected | `F` |
| Toggle wireframe | `Alt+W` |
| Toggle grid | `G` |
| Perspective / Orthographic | `Numpad 5` |
| Front view | `Numpad 1` |
| Side view | `Numpad 3` |
| Top view | `Numpad 7` |

---

## Gizmo Shortcuts

| Key | Mode |
|-----|------|
| `W` | Translate |
| `E` | Rotate |
| `R` | Scale |
| `Q` | Select / pointer |
| `X` / `Y` / `Z` | Constrain to axis (while dragging) |
| `Esc` | Cancel current gizmo operation |

---

## Scene Outliner

The Scene Outliner displays the entity hierarchy for the current scene.

- **Select entity:** single click
- **Rename entity:** double click the name
- **Add entity:** click the `+` button or `Ctrl+Shift+N`
- **Delete entity:** `Delete` key (archives, does not destroy permanently)
- **Reparent:** drag entity onto new parent
- **Toggle visibility:** click the eye icon
- **Filter by name:** type in the search bar at the top
- **Layer / tag filter:** use the dropdown next to the search bar

---

## Inspector Panel

The Inspector shows the components and properties of the currently selected
entity.

- **Edit a field:** click the value and type a new value; press `Enter` or
  click elsewhere to confirm.
- **Add component:** click `Add Component` and choose from the dropdown.
- **Remove component:** click the `⋮` menu next to the component header.
- **Reset field to default:** right-click the field label.
- **Material override:** select an entity with a mesh, then drag a material
  asset from the Content Browser onto the material slot.

---

## Content Browser

Displays all project assets organised by directory.

- **Navigate:** double-click a folder to enter it; breadcrumb trail at top.
- **Search:** type in the search bar to filter by name.
- **Import asset:** drag a file from the OS file manager onto the panel.
- **Create asset:** right-click → New → (Material / Blueprint / Script …).
- **Open asset:** double-click (opens in appropriate editor).
- **Drag to scene:** drag a mesh asset from the browser to the viewport to
  instantiate it.

---

## Console Panel

The Console panel displays engine log output and accepts commands.

| Command | Description |
|---------|-------------|
| `help` | List available commands |
| `clear` | Clear the console output |
| `spawn <name>` | Spawn a named entity in the current scene |
| `destroy <id>` | Destroy entity by ID |
| `reload scene` | Reload the current scene from disk |
| `set <var> <val>` | Set a CVar (console variable) |
| `get <var>` | Query a CVar value |
| `exec <lua>` | Execute a Lua snippet inline |

Log level filter buttons (Info / Warn / Error / All) appear in the toolbar.

---

## Builder Editor Workflow

1. Open the **Builder Editor** panel (`Ctrl+B`).
2. Select a **Part** from the Part Library panel (left side).
3. Click a **snap point** on the existing assembly (highlighted in blue) to
   attach the new part. The part previews in green when the snap is valid.
4. Press `Enter` to confirm placement or `Esc` to cancel.
5. Use the **Weld Tool** (`W` in builder mode) to manually connect two snap
   points.
6. Use the **Validate Assembly** button to run the integrity check. Errors
   appear highlighted in red.
7. Press `Play` to hand the assembly to the Runtime for physics simulation.
