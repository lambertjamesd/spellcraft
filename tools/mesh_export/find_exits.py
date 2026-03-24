import bpy
import sys
import os

sys.path.append(os.path.dirname(__file__))

import entities.entry_point

loading_zones = []

with open(sys.argv[-1], 'w') as output:
    output.write('exits\n')
    for obj in bpy.data.objects:
        if 'loading_zone' in obj:
            output.write(f"{obj['loading_zone']}\n")

    output.write('entrances\n')

    for obj in bpy.data.objects:
        if entities.entry_point.is_entry_point(obj):
            output.write(f"${entities.entry_point.get_entry_point(obj)} -> {obj['on_enter'] if 'on_enter' in obj else ''}\n")




