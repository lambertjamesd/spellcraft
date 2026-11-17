import bpy
from bpy.app.handlers import persistent

# fast64 relinks its material library after every file load. The exporters
# don't need it and it can take most of an export's time for files that
# link materials saved on other machines, so it is skipped when blender runs
# in the background. See docs/slow_load.md
FAST64_LOAD_HANDLER_NAME = 'load_handler'
FAST64_LOAD_HANDLER_MODULE = 'f3d_material'

def _is_fast64_load_handler(handler) -> bool:
    return getattr(handler, '__name__', None) == FAST64_LOAD_HANDLER_NAME and \
        FAST64_LOAD_HANDLER_MODULE in getattr(handler, '__module__', '')

def _remove_fast64_load_handler():
    for handler in list(bpy.app.handlers.load_post):
        if _is_fast64_load_handler(handler):
            bpy.app.handlers.load_post.remove(handler)

# runs before every load so it works whether fast64 registers before or after this addon
@persistent
def _on_load_pre(*args):
    if bpy.app.background:
        _remove_fast64_load_handler()

def register():
    bpy.app.handlers.load_pre.append(_on_load_pre)

def unregister():
    if _on_load_pre in bpy.app.handlers.load_pre:
        bpy.app.handlers.load_pre.remove(_on_load_pre)
