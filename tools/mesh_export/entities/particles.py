import bpy
import mathutils
import io
import struct
import math

def find_mesh_instance(obj: bpy.types.Object) -> tuple[bpy.types.Mesh | None, mathutils.Vector | None]:
    for modifier in obj.modifiers:
        if not isinstance(modifier, bpy.types.NodesModifier):
            continue

        for node in modifier.node_group.nodes:
            if not isinstance(node, bpy.types.GeometryNodeObjectInfo):
                continue

            if not 'Object' in node.inputs:
                continue

            object_instance = node.inputs['Object']

            if object_instance.is_linked:
                continue

            default_value = object_instance.default_value

            if not default_value:
                continue

            if not isinstance(default_value, bpy.types.Object):
                continue

            mesh = default_value.data

            if mesh == None or not isinstance(mesh, bpy.types.Mesh):
                continue

            return mesh, default_value.dimensions

    return None, None

def transform_particle(point: mathutils.Vector, mid_point: mathutils.Vector, scale_inv: mathutils.Vector) -> mathutils.Vector:
    pos = (point - mid_point) * scale_inv

    return mathutils.Vector((
        round(pos.x),
        round(pos.y),
        round(pos.z)
    ))

class Particles:
    def __init__(self, obj: bpy.types.Object):
        self.obj: bpy.types.Object = obj
        self.center: mathutils.Vector = mathutils.Vector()
        self.scale: mathutils.Vector = mathutils.Vector()
        self.particle_count: int = 0
        self.particle_size: int = 0
        self.particle_scale_width: int = 0
        self.particle_scale_height: int = 0
        self.particles: bytes

    def set_dimensions(self, dimensions: mathutils.Vector):
        scaled = dimensions * 32
        self.particle_size = math.ceil(max(scaled.x, scaled.y))
        self.particle_scale_width = math.floor(0xFFFF * scaled.x / self.particle_size)
        self.particle_scale_height = math.floor(0xFFFF * scaled.y / self.particle_size)

    def write_into(self, file):
        file.write(struct.pack('>fff', self.center.x, self.center.y, self.center.z))
        file.write(struct.pack('>fff', self.scale.x, self.scale.y, self.scale.z))
        file.write(struct.pack('>HHHH', self.particle_count, self.particle_count, self.particle_scale_width, self.particle_scale_height))

def build_particles(obj: bpy.types.Object, base_transform: mathutils.Matrix) -> Particles | None:
    if obj.type != 'MESH':
        return None
    
    instance, dimensions = find_mesh_instance(obj)

    if not instance or not dimensions:
        return None

    mesh: bpy.types.Mesh = obj.data

    full_transform = base_transform @ obj.matrix_world

    min_pos = None
    max_pos = None

    for vertex in mesh.vertices:
        pos = full_transform @ vertex.co
        
        if min_pos is None:
            min_pos = pos
        else:
            min_pos = mathutils.Vector((
                min(min_pos.x, pos.x),
                min(min_pos.y, pos.y),
                min(min_pos.z, pos.z)
            ))

        if max_pos is None:
            max_pos = pos
        else:
            max_pos = mathutils.Vector((
                max(max_pos.x, pos.x),
                max(max_pos.y, pos.y),
                max(max_pos.z, pos.z)
            ))

    scale = (max_pos - min_pos) * 0.5

    scale_inv = mathutils.Vector((
        127 / scale.x,
        127 / scale.y,
        127 / scale.z
    ))

    mid_point = (max_pos + min_pos) * 0.5

    particle_data = io.BytesIO()

    result = Particles(obj)

    result.center = mid_point
    result.scale = scale
    result.particle_count = len(mesh.vertices)
    result.set_dimensions(dimensions)

    for index in range(0, len(mesh.vertices), 2):
        vertex = mesh.vertices[index]
        pos = ((full_transform @ vertex.co) - mid_point) * scale_inv

        posA = transform_particle(full_transform @ vertex.co, mid_point, scale_inv)

        if index + 1 < len(mesh.vertices):
            posB = transform_particle(full_transform @ vertex.co, mid_point, scale_inv)
        else:
            posB = mathutils.Vector()

        particle_data.write(struct.pack(
            '>bbbb', 
            int(posA.x), int(posA.y), int(posA.z), 127
        ))

        particle_data.write(struct.pack(
            '>bbbb', 
            int(posB.x), int(posB.y), int(posB.z), 127
        ))

        # TODO color
        particle_data.write(struct.pack(
            '>BBBB', 
            255, 255, 255, 255
        ))
        particle_data.write(struct.pack(
            '>BBBB', 
            255, 255, 255, 255
        ))

    result.particles = particle_data.getvalue()

    return result
