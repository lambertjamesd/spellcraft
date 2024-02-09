import bpy
import sys
import os

def process_scene():
    input_filename = sys.argv[1]
    output_filename = sys.argv[-1]
    print(sys.argv)

    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue

        if obj.name.startswith('@'):
            # TODO process non static objects
            continue

        mesh: bpy.types.Mesh = obj.data
        print(mesh.name, mesh.library)

        mesh_source = None

        if mesh.library:
            mesh_source = os.path.normpath(os.path.join(os.path.dirname(input_filename), mesh.library.filepath[2:]))

        print(mesh_source)

process_scene()