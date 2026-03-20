-- =============================================================================
-- Atlas Engine — Lua API bootstrap
-- =============================================================================
-- This file defines and documents the Lua-side bindings that the C++ engine
-- populates at runtime via sol2 / lua_State. Each table and function listed
-- here is either replaced by a native closure during engine startup or left
-- as a pure-Lua stub so scripts can run in isolation for unit testing.
-- =============================================================================

-- ---------------------------------------------------------------------------
-- Engine — core engine controls
-- ---------------------------------------------------------------------------
Engine = Engine or {}

Engine.deltaTime  = 0.0   -- seconds since last frame (set by C++ each tick)
Engine.frameCount = 0     -- total frames elapsed (set by C++ each tick)

--- Log an informational message to the engine console.
function Engine.log(msg)
    print("[LOG] " .. tostring(msg))
end

--- Log a warning message to the engine console.
function Engine.warn(msg)
    print("[WARN] " .. tostring(msg))
end

--- Log an error message to the engine console.
function Engine.error(msg)
    print("[ERROR] " .. tostring(msg))
end

-- Sub-tables populated by C++ bindings at startup
Engine.Window  = Engine.Window  or {}
Engine.Input   = Engine.Input   or {}
Engine.Render  = Engine.Render  or {}
Engine.Audio   = Engine.Audio   or {}
Engine.Physics = Engine.Physics or {}

--- Query current window dimensions. Returns width, height in pixels.
function Engine.Window.getSize()
    return 1920, 1080   -- stub; overridden by C++
end

--- Set the window title string.
function Engine.Window.setTitle(title)
    -- stub; overridden by C++
end

--- Returns true while a keyboard key is held (key = SDL scancode string).
function Engine.Input.isKeyDown(key)
    return false   -- stub
end

--- Returns true for the single frame a key is first pressed.
function Engine.Input.isKeyPressed(key)
    return false   -- stub
end

--- Returns mouse position as x, y in screen pixels.
function Engine.Input.getMousePos()
    return 0, 0   -- stub
end

--- Returns true while the specified mouse button is held (1=left,2=right,3=mid).
function Engine.Input.isMouseDown(button)
    return false   -- stub
end

--- Submit a mesh for rendering this frame.
--- @param meshId   integer   handle from Engine.Render.loadMesh()
--- @param transform table    {x,y,z, rx,ry,rz, sx,sy,sz}
function Engine.Render.drawMesh(meshId, transform)
    -- stub; overridden by C++
end

--- Load a mesh from a file path; returns an integer mesh handle.
function Engine.Render.loadMesh(path)
    return 0   -- stub
end

--- Play a one-shot sound by asset path.
function Engine.Audio.playSound(path, volume)
    -- stub; overridden by C++
end

--- Apply a physics impulse to a body (bodyId from Runtime ECS).
function Engine.Physics.applyImpulse(bodyId, ix, iy, iz)
    -- stub; overridden by C++
end

-- ---------------------------------------------------------------------------
-- Runtime — ECS entity/component management
-- ---------------------------------------------------------------------------
Runtime = Runtime or {}

--- Spawn a new entity with the given display name.
--- Returns the entity ID (integer) or nil on failure.
function Runtime.spawnEntity(name)
    Engine.log("Runtime.spawnEntity: " .. tostring(name))
    return nil   -- stub; returns uint32_t entity ID from C++
end

--- Destroy an entity and all its components.
function Runtime.destroyEntity(id)
    Engine.log("Runtime.destroyEntity: " .. tostring(id))
    -- stub; overridden by C++
end

--- Retrieve a component table for the given entity and type name.
--- Returns a table (copy of component data) or nil if not present.
function Runtime.getComponent(entityId, typeName)
    return nil   -- stub; overridden by C++
end

--- Set or replace a component on an entity.
--- @param data table   fields matching the C++ component struct
function Runtime.setComponent(entityId, typeName, data)
    -- stub; overridden by C++
end

-- ---------------------------------------------------------------------------
-- AI — inference and memory
-- ---------------------------------------------------------------------------
AI = AI or {}

--- Blocking inference call to the active AI model.
--- Returns the completion string, or empty string on failure.
function AI.prompt(text, maxTokens)
    Engine.warn("AI.prompt: no model loaded (stub)")
    return ""   -- stub; overridden by C++
end

--- Store a key-value pair in the AI agent's persistent memory.
function AI.remember(key, value)
    -- stub; overridden by C++
end

--- Retrieve a previously stored memory value by key.
--- Returns the value or nil if not found.
function AI.recall(key)
    return nil   -- stub; overridden by C++
end

-- ---------------------------------------------------------------------------
-- Builder — in-game part placement
-- ---------------------------------------------------------------------------
Builder = Builder or {}

--- Place a part definition by defId at world position (x, y, z).
--- Returns the new instance ID (integer) or nil on failure.
function Builder.placePart(defId, x, y, z)
    Engine.log(string.format("Builder.placePart: def=%d @ (%.2f,%.2f,%.2f)", defId, x, y, z))
    return nil   -- stub; overridden by C++
end

--- Remove a placed part instance.
function Builder.removePart(instanceId)
    Engine.log("Builder.removePart: " .. tostring(instanceId))
    -- stub; overridden by C++
end

--- Weld two snap points together.
--- @param instA   integer   instance ID of first part
--- @param snapA   integer   snap-point ID on first part
--- @param instB   integer   instance ID of second part
--- @param snapB   integer   snap-point ID on second part
--- Returns true on success.
function Builder.weld(instA, snapA, instB, snapB)
    return false   -- stub; overridden by C++
end

-- ---------------------------------------------------------------------------
-- PCG — procedural content generation
-- ---------------------------------------------------------------------------
PCG = PCG or {}

--- Generate a full world (terrain, structures) from a seed integer.
--- Returns a world handle (integer) or nil on failure.
function PCG.generateWorld(seed)
    Engine.log("PCG.generateWorld: seed=" .. tostring(seed))
    return nil   -- stub; overridden by C++
end

--- Generate a mesh using a named grammar rule and seed.
--- Returns a mesh handle (integer) or nil on failure.
function PCG.generateMesh(ruleName, seed)
    Engine.log("PCG.generateMesh: rule=" .. tostring(ruleName) .. " seed=" .. tostring(seed))
    return nil   -- stub; overridden by C++
end

-- ---------------------------------------------------------------------------
-- Module export (for require()-based loading)
-- ---------------------------------------------------------------------------
return {
    Engine  = Engine,
    Runtime = Runtime,
    AI      = AI,
    Builder = Builder,
    PCG     = PCG,
}
