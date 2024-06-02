
from . import material

class ExportSettings():
    def __init__(self):
        self.fixed_point_scale: int = 64
        self.default_material: material.Material = material.Material()
        self.default_material_name: str | None = None