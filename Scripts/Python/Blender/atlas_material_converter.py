"""
Atlas Engine — Material Converter (Phase 13)
============================================
Standalone Blender Python utility that converts every material in the current
.blend file from Blender's Principled BSDF node-graph to the Atlas Engine flat
JSON material format and writes them to a directory.

Usage (from Blender's Scripting workspace or via --python):

    blender --background my_scene.blend \
            --python atlas_material_converter.py \
            -- --output /path/to/output_dir

Or register as a Blender addon to access it from the Atlas panel.
"""

bl_info = {
    "name": "Atlas Material Converter",
    "author": "Atlas Engine Team",
    "version": (1, 0, 0),
    "blender": (3, 6, 0),
    "location": "View3D > Sidebar > Atlas > Materials",
    "description": "Batch-convert Blender Principled BSDF materials to "
                   "Atlas Engine JSON material format",
    "category": "Material",
}

import bpy
import json
import os
import sys
from bpy.props import StringProperty


# ──────────────────────────────────────────────────────────────────────────────
# Core conversion logic (also imported by atlas_blender_addon.py)
# ──────────────────────────────────────────────────────────────────────────────

def _socket_scalar(node, socket_name: str):
    """Return the float value of a socket, or None if it is linked."""
    s = node.inputs.get(socket_name)
    if s is None or s.is_linked:
        return None
    return float(s.default_value)


def _socket_color(node, socket_name: str):
    """Return [r, g, b, a] from a color socket, or None if linked."""
    s = node.inputs.get(socket_name)
    if s is None or s.is_linked:
        return None
    v = s.default_value
    return [float(v[0]), float(v[1]), float(v[2]), float(v[3]) if len(v) > 3 else 1.0]


def _linked_image_path(node, socket_name: str) -> str:
    """Return the filepath of an Image Texture linked to a socket, or ''."""
    s = node.inputs.get(socket_name)
    if s is None or not s.is_linked:
        return ""
    src = s.links[0].from_node
    if src.type == 'TEX_IMAGE' and src.image:
        return src.image.filepath_from_user()
    return ""


def convert_material(mat: bpy.types.Material) -> dict:
    """
    Convert a single Blender material to the Atlas Engine flat JSON format.

    The output schema::

        {
          "name": str,
          "shader": "pbr_metallic_roughness",
          "base_color": [r, g, b, a],
          "base_color_texture": str,         // file path or ""
          "metallic": float,
          "roughness": float,
          "metallic_roughness_texture": str, // file path or ""
          "normal_map": str,                 // file path or ""
          "normal_scale": float,
          "emission": [r, g, b],
          "emission_strength": float,
          "alpha": float,
          "ior": float,
          "double_sided": bool,
          "alpha_mode": "OPAQUE" | "CLIP" | "BLEND"
        }
    """
    result = {
        "name": mat.name,
        "shader": "pbr_metallic_roughness",
        "base_color": [1.0, 1.0, 1.0, 1.0],
        "base_color_texture": "",
        "metallic": 0.0,
        "roughness": 0.5,
        "metallic_roughness_texture": "",
        "normal_map": "",
        "normal_scale": 1.0,
        "emission": [0.0, 0.0, 0.0],
        "emission_strength": 0.0,
        "alpha": 1.0,
        "ior": 1.45,
        "double_sided": not mat.use_backface_culling,
        "alpha_mode": mat.blend_method.upper() if hasattr(mat, 'blend_method') else "OPAQUE",
    }

    if not mat.use_nodes:
        return result

    principled = next(
        (n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'),
        None,
    )
    if principled is None:
        return result

    # Base color
    bc_path = _linked_image_path(principled, "Base Color")
    if bc_path:
        result["base_color_texture"] = bc_path
    else:
        bc = _socket_color(principled, "Base Color")
        if bc is not None:
            result["base_color"] = bc

    # Metallic
    met = _socket_scalar(principled, "Metallic")
    if met is not None:
        result["metallic"] = met

    # Roughness
    rough = _socket_scalar(principled, "Roughness")
    rough_path = _linked_image_path(principled, "Roughness")
    if rough_path:
        result["metallic_roughness_texture"] = rough_path
    elif rough is not None:
        result["roughness"] = rough

    # IOR
    ior = _socket_scalar(principled, "IOR")
    if ior is not None:
        result["ior"] = ior

    # Alpha
    alpha = _socket_scalar(principled, "Alpha")
    if alpha is not None:
        result["alpha"] = alpha

    # Emission
    em_path = _linked_image_path(principled, "Emission Color")
    if not em_path:
        em = _socket_color(principled, "Emission Color")
        if em is not None:
            result["emission"] = em[:3]
    em_strength = _socket_scalar(principled, "Emission Strength")
    if em_strength is not None:
        result["emission_strength"] = em_strength

    # Normal map
    normal_socket = principled.inputs.get("Normal")
    if normal_socket and normal_socket.is_linked:
        nmap_node = normal_socket.links[0].from_node
        if nmap_node.type == 'NORMAL_MAP':
            col_sock = nmap_node.inputs.get("Color")
            if col_sock and col_sock.is_linked:
                tex_node = col_sock.links[0].from_node
                if tex_node.type == 'TEX_IMAGE' and tex_node.image:
                    result["normal_map"] = tex_node.image.filepath_from_user()
            strength = nmap_node.inputs.get("Strength")
            if strength and not strength.is_linked:
                result["normal_scale"] = float(strength.default_value)

    return result


def convert_all_materials(output_dir: str, *, overwrite: bool = True) -> list:
    """
    Convert every material in the current .blend and write JSON files to
    *output_dir*.  Returns a list of output file paths.
    """
    os.makedirs(output_dir, exist_ok=True)
    written = []
    for mat in bpy.data.materials:
        data = convert_material(mat)
        safe = bpy.path.clean_name(mat.name)
        path = os.path.join(output_dir, f"{safe}.mat.json")
        if os.path.exists(path) and not overwrite:
            continue
        with open(path, 'w') as f:
            json.dump(data, f, indent=2)
        written.append(path)
    return written


# ──────────────────────────────────────────────────────────────────────────────
# Blender Operator
# ──────────────────────────────────────────────────────────────────────────────

class ATLAS_OT_ConvertMaterials(bpy.types.Operator):
    """Convert all materials in the scene to Atlas Engine JSON format"""
    bl_idname = "atlas_mat.convert_all"
    bl_label = "Convert All Materials"

    directory: StringProperty(subtype='DIR_PATH',
                              description="Output directory for .mat.json files")

    def execute(self, context):
        if not self.directory:
            self.report({'ERROR'}, "No output directory specified")
            return {'CANCELLED'}

        written = convert_all_materials(self.directory)
        self.report({'INFO'},
                    f"Converted {len(written)} material(s) to {self.directory}")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


class ATLAS_OT_ConvertActiveMaterial(bpy.types.Operator):
    """Convert the active object's active material to Atlas Engine JSON"""
    bl_idname = "atlas_mat.convert_active"
    bl_label = "Convert Active Material"

    filepath: StringProperty(subtype='FILE_PATH',
                             description="Output .mat.json file path")

    def execute(self, context):
        obj = context.active_object
        if obj is None:
            self.report({'ERROR'}, "No active object")
            return {'CANCELLED'}
        mat = obj.active_material
        if mat is None:
            self.report({'ERROR'}, "Active object has no material")
            return {'CANCELLED'}

        data = convert_material(mat)
        path = self.filepath
        if not path.endswith(".json"):
            path += ".json"
        with open(path, 'w') as f:
            json.dump(data, f, indent=2)

        self.report({'INFO'}, f"Converted '{mat.name}' → {path}")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


class ATLAS_PT_MaterialPanel(bpy.types.Panel):
    bl_label = "Materials"
    bl_idname = "ATLAS_PT_material_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Atlas'

    def draw(self, context):
        layout = self.layout
        layout.label(text="Material Conversion", icon='MATERIAL')
        layout.operator("atlas_mat.convert_active", text="Export Active Material")
        layout.operator("atlas_mat.convert_all",    text="Export All Materials")


# ──────────────────────────────────────────────────────────────────────────────
# Command-line batch mode (blender --background ... --python this_file.py -- ...)
# ──────────────────────────────────────────────────────────────────────────────

def _cli_main():
    import argparse
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = []

    parser = argparse.ArgumentParser(
        description="Atlas Material Converter — CLI mode",
    )
    parser.add_argument("--output", "-o", required=True,
                        help="Output directory for .mat.json files")
    parser.add_argument("--no-overwrite", action="store_true",
                        help="Skip materials whose output file already exists")
    args = parser.parse_args(argv)

    written = convert_all_materials(args.output,
                                    overwrite=not args.no_overwrite)
    print(f"[atlas_material_converter] Wrote {len(written)} file(s) to {args.output}")
    for p in written:
        print(f"  {p}")


# ──────────────────────────────────────────────────────────────────────────────
# Register / Unregister
# ──────────────────────────────────────────────────────────────────────────────

_CLASSES = (
    ATLAS_OT_ConvertMaterials,
    ATLAS_OT_ConvertActiveMaterial,
    ATLAS_PT_MaterialPanel,
)


def register():
    for cls in _CLASSES:
        bpy.utils.register_class(cls)


def unregister():
    for cls in reversed(_CLASSES):
        bpy.utils.unregister_class(cls)


if __name__ == "__main__":
    # When run as a script (not as an addon) in background mode, do CLI batch.
    if "--" in sys.argv:
        _cli_main()
    else:
        register()
