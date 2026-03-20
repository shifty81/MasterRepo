bl_info = {
    "name": "Atlas Engine Exporter",
    "author": "Atlas Engine Team",
    "version": (1, 0, 0),
    "blender": (3, 6, 0),
    "location": "View3D > Sidebar > Atlas",
    "description": "Export meshes and scenes to Atlas Engine JSON format",
    "category": "Import-Export",
}

import bpy
import bmesh
import json
import os
from bpy.props import StringProperty


def mesh_to_dict(obj):
    """Convert a Blender mesh object to an Atlas Engine dict."""
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

            key = (v.index, round(n.x, 5), round(n.y, 5), round(n.z, 5),
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

    return {
        "name": obj.name,
        "vertices": vertices,
        "normals": normals,
        "uvs": uvs,
        "indices": indices,
    }


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


class ATLAS_PT_ExportPanel(bpy.types.Panel):
    bl_label = "Atlas Engine Export"
    bl_idname = "ATLAS_PT_export_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Atlas'

    def draw(self, context):
        layout = self.layout
        layout.operator("export_atlas.mesh", text="Export Selected Mesh")
        layout.operator("export_atlas.scene", text="Export Scene")


def register():
    bpy.utils.register_class(ExportAtlasMesh)
    bpy.utils.register_class(ExportAtlasScene)
    bpy.utils.register_class(ATLAS_PT_ExportPanel)


def unregister():
    bpy.utils.unregister_class(ATLAS_PT_ExportPanel)
    bpy.utils.unregister_class(ExportAtlasScene)
    bpy.utils.unregister_class(ExportAtlasMesh)


if __name__ == "__main__":
    register()
