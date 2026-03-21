bl_info = {
    "name": "Atlas Engine Addon",
    "author": "Atlas Engine Team",
    "version": (2, 0, 0),
    "blender": (3, 6, 0),
    "location": "View3D > Sidebar > Atlas",
    "description": "Bidirectional asset pipeline: meshes, rigs, animations, "
                   "materials, and batch export/import for Atlas Engine",
    "category": "Import-Export",
}

# ==============================================================================
# Phase 13 — Blender Addon Integration (Deep)
#   • Export: mesh, scene, rig/armature, animation, material, batch
#   • Import: Atlas JSON mesh, Atlas JSON scene
#   • Helpers: mesh_to_dict, armature_to_dict, action_to_dict, material_to_dict
# ==============================================================================

import bpy
import bmesh
import json
import math
import os
from bpy.props import BoolProperty, StringProperty
from mathutils import Matrix


# ──────────────────────────────────────────────────────────────────────────────
# Conversion helpers
# ──────────────────────────────────────────────────────────────────────────────

def mesh_to_dict(obj):
    """Triangulate and convert a Blender mesh object to an Atlas Engine dict."""
    depsgraph = bpy.context.evaluated_depsgraph_get()
    obj_eval = obj.evaluated_get(depsgraph)
    mesh = obj_eval.to_mesh()

    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces)

    uv_layer = bm.loops.layers.uv.active

    vertices = []
    normals = []
    uvs = []
    indices = []

    vert_map = {}
    idx = 0
    for face in bm.faces:
        for loop in face.loops:
            v = loop.vert
            n = loop.calc_normal() if face.smooth else face.normal
            uv = loop[uv_layer].uv if uv_layer else (0.0, 0.0)

            key = (v.index,
                   round(n.x, 5), round(n.y, 5), round(n.z, 5),
                   round(uv[0], 5), round(uv[1], 5))
            if key not in vert_map:
                vert_map[key] = idx
                vertices.extend([v.co.x, v.co.y, v.co.z])
                normals.extend([n.x, n.y, n.z])
                uvs.extend([uv[0], uv[1]])
                idx += 1
            indices.append(vert_map[key])

    bm.free()
    obj_eval.to_mesh_clear()

    # Collect material slot names used by this mesh
    material_names = [slot.material.name if slot.material else "" for slot in obj.material_slots]

    return {
        "name": obj.name,
        "vertices": vertices,
        "normals": normals,
        "uvs": uvs,
        "indices": indices,
        "materials": material_names,
    }


def armature_to_dict(obj):
    """Convert an Armature object to an Atlas Engine rig dict."""
    if obj.type != 'ARMATURE':
        return None
    arm = obj.data
    bones = []
    for bone in arm.bones:
        parent_name = bone.parent.name if bone.parent else ""
        head = list(bone.head_local)
        tail = list(bone.tail_local)
        mat = bone.matrix_local
        rot = mat.to_quaternion()
        bones.append({
            "name": bone.name,
            "parent": parent_name,
            "head": head,
            "tail": tail,
            "rotation": [rot.w, rot.x, rot.y, rot.z],
            "length": bone.length,
        })
    return {"name": obj.name, "bones": bones}


def action_to_dict(action, armature_obj):
    """Convert an Action to an Atlas Engine animation dict (bone keyframes)."""
    fps = bpy.context.scene.render.fps
    curves_by_bone = {}

    for fcurve in action.fcurves:
        # data_path format: 'pose.bones["BoneName"].location' etc.
        path = fcurve.data_path
        if not path.startswith('pose.bones["'):
            continue
        end = path.find('"]', 12)
        if end == -1:
            continue  # malformed data path — skip gracefully
        bone_name = path[12:end]
        prop = path[end + 3:]  # e.g. 'location', 'rotation_quaternion', 'scale'

        if bone_name not in curves_by_bone:
            curves_by_bone[bone_name] = {}
        if prop not in curves_by_bone[bone_name]:
            curves_by_bone[bone_name][prop] = {}
        curves_by_bone[bone_name][prop][fcurve.array_index] = [
            {"frame": kp.co[0], "value": kp.co[1]}
            for kp in fcurve.keyframe_points
        ]

    channels = []
    for bone_name, props in curves_by_bone.items():
        channels.append({"bone": bone_name, "curves": props})

    frame_start = int(action.frame_range[0])
    frame_end = int(action.frame_range[1])
    return {
        "name": action.name,
        "fps": fps,
        "frame_start": frame_start,
        "frame_end": frame_end,
        "duration": (frame_end - frame_start) / fps,
        "channels": channels,
    }


def material_to_dict(mat):
    """Convert a Blender material's Principled BSDF to an Atlas material dict."""
    result = {
        "name": mat.name,
        "base_color": [1.0, 1.0, 1.0, 1.0],
        "metallic": 0.0,
        "roughness": 0.5,
        "ior": 1.45,
        "alpha": 1.0,
        "emission": [0.0, 0.0, 0.0],
        "normal_map": "",
        "base_color_texture": "",
        "metallic_roughness_texture": "",
    }

    if not mat.use_nodes:
        return result

    for node in mat.node_tree.nodes:
        if node.type != 'BSDF_PRINCIPLED':
            continue

        def _socket_value(name):
            s = node.inputs.get(name)
            if s is None:
                return None
            # If linked to an Image Texture, return the image path
            if s.is_linked:
                linked = s.links[0].from_node
                if linked.type == 'TEX_IMAGE' and linked.image:
                    return linked.image.filepath_from_user()
            return s.default_value

        bc = _socket_value("Base Color")
        if bc is not None:
            if isinstance(bc, str):
                result["base_color_texture"] = bc
            else:
                result["base_color"] = list(bc)

        met = _socket_value("Metallic")
        if met is not None and not isinstance(met, str):
            result["metallic"] = float(met)

        rough = _socket_value("Roughness")
        if rough is not None and not isinstance(rough, str):
            result["roughness"] = float(rough)
        elif isinstance(rough, str):
            result["metallic_roughness_texture"] = rough

        ior = _socket_value("IOR")
        if ior is not None and not isinstance(ior, str):
            result["ior"] = float(ior)

        alpha = _socket_value("Alpha")
        if alpha is not None and not isinstance(alpha, str):
            result["alpha"] = float(alpha)

        em = _socket_value("Emission Color")
        if em is not None and not isinstance(em, str):
            result["emission"] = list(em)[:3]

        normal = node.inputs.get("Normal")
        if normal and normal.is_linked:
            nmap = normal.links[0].from_node
            if nmap.type == 'NORMAL_MAP':
                col = nmap.inputs.get("Color")
                if col and col.is_linked:
                    tex = col.links[0].from_node
                    if tex.type == 'TEX_IMAGE' and tex.image:
                        result["normal_map"] = tex.image.filepath_from_user()

        break  # only process first Principled BSDF

    return result


# ──────────────────────────────────────────────────────────────────────────────
# Import operators
# ──────────────────────────────────────────────────────────────────────────────

class ImportAtlasMesh(bpy.types.Operator):
    """Import an Atlas Engine JSON mesh into the current scene"""
    bl_idname = "import_atlas.mesh"
    bl_label = "Import Atlas Mesh"

    filepath: StringProperty(subtype='FILE_PATH')
    filter_glob: StringProperty(default="*.json", options={'HIDDEN'})

    def execute(self, context):
        with open(self.filepath, 'r') as f:
            data = json.load(f)

        name = data.get("name", os.path.splitext(os.path.basename(self.filepath))[0])
        verts_flat = data.get("vertices", [])
        indices = data.get("indices", [])

        if not verts_flat or not indices:
            self.report({'ERROR'}, "JSON is missing 'vertices' or 'indices'")
            return {'CANCELLED'}

        verts = [(verts_flat[i], verts_flat[i + 1], verts_flat[i + 2])
                 for i in range(0, len(verts_flat), 3)]
        faces = [(indices[i], indices[i + 1], indices[i + 2])
                 for i in range(0, len(indices), 3)]

        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata(verts, [], faces)
        mesh.update()

        obj = bpy.data.objects.new(name, mesh)
        context.collection.objects.link(obj)
        context.view_layer.objects.active = obj
        obj.select_set(True)

        self.report({'INFO'}, f"Imported '{name}' ({len(verts)} verts, {len(faces)} faces)")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


class ImportAtlasScene(bpy.types.Operator):
    """Import an Atlas Engine JSON scene (multiple meshes) into the current scene"""
    bl_idname = "import_atlas.scene"
    bl_label = "Import Atlas Scene"

    filepath: StringProperty(subtype='FILE_PATH')
    filter_glob: StringProperty(default="*.json", options={'HIDDEN'})

    def execute(self, context):
        with open(self.filepath, 'r') as f:
            data = json.load(f)

        objects_data = data.get("objects", [])
        if not objects_data:
            self.report({'WARNING'}, "Scene JSON contains no objects")
            return {'FINISHED'}

        count = 0
        for obj_data in objects_data:
            name = obj_data.get("name", f"atlas_obj_{count}")
            verts_flat = obj_data.get("vertices", [])
            indices = obj_data.get("indices", [])
            if not verts_flat or not indices:
                continue

            verts = [(verts_flat[i], verts_flat[i + 1], verts_flat[i + 2])
                     for i in range(0, len(verts_flat), 3)]
            faces = [(indices[i], indices[i + 1], indices[i + 2])
                     for i in range(0, len(indices), 3)]

            mesh = bpy.data.meshes.new(name)
            mesh.from_pydata(verts, [], faces)
            mesh.update()

            obj = bpy.data.objects.new(name, mesh)
            context.collection.objects.link(obj)
            count += 1

        self.report({'INFO'}, f"Imported {count} object(s) from scene")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


# ──────────────────────────────────────────────────────────────────────────────
# Export operators (mesh / scene)
# ──────────────────────────────────────────────────────────────────────────────

class ExportAtlasMesh(bpy.types.Operator):
    """Export selected mesh to Atlas Engine JSON format"""
    bl_idname = "export_atlas.mesh"
    bl_label = "Export Atlas Mesh"

    filepath: StringProperty(subtype='FILE_PATH')

    def execute(self, context):
        obj = context.active_object
        if obj is None or obj.type != 'MESH':
            self.report({'ERROR'}, "Select a mesh object first")
            return {'CANCELLED'}

        data = mesh_to_dict(obj)
        path = self.filepath
        if not path.endswith(".json"):
            path += ".json"

        with open(path, 'w') as f:
            json.dump(data, f, indent=2)

        self.report({'INFO'}, f"Exported '{obj.name}' to {path}")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


class ExportAtlasScene(bpy.types.Operator):
    """Export all named mesh objects as an Atlas Engine scene JSON"""
    bl_idname = "export_atlas.scene"
    bl_label = "Export Atlas Scene"

    filepath: StringProperty(subtype='FILE_PATH')

    def execute(self, context):
        scene_data = {"scene": bpy.context.scene.name, "objects": []}
        for obj in bpy.context.scene.objects:
            if obj.type == 'MESH' and obj.name:
                scene_data["objects"].append(mesh_to_dict(obj))

        path = self.filepath
        if not path.endswith(".json"):
            path += ".json"

        with open(path, 'w') as f:
            json.dump(scene_data, f, indent=2)

        self.report({'INFO'}, f"Exported scene to {path}")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


# ──────────────────────────────────────────────────────────────────────────────
# Rig / Animation export operators
# ──────────────────────────────────────────────────────────────────────────────

class ExportAtlasRig(bpy.types.Operator):
    """Export selected Armature skeleton to Atlas Engine rig JSON"""
    bl_idname = "export_atlas.rig"
    bl_label = "Export Atlas Rig"

    filepath: StringProperty(subtype='FILE_PATH')

    def execute(self, context):
        obj = context.active_object
        if obj is None or obj.type != 'ARMATURE':
            self.report({'ERROR'}, "Select an Armature object first")
            return {'CANCELLED'}

        data = armature_to_dict(obj)
        path = self.filepath
        if not path.endswith(".json"):
            path += ".json"

        with open(path, 'w') as f:
            json.dump(data, f, indent=2)

        self.report({'INFO'}, f"Exported rig '{obj.name}' ({len(data['bones'])} bones) to {path}")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


class ExportAtlasAnimation(bpy.types.Operator):
    """Export active Action on the selected Armature to Atlas Engine animation JSON"""
    bl_idname = "export_atlas.animation"
    bl_label = "Export Atlas Animation"

    filepath: StringProperty(subtype='FILE_PATH')

    def execute(self, context):
        obj = context.active_object
        if obj is None or obj.type != 'ARMATURE':
            self.report({'ERROR'}, "Select an Armature object first")
            return {'CANCELLED'}
        if obj.animation_data is None or obj.animation_data.action is None:
            self.report({'ERROR'}, "Armature has no active Action")
            return {'CANCELLED'}

        action = obj.animation_data.action
        data = action_to_dict(action, obj)
        path = self.filepath
        if not path.endswith(".json"):
            path += ".json"

        with open(path, 'w') as f:
            json.dump(data, f, indent=2)

        self.report({'INFO'}, f"Exported animation '{action.name}' to {path}")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


# ──────────────────────────────────────────────────────────────────────────────
# Material export operator
# ──────────────────────────────────────────────────────────────────────────────

class ExportAtlasMaterial(bpy.types.Operator):
    """Export the active object's materials to Atlas Engine material JSON"""
    bl_idname = "export_atlas.material"
    bl_label = "Export Atlas Materials"

    filepath: StringProperty(subtype='FILE_PATH')

    def execute(self, context):
        obj = context.active_object
        if obj is None:
            self.report({'ERROR'}, "Select an object first")
            return {'CANCELLED'}

        materials = []
        for slot in obj.material_slots:
            if slot.material:
                materials.append(material_to_dict(slot.material))

        if not materials:
            self.report({'WARNING'}, "No materials found on selected object")
            return {'CANCELLED'}

        data = {"object": obj.name, "materials": materials}
        path = self.filepath
        if not path.endswith(".json"):
            path += ".json"

        with open(path, 'w') as f:
            json.dump(data, f, indent=2)

        self.report({'INFO'}, f"Exported {len(materials)} material(s) to {path}")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


# ──────────────────────────────────────────────────────────────────────────────
# Batch export operator
# ──────────────────────────────────────────────────────────────────────────────

class BatchExportAtlas(bpy.types.Operator):
    """Batch-export all selected objects (meshes, rigs, animations, materials)
    into a directory as individual Atlas Engine JSON files"""
    bl_idname = "export_atlas.batch"
    bl_label = "Batch Export to Atlas"

    directory: StringProperty(subtype='DIR_PATH')
    export_meshes: BoolProperty(name="Export Meshes", default=True)
    export_rigs: BoolProperty(name="Export Rigs", default=True)
    export_animations: BoolProperty(name="Export Animations", default=True)
    export_materials: BoolProperty(name="Export Materials", default=True)

    def execute(self, context):
        out_dir = self.directory
        os.makedirs(out_dir, exist_ok=True)

        meshes_done = rigs_done = anims_done = mats_done = 0

        for obj in context.selected_objects:
            safe_name = bpy.path.clean_name(obj.name)

            if self.export_meshes and obj.type == 'MESH':
                data = mesh_to_dict(obj)
                path = os.path.join(out_dir, f"{safe_name}.mesh.json")
                with open(path, 'w') as f:
                    json.dump(data, f, indent=2)
                meshes_done += 1

            if obj.type == 'MESH' and self.export_materials:
                mats = [material_to_dict(s.material)
                        for s in obj.material_slots if s.material]
                if mats:
                    mat_data = {"object": obj.name, "materials": mats}
                    path = os.path.join(out_dir, f"{safe_name}.mat.json")
                    with open(path, 'w') as f:
                        json.dump(mat_data, f, indent=2)
                    mats_done += 1

            if self.export_rigs and obj.type == 'ARMATURE':
                data = armature_to_dict(obj)
                path = os.path.join(out_dir, f"{safe_name}.rig.json")
                with open(path, 'w') as f:
                    json.dump(data, f, indent=2)
                rigs_done += 1

            if self.export_animations and obj.type == 'ARMATURE':
                if obj.animation_data:
                    exported_actions: set = set()
                    for track in (obj.animation_data.nla_tracks or []):
                        for strip in track.strips:
                            if strip.action and strip.action.name not in exported_actions:
                                anim = action_to_dict(strip.action, obj)
                                anim_name = bpy.path.clean_name(strip.action.name)
                                path = os.path.join(out_dir, f"{safe_name}_{anim_name}.anim.json")
                                with open(path, 'w') as f:
                                    json.dump(anim, f, indent=2)
                                exported_actions.add(strip.action.name)
                                anims_done += 1
                    # Export the active action only if it wasn't already exported
                    active_action = obj.animation_data.action
                    if active_action and active_action.name not in exported_actions:
                        anim = action_to_dict(active_action, obj)
                        anim_name = bpy.path.clean_name(active_action.name)
                        path = os.path.join(out_dir, f"{safe_name}_{anim_name}.anim.json")
                        with open(path, 'w') as f:
                            json.dump(anim, f, indent=2)
                        anims_done += 1

        self.report({'INFO'},
                    f"Batch export complete — "
                    f"{meshes_done} meshes, {rigs_done} rigs, "
                    f"{anims_done} animations, {mats_done} material sets → {out_dir}")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


# ──────────────────────────────────────────────────────────────────────────────
# UI Panel
# ──────────────────────────────────────────────────────────────────────────────

class ATLAS_PT_MainPanel(bpy.types.Panel):
    bl_label = "Atlas Engine"
    bl_idname = "ATLAS_PT_main_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Atlas'

    def draw(self, context):
        layout = self.layout

        layout.label(text="Import", icon='IMPORT')
        layout.operator("import_atlas.mesh",  text="Import Atlas Mesh")
        layout.operator("import_atlas.scene", text="Import Atlas Scene")

        layout.separator()
        layout.label(text="Export Mesh / Scene", icon='EXPORT')
        layout.operator("export_atlas.mesh",  text="Export Selected Mesh")
        layout.operator("export_atlas.scene", text="Export Scene")

        layout.separator()
        layout.label(text="Export Rig & Animation", icon='ARMATURE_DATA')
        layout.operator("export_atlas.rig",       text="Export Rig (Armature)")
        layout.operator("export_atlas.animation", text="Export Animation (Action)")

        layout.separator()
        layout.label(text="Export Materials", icon='MATERIAL')
        layout.operator("export_atlas.material", text="Export Materials")

        layout.separator()
        layout.label(text="Batch", icon='FILE_FOLDER')
        layout.operator("export_atlas.batch", text="Batch Export Selected")


# ──────────────────────────────────────────────────────────────────────────────
# Register / Unregister
# ──────────────────────────────────────────────────────────────────────────────

_CLASSES = (
    ImportAtlasMesh,
    ImportAtlasScene,
    ExportAtlasMesh,
    ExportAtlasScene,
    ExportAtlasRig,
    ExportAtlasAnimation,
    ExportAtlasMaterial,
    BatchExportAtlas,
    ATLAS_PT_MainPanel,
)


def register():
    for cls in _CLASSES:
        bpy.utils.register_class(cls)


def unregister():
    for cls in reversed(_CLASSES):
        bpy.utils.unregister_class(cls)


if __name__ == "__main__":
    register()
