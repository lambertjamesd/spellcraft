import bpy

import math
import mathutils
import struct
import io
from . import armature
from ..cutscene import parser
from ..cutscene import tokenizer
from ..cutscene import step_generator
from ..cutscene import variable_layout

# FOV = 2 * arctan(sensor_size / (2 * focal_length))
SENSOR_SIZE = 36

base_transform = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')
base_rotation = base_transform.to_quaternion()

class camera_animation_frame:
    def __init__(self):
        self.location = mathutils.Vector()
        self.rotation = mathutils.Quaternion()
        self.has_quat = False
        self.euler_angles = mathutils.Euler()
        self.has_euler = False
        self.fov = 70 * math.pi / 180

    def __str__(self):
        return f"loc = {self.location}, rot = {self.get_rotation()} fov = {self.fov}"
    
    def get_rotation(self):
        if self.has_quat:
            return self.rotation
        if self.has_euler:
            return self.euler_angles.to_quaternion()
        return mathutils.Quaternion()

    def set_value(self, name: str, index: int, value: float):
        if name == 'location':
            self.location[index] = value
        elif name == 'rotation_euler':
            self.euler_angles[index] = value
            self.has_euler = True
        elif name == 'scale':
            # intentionally ignored
            pass
        elif name == 'lens':
            self.fov = 2 * math.atan(0.5 * SENSOR_SIZE / value)
        else:
            raise Exception(f'unknown animation frame value {name}')
        
    def pack(self):
        position = base_transform @ self.location
        rotation = base_rotation @ self.get_rotation()

        if rotation.w < 0:
            rotation = -rotation
        else:
            rotation = rotation
            
        return struct.pack(
            '>fffffff',
            position.x, position.y, position.z,
            rotation.x, rotation.y, rotation.z,
            self.fov
        )

class camera_animation:
    def __init__(self, name: str):
        self.name: str = name
        self.movement_action: bpy.types.Action | None = None
        self.lens_action: bpy.types.Action | None = None

    def start_frame(self):
        return math.floor(min(self.movement_action.frame_range[0], self.lens_action.frame_range[0]))

    def end_frame(self):
        return math.ceil(max(self.movement_action.frame_range[1], self.lens_action.frame_range[1]))
    
    def _evaluate_action(self, action: bpy.types.Action, frame, result: camera_animation_frame):
        if hasattr(action, 'slots'):
            for slot in action.slots:
                for layer in action.layers:
                    for strip in layer.strips:
                        bag = strip.channelbag(slot)

                        for fcurve in bag.fcurves:
                            result.set_value(fcurve.data_path, fcurve.array_index, fcurve.evaluate(frame))
        else:
            for fcurve in action.fcurves:
                result.set_value(fcurve.data_path, fcurve.array_index, fcurve.evaluate(frame))
    
    def evaluate(self, frame) -> camera_animation_frame:
        result = camera_animation_frame()

        if self.movement_action:
            self._evaluate_action(self.movement_action, frame, result)

        if self.lens_action and self.movement_action != self.lens_action:
            self._evaluate_action(self.lens_action, frame, result)

        return result
    
    def pack_animation_data(self) -> bytes:
        result = io.BytesIO()

        for frame in range(self.start_frame(), self.end_frame()):
            result.write(self.evaluate(frame).pack())

        return result.getvalue()

    def pose_markers(self) -> list:
        actions = [self.movement_action]

        if self.lens_action and self.lens_action != self.movement_action:
            actions.append(self.lens_action)

        result = []

        for action in actions:
            if action and hasattr(action, 'pose_markers'):
                result.extend(action.pose_markers)

        return result

    def pack_events(self, context: variable_layout.VariableContext) -> bytes:
        events: list[tuple[int, int, int, bytes]] = []

        start_frame = self.start_frame()

        for marker in self.pose_markers():
            frame = max(0, marker.frame - start_frame)
            event_type, data = _pack_event(marker.name, f"{self.name}@{marker.frame}", context)
            events.append((frame, len(events), event_type, data))

        if len(events) == 0:
            return bytes([])

        events.sort(key=lambda x: (x[0], x[1]))

        result = io.BytesIO()
        # +1 for the end event
        result.write(struct.pack('>H', len(events) + 1))

        for frame, _, event_type, data in events:
            if frame > 0xFFFF:
                raise Exception(f"camera animation {self.name} has an event past frame {0xFFFF}")
            result.write(struct.pack('>BH', event_type, frame))
            result.write(data)

        result.write(struct.pack('>BH', CAMERA_EVENT_TYPE_END, self.end_frame() - start_frame))

        return result.getvalue()

# matches enum camera_event_type in src/player/camera_animation.h
CAMERA_EVENT_TYPE_END = 0
CAMERA_EVENT_TYPE_RUN = 1
CAMERA_EVENT_SIGNAL = 2
CAMERA_EVENT_WAIT = 3

_string_event_types = {
    'signal': CAMERA_EVENT_SIGNAL,
    'wait': CAMERA_EVENT_WAIT,
}

def _build_fake_token(value: str) -> tokenizer.Token:
    return tokenizer.Token('identifier', value, 0, None)

def _pack_run_event(contents: str, source: str, context: variable_layout.VariableContext) -> bytes:
    body = parser.parse_block(contents, source)

    fn_def = parser.FunctionDefinition(
        _build_fake_token('func'),
        _build_fake_token('main'),
        [],
        [],
        body
    )

    cutscene = parser.Cutscene()
    cutscene.functions.append(fn_def)

    errors: list[str] = []
    step_generator.validate_cutscene(cutscene, errors, context)

    if len(errors) > 0:
        raise Exception(f"errors in camera event {source}\n" + '\n\n'.join(errors))

    result = io.BytesIO()
    step_generator.generate_steps(result, cutscene, context)
    return result.getvalue()

def _pack_event(marker_text: str, source: str, context: variable_layout.VariableContext) -> tuple[int, bytes]:
    if not ':' in marker_text:
        raise Exception(f"camera event {source} '{marker_text}' should be in the form <event>: <contents>")

    event_name, contents = marker_text.split(':', 1)
    event_name = event_name.strip()

    if event_name == 'run':
        return CAMERA_EVENT_TYPE_RUN, _pack_run_event(contents, source, context)

    if event_name in _string_event_types:
        contents_bytes = contents.strip().encode()

        if len(contents_bytes) > 255:
            raise Exception(f"camera event {source} contents is too long")

        return _string_event_types[event_name], bytes([len(contents_bytes)]) + contents_bytes

    raise Exception(f"camera event {source} has unknown event type '{event_name}' expected one of run, signal, wait")


def get_camera_animation(actions: dict[str, camera_animation], name: str) -> camera_animation:
    if not name in actions:
        result = camera_animation(name)
        actions[name] = result
        return result
    
    return actions[name]

def export_camera_animations(filename: str, scene_file, context: variable_layout.VariableContext):
    actions: dict[str, camera_animation] = {}

    for action in bpy.data.actions:
        if action.name.endswith('_cam'):
            get_camera_animation(actions, action.name.removesuffix('_cam')).movement_action = action
        elif action.name.endswith('_lens'):
            get_camera_animation(actions, action.name.removesuffix('_lens')).lens_action = action

    items = actions.items()

    animation_content = []

    events = []

    for name, anim in items:
        if not anim.movement_action:
            raise Exception(f"animation {name} only has lens data add an action to the camera object that ends in _cam")
        if not anim.lens_action:
            anim.lens_action = anim.movement_action


        animation_content.append(anim.pack_animation_data())
        events.append(anim.pack_events(context))

    scene_file.write(len(items).to_bytes(2, 'big'))

    offset = 0
    index = 0

    for name, anim in items:
        name_bytes = name.encode()
        scene_file.write(len(name_bytes).to_bytes(1, 'big'))
        scene_file.write(name_bytes)

        frame_count = anim.end_frame() - anim.start_frame()
        scene_file.write(frame_count.to_bytes(2, 'big'))
        scene_file.write(offset.to_bytes(4, 'big'))
        offset += len(animation_content[index])
        scene_file.write(len(events[index]).to_bytes(2, 'big'))
        scene_file.write(offset.to_bytes(4, 'big'))
        offset += len(events[index])
        index += 1

        if len(events[index - 1]) > 0xFFFF:
            raise Exception(f"camera animation {name} has too much event data")

    with open(filename, 'wb') as file:
        for index, content in enumerate(animation_content):
            file.write(content)
            file.write(events[index])



    