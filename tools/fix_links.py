import bpy

def check_obj(obj):
    if not obj.library or not obj.library.filepath.endswith('overworld.blend'):
        return
    
    obj.make_local()
    
for obj in bpy.data.collections:
    check_obj(obj)
    
for obj in bpy.data.node_groups:
    check_obj(obj)
    
for obj in bpy.data.objects:
    check_obj(obj)
    
for obj in bpy.data.meshes:
    check_obj(obj)
    
for obj in bpy.data.cameras:
    check_obj(obj)
    
for action in bpy.data.actions:
    check_obj(action)
    
for action in bpy.data.worlds:
    check_obj(action)
    
for action in bpy.data.materials:
    check_obj(action)
    
for action in bpy.data.images:
    check_obj(action)