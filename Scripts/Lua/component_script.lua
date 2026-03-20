-- =============================================================================
-- component_script.lua — Example: Patrol component
-- =============================================================================
-- Demonstrates a Lua component that uses the Atlas Engine API to move an
-- entity along a list of world-space waypoints at a constant speed.
-- =============================================================================

local api = require("Scripts/Lua/engine_api")
local Engine  = api.Engine
local Runtime = api.Runtime

-- ---------------------------------------------------------------------------
-- PatrolComponent
-- ---------------------------------------------------------------------------

local PatrolComponent = {}
PatrolComponent.__index = PatrolComponent

--- Constructor.
--- @param entityId  integer        The ECS entity this component drives.
--- @param waypoints table          Array of {x, y, z} world positions.
--- @param speed     number         Movement speed in units/second.
--- @return PatrolComponent
function PatrolComponent.new(entityId, waypoints, speed)
    local self = setmetatable({}, PatrolComponent)
    self.entityId     = entityId
    self.waypoints    = waypoints or {}
    self.speed        = speed or 5.0
    self.currentTarget = 1   -- index into waypoints
    self._active      = false
    return self
end

-- ---------------------------------------------------------------------------
-- Internal helpers
-- ---------------------------------------------------------------------------

local function vecDistance(a, b)
    local dx = b.x - a.x
    local dy = b.y - a.y
    local dz = b.z - a.z
    return math.sqrt(dx*dx + dy*dy + dz*dz)
end

local function vecNormalize(dx, dy, dz)
    local len = math.sqrt(dx*dx + dy*dy + dz*dz)
    if len < 1e-6 then return 0, 0, 0 end
    return dx/len, dy/len, dz/len
end

-- ---------------------------------------------------------------------------
-- Lifecycle
-- ---------------------------------------------------------------------------

--- Called when the entity is spawned into the world.
function PatrolComponent:onSpawn()
    self._active = true
    Engine.log(string.format("[Patrol] entity %d spawned with %d waypoints",
        self.entityId, #self.waypoints))
end

--- Called when the entity is removed from the world.
function PatrolComponent:onDespawn()
    self._active = false
    Engine.log(string.format("[Patrol] entity %d despawned", self.entityId))
end

-- ---------------------------------------------------------------------------
-- Per-frame tick
-- ---------------------------------------------------------------------------

--- Update patrol movement.
--- @param dt  number  Delta time in seconds.
function PatrolComponent:tick(dt)
    if not self._active then return end
    if #self.waypoints == 0 then return end

    -- Fetch current transform from ECS
    local transform = Runtime.getComponent(self.entityId, "Transform")
    if not transform then return end

    local pos = { x = transform.x or 0, y = transform.y or 0, z = transform.z or 0 }
    local target = self.waypoints[self.currentTarget]
    if not target then return end

    local dist = vecDistance(pos, target)
    local stepDist = self.speed * dt

    if dist <= stepDist then
        -- Arrived at waypoint — snap to it and advance
        transform.x = target.x
        transform.y = target.y
        transform.z = target.z
        Runtime.setComponent(self.entityId, "Transform", transform)

        local prev = self.currentTarget
        self.currentTarget = (self.currentTarget % #self.waypoints) + 1
        Engine.log(string.format("[Patrol] entity %d reached waypoint %d → next: %d",
            self.entityId, prev, self.currentTarget))
    else
        -- Move toward waypoint
        local dx, dy, dz = vecNormalize(
            target.x - pos.x,
            target.y - pos.y,
            target.z - pos.z)
        transform.x = pos.x + dx * stepDist
        transform.y = pos.y + dy * stepDist
        transform.z = pos.z + dz * stepDist
        Runtime.setComponent(self.entityId, "Transform", transform)
    end
end

-- ---------------------------------------------------------------------------
-- Module export
-- ---------------------------------------------------------------------------
return PatrolComponent
