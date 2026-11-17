import bpy
import os
import subprocess
import tempfile
from bpy.app.handlers import persistent

from ..asset import paths as asset_paths

ADDON_NAME = __package__.split('.')[0]

# the same entry scripts the makefile uses
MESH_EXPORT_SCRIPT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 't3d_mesh.py'))
MATERIAL_EXPORT_SCRIPT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'material_fast64.py'))

# returns the entry script that exports filename or None if it isn't an exported asset
def _export_script(filename: str) -> str | None:
    if asset_paths.is_material_source(filename):
        return MATERIAL_EXPORT_SCRIPT

    if asset_paths.is_tmesh_source(filename):
        return MESH_EXPORT_SCRIPT

    return None

POLL_INTERVAL = 0.5
LOG_TAIL_LINES = 20

def _default_mk_asset() -> str:
    if os.environ.get('MK_ASSET'):
        return os.environ['MK_ASSET']

    n64_inst = os.environ.get('N64_INST')

    if n64_inst:
        candidate = os.path.join(n64_inst, 'bin', 'mkasset')

        if os.path.exists(candidate):
            return candidate

    return ''

class MeshExportPreferences(bpy.types.AddonPreferences):
    bl_idname = ADDON_NAME

    hot_reload_enabled: bpy.props.BoolProperty(
        name="Export assets on save",
        description="Export blend files in assets/meshes/ and .mat.blend files to filesystem/ in a background blender process after saving",
        default=True,
    )

    mk_asset_path: bpy.props.StringProperty(
        name="mkasset",
        description="Path to libdragon's mkasset. Leave empty to copy assets into filesystem/ uncompressed",
        subtype='FILE_PATH',
        default=_default_mk_asset(),
    )

    addons: bpy.props.StringProperty(
        name="Export addons",
        description="Comma separated addons to enable in the background export process",
        default="fast64-main,mesh_export",
    )

    def draw(self, context):
        layout = self.layout
        layout.prop(self, "hot_reload_enabled")
        layout.prop(self, "mk_asset_path")
        layout.prop(self, "addons")

def _get_preferences() -> MeshExportPreferences | None:
    addon = bpy.context.preferences.addons.get(ADDON_NAME)
    return addon.preferences if addon else None

class _Export():
    def __init__(self, filename: str, process: subprocess.Popen, log):
        self.filename = filename
        self.process = process
        self.log = log

# exports currently running, keyed by blend filename
_running: dict[str, _Export] = {}
# files saved again while their export was still running
_pending: set[str] = set()

def _start_export(filename: str):
    prefs = _get_preferences()

    if not prefs:
        return

    script = _export_script(filename)

    if not script:
        return

    paths = asset_paths.AssetPaths(filename)

    env = os.environ.copy()
    mk_asset = bpy.path.abspath(prefs.mk_asset_path) if prefs.mk_asset_path else ''

    if mk_asset:
        env['MK_ASSET'] = mk_asset
    else:
        env.pop('MK_ASSET', None)

    args = [
        bpy.app.binary_path,
        filename,
        '--background',
        '--log-level', 'fatal',
        '--addons', prefs.addons,
        '--python-exit-code', '1',
        '--python', script,
    ]

    # output goes to a file instead of a pipe so a chatty export can't
    # fill the pipe buffer and stall while nothing is reading it
    log = tempfile.TemporaryFile(mode='w+b')

    try:
        process = subprocess.Popen(args, cwd=paths.root, env=env, stdout=log, stderr=subprocess.STDOUT)
    except Exception as e:
        log.close()
        _report_failure(filename, str(e))
        return

    print(f"hot reload: exporting {os.path.relpath(filename, paths.root)}")
    _running[filename] = _Export(filename, process, log)

    if not bpy.app.timers.is_registered(_poll_exports):
        bpy.app.timers.register(_poll_exports, first_interval=POLL_INTERVAL)

def _read_log(export: _Export) -> str:
    export.log.seek(0)
    result = export.log.read().decode('utf-8', errors='replace')
    export.log.close()
    return result

def _report_failure(filename: str, output: str):
    tail = output.strip().splitlines()[-LOG_TAIL_LINES:]
    print(f"hot reload: failed to export {filename}\n" + "\n".join(tail))

    def draw(self, context):
        for line in tail:
            self.layout.label(text=line)

    # there is no real window to show a popup in and trying crashes blender
    if bpy.app.background:
        return

    wm = bpy.context.window_manager

    # timers don't have a window in their context
    window = bpy.context.window or (wm.windows[0] if wm.windows else None)

    if not window:
        return

    try:
        with bpy.context.temp_override(window=window):
            wm.popup_menu(draw, title=f"Export failed: {os.path.basename(filename)}", icon='ERROR')
    except Exception as e:
        print(f"hot reload: could not show error popup {e}")

def _poll_exports():
    for filename, export in list(_running.items()):
        result = export.process.poll()

        if result is None:
            continue

        del _running[filename]
        output = _read_log(export)

        if result == 0:
            print(f"hot reload: exported {os.path.basename(filename)}")
        else:
            _report_failure(filename, output)

        # saved again while exporting so the output may be stale
        if filename in _pending:
            _pending.discard(filename)
            _start_export(filename)

    # returning None unregisters the timer
    return POLL_INTERVAL if _running else None

@persistent
def _on_save_post(*args):
    # the background export process loads this addon too
    if bpy.app.background:
        return

    prefs = _get_preferences()

    if not prefs or not prefs.hot_reload_enabled:
        return

    filename = args[0] if args and isinstance(args[0], str) and args[0] else bpy.data.filepath

    if not filename or not _export_script(filename):
        return

    filename = os.path.abspath(filename)

    if filename in _running:
        _pending.add(filename)
        return

    _start_export(filename)

def register():
    bpy.utils.register_class(MeshExportPreferences)
    bpy.app.handlers.save_post.append(_on_save_post)

def unregister():
    if _on_save_post in bpy.app.handlers.save_post:
        bpy.app.handlers.save_post.remove(_on_save_post)

    if bpy.app.timers.is_registered(_poll_exports):
        bpy.app.timers.unregister(_poll_exports)

    bpy.utils.unregister_class(MeshExportPreferences)
