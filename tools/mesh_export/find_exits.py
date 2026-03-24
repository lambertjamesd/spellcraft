import bpy
import sys
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mesh_export.entities.entry_point

loading_zones = []

with open(sys.argv[-1], 'w') as output:
    output.write('exits\n')
    for obj in bpy.data.objects:
        if 'loading_zone' in obj:
            output.write(f"{obj['loading_zone']}\n")

    output.write('entrances\n')

    for obj in bpy.data.objects:
        if mesh_export.entities.entry_point.is_entry_point(obj):
            output.write(f"${mesh_export.entities.entry_point.get_entry_point(obj)} -> {obj['on_enter'] if 'on_enter' in obj else ''}\n")




