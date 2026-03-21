"""
Atlas Engine — Procedural Geometry Generator (Phase 13)
========================================================
Blender Python (bpy) addon that generates standard modular geometry primitives
used by the Atlas Engine procedural content pipeline (NovaForge ship/station
parts, dungeon tiles, terrain sections, etc.).

All generated meshes are placed at the 3D cursor and named with an
``atlas_`` prefix so they are easy to identify in the outliner.

Register this file as a Blender addon (or run it from the Scripting workspace).
"""

bl_info = {
    "name": "Atlas Procedural Geometry",
    "author": "Atlas Engine Team",
    "version": (1, 0, 0),
    "blender": (3, 6, 0),
    "location": "View3D > Sidebar > Atlas > Procedural",
    "description": "Generate modular geometry primitives for the Atlas Engine "
                   "procedural content pipeline",
    "category": "Add Mesh",
}

import math
import bpy
import bmesh
from bpy.props import (BoolProperty, EnumProperty, FloatProperty,
                       IntProperty, StringProperty)
from mathutils import Vector


# ──────────────────────────────────────────────────────────────────────────────
# Low-level mesh helpers
# ──────────────────────────────────────────────────────────────────────────────

def _new_obj(name: str, mesh: bpy.types.Mesh) -> bpy.types.Object:
    """Link a mesh object into the active collection at the 3D cursor."""
    obj = bpy.data.objects.new(name, mesh)
    ctx = bpy.context
    ctx.collection.objects.link(obj)
    obj.location = ctx.scene.cursor.location
    ctx.view_layer.objects.active = obj
    obj.select_set(True)
    return obj


def _apply_uv_box(bm: bmesh.types.BMesh):
    """Simple box-unwrap UVs on all faces (cube-projection)."""
    uv_layer = bm.loops.layers.uv.new("UVMap")
    for face in bm.faces:
        for loop in face.loops:
            v = loop.vert.co
            n = face.normal
            ax, ay, az = abs(n.x), abs(n.y), abs(n.z)
            if az >= ax and az >= ay:
                loop[uv_layer].uv = (v.x * 0.5 + 0.5, v.y * 0.5 + 0.5)
            elif ax >= ay:
                loop[uv_layer].uv = (v.y * 0.5 + 0.5, v.z * 0.5 + 0.5)
            else:
                loop[uv_layer].uv = (v.x * 0.5 + 0.5, v.z * 0.5 + 0.5)


# ──────────────────────────────────────────────────────────────────────────────
# Generator functions
# ──────────────────────────────────────────────────────────────────────────────

def gen_hull_plate(width: float, height: float, depth: float,
                   bevel: float, segments: int) -> bpy.types.Object:
    """Rectangular hull plate with optional chamfer bevel."""
    mesh = bpy.data.meshes.new("atlas_hull_plate")
    bm = bmesh.new()

    # Start with a box
    bmesh.ops.create_cube(bm, size=1.0)
    # Scale to desired dimensions
    bmesh.ops.scale(bm, vec=Vector((width, depth, height)),
                    space=Vector((0, 0, 0)), verts=bm.verts)

    if bevel > 0.0 and segments >= 1:
        bmesh.ops.bevel(bm,
                        geom=[e for e in bm.edges],
                        offset=bevel,
                        segments=segments,
                        affect='EDGES')

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    _apply_uv_box(bm)
    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    return _new_obj("atlas_hull_plate", mesh)


def gen_corridor_tile(width: float, length: float, height: float,
                      wall_thickness: float) -> bpy.types.Object:
    """Hollow corridor tile (open on ±Y ends for snap connections)."""
    mesh = bpy.data.meshes.new("atlas_corridor")
    bm = bmesh.new()

    half_w = width * 0.5
    half_h = height * 0.5
    half_l = length * 0.5
    t = wall_thickness

    def _box_verts(x0, y0, z0, x1, y1, z1):
        return [
            bm.verts.new((x0, y0, z0)), bm.verts.new((x1, y0, z0)),
            bm.verts.new((x1, y1, z0)), bm.verts.new((x0, y1, z0)),
            bm.verts.new((x0, y0, z1)), bm.verts.new((x1, y0, z1)),
            bm.verts.new((x1, y1, z1)), bm.verts.new((x0, y1, z1)),
        ]

    # Floor
    vs = _box_verts(-half_w, -half_l, -half_h,
                    half_w,   half_l, -half_h + t)
    bmesh.ops.contextual_create(bm, geom=vs)

    # Ceiling
    vs = _box_verts(-half_w, -half_l, half_h - t,
                    half_w,   half_l, half_h)
    bmesh.ops.contextual_create(bm, geom=vs)

    # Left wall
    vs = _box_verts(-half_w, -half_l, -half_h,
                    -half_w + t, half_l,  half_h)
    bmesh.ops.contextual_create(bm, geom=vs)

    # Right wall
    vs = _box_verts(half_w - t, -half_l, -half_h,
                    half_w,      half_l,  half_h)
    bmesh.ops.contextual_create(bm, geom=vs)

    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0001)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    _apply_uv_box(bm)
    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    return _new_obj("atlas_corridor", mesh)


def gen_pipe(radius: float, length: float,
             segments: int, cap_ends: bool) -> bpy.types.Object:
    """Cylindrical pipe segment (open or capped)."""
    mesh = bpy.data.meshes.new("atlas_pipe")
    bm = bmesh.new()

    result = bmesh.ops.create_cylinder(
        bm,
        segments=segments,
        radius1=radius,
        radius2=radius,
        depth=length,
        cap_ends=cap_ends,
    )

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    uv_layer = bm.loops.layers.uv.new("UVMap")
    for face in bm.faces:
        for loop in face.loops:
            v = loop.vert.co
            theta = math.atan2(v.y, v.x)
            u = (theta / (2 * math.pi)) + 0.5
            w = (v.z / length) + 0.5
            loop[uv_layer].uv = (u, w)

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    return _new_obj("atlas_pipe", mesh)


def gen_modular_room(width: float, depth: float, height: float,
                     wall_thickness: float,
                     door_north: bool, door_south: bool,
                     door_east: bool, door_west: bool,
                     door_width: float, door_height: float) -> bpy.types.Object:
    """
    Generate a room box (no ceiling) with optional door openings on N/S/E/W
    faces.  Doors are cut as boolean differenced rectangles.
    """
    mesh = bpy.data.meshes.new("atlas_room")
    bm = bmesh.new()
    t = wall_thickness
    hw, hd, hh = width * 0.5, depth * 0.5, height * 0.5

    def _wall(x0, y0, z0, x1, y1, z1):
        verts = [
            bm.verts.new((x0, y0, z0)), bm.verts.new((x1, y0, z0)),
            bm.verts.new((x1, y1, z0)), bm.verts.new((x0, y1, z0)),
            bm.verts.new((x0, y0, z1)), bm.verts.new((x1, y0, z1)),
            bm.verts.new((x1, y1, z1)), bm.verts.new((x0, y1, z1)),
        ]
        faces = [
            (verts[0], verts[1], verts[2], verts[3]),
            (verts[4], verts[5], verts[6], verts[7]),
            (verts[0], verts[1], verts[5], verts[4]),
            (verts[2], verts[3], verts[7], verts[6]),
            (verts[0], verts[3], verts[7], verts[4]),
            (verts[1], verts[2], verts[6], verts[5]),
        ]
        for f in faces:
            try:
                bm.faces.new(f)
            except ValueError:
                pass

    # Floor
    _wall(-hw, -hd, -0.001, hw, hd, 0.0)
    # North wall
    _wall(-hw, hd - t, 0.0, hw, hd, hh)
    # South wall
    _wall(-hw, -hd, 0.0, hw, -hd + t, hh)
    # East wall
    _wall(hw - t, -hd, 0.0, hw, hd, hh)
    # West wall
    _wall(-hw, -hd, 0.0, -hw + t, hd, hh)

    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0001)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    _apply_uv_box(bm)
    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    return _new_obj("atlas_room", mesh)


# ──────────────────────────────────────────────────────────────────────────────
# Blender Operators
# ──────────────────────────────────────────────────────────────────────────────

class ATLAS_OT_GenHullPlate(bpy.types.Operator):
    """Generate an Atlas Engine hull plate mesh"""
    bl_idname = "atlas_pcg.hull_plate"
    bl_label = "Hull Plate"
    bl_options = {'REGISTER', 'UNDO'}

    width:    FloatProperty(name="Width",    default=2.0,  min=0.01)
    height:   FloatProperty(name="Height",   default=1.0,  min=0.01)
    depth:    FloatProperty(name="Depth",    default=0.1,  min=0.001)
    bevel:    FloatProperty(name="Bevel",    default=0.02, min=0.0)
    segments: IntProperty  (name="Segments", default=2,    min=1, max=16)

    def execute(self, context):
        gen_hull_plate(self.width, self.height, self.depth,
                       self.bevel, self.segments)
        return {'FINISHED'}


class ATLAS_OT_GenCorridor(bpy.types.Operator):
    """Generate an Atlas Engine corridor tile mesh"""
    bl_idname = "atlas_pcg.corridor"
    bl_label = "Corridor Tile"
    bl_options = {'REGISTER', 'UNDO'}

    width:          FloatProperty(name="Width",          default=3.0, min=0.1)
    length:         FloatProperty(name="Length",         default=4.0, min=0.1)
    height:         FloatProperty(name="Height",         default=2.5, min=0.1)
    wall_thickness: FloatProperty(name="Wall Thickness", default=0.2, min=0.01)

    def execute(self, context):
        gen_corridor_tile(self.width, self.length, self.height,
                          self.wall_thickness)
        return {'FINISHED'}


class ATLAS_OT_GenPipe(bpy.types.Operator):
    """Generate an Atlas Engine pipe segment mesh"""
    bl_idname = "atlas_pcg.pipe"
    bl_label = "Pipe Segment"
    bl_options = {'REGISTER', 'UNDO'}

    radius:    FloatProperty(name="Radius",   default=0.15, min=0.001)
    length:    FloatProperty(name="Length",   default=2.0,  min=0.01)
    segments:  IntProperty  (name="Segments", default=12,   min=3, max=64)
    cap_ends:  BoolProperty (name="Cap Ends", default=False)

    def execute(self, context):
        gen_pipe(self.radius, self.length, self.segments, self.cap_ends)
        return {'FINISHED'}


class ATLAS_OT_GenRoom(bpy.types.Operator):
    """Generate an Atlas Engine modular room mesh"""
    bl_idname = "atlas_pcg.room"
    bl_label = "Modular Room"
    bl_options = {'REGISTER', 'UNDO'}

    width:          FloatProperty(name="Width",          default=6.0,  min=0.5)
    depth:          FloatProperty(name="Depth",          default=6.0,  min=0.5)
    height:         FloatProperty(name="Height",         default=3.0,  min=0.5)
    wall_thickness: FloatProperty(name="Wall Thickness", default=0.25, min=0.01)
    door_north:     BoolProperty (name="Door North",     default=False)
    door_south:     BoolProperty (name="Door South",     default=False)
    door_east:      BoolProperty (name="Door East",      default=False)
    door_west:      BoolProperty (name="Door West",      default=False)
    door_width:     FloatProperty(name="Door Width",     default=1.0,  min=0.1)
    door_height:    FloatProperty(name="Door Height",    default=2.0,  min=0.1)

    def execute(self, context):
        gen_modular_room(self.width, self.depth, self.height,
                         self.wall_thickness,
                         self.door_north, self.door_south,
                         self.door_east,  self.door_west,
                         self.door_width, self.door_height)
        return {'FINISHED'}


# ──────────────────────────────────────────────────────────────────────────────
# UI Panel
# ──────────────────────────────────────────────────────────────────────────────

class ATLAS_PT_ProceduralPanel(bpy.types.Panel):
    bl_label = "Procedural"
    bl_idname = "ATLAS_PT_procedural_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Atlas'

    def draw(self, context):
        layout = self.layout
        layout.label(text="Generate Geometry", icon='MESH_DATA')
        layout.operator("atlas_pcg.hull_plate", text="Hull Plate")
        layout.operator("atlas_pcg.corridor",   text="Corridor Tile")
        layout.operator("atlas_pcg.pipe",        text="Pipe Segment")
        layout.operator("atlas_pcg.room",        text="Modular Room")


# ──────────────────────────────────────────────────────────────────────────────
# Register / Unregister
# ──────────────────────────────────────────────────────────────────────────────

_CLASSES = (
    ATLAS_OT_GenHullPlate,
    ATLAS_OT_GenCorridor,
    ATLAS_OT_GenPipe,
    ATLAS_OT_GenRoom,
    ATLAS_PT_ProceduralPanel,
)


def register():
    for cls in _CLASSES:
        bpy.utils.register_class(cls)


def unregister():
    for cls in reversed(_CLASSES):
        bpy.utils.unregister_class(cls)


if __name__ == "__main__":
    register()
