import json
import os.path
import numbers
import struct
import re
import json
from enum import Enum

class PngMetadata():
    def __init__(self, filename):
        self.palette: list[int] | None = None
        self.width: int = 0
        self.height: int = 0
        self.fmt: str = 'FMT_NONE'

        self._parse_data(filename)

    def _parse_data(self, filename):
        with open(filename, 'rb') as f:
            data = f.read()
        
        if data[:8] != b'\x89PNG\r\n\x1a\n':
            return
        
        index = 8  # Skip PNG signature
        palette = []
        transparency = []
        
        while index < len(data):
            chunk_length = struct.unpack('>I', data[index:index+4])[0]
            chunk_type = data[index+4:index+8].decode('ascii')
            chunk_data = data[index+8:index+8+chunk_length]
            
            if chunk_type == 'PLTE':
                # Palette contains RGB triplets
                palette = [chunk_data[i:i+3] for i in range(0, len(chunk_data), 3)]
            elif chunk_type == 'tRNS':
                transparency = list(chunk_data)
            elif chunk_type == 'IHDR':
                self.width = struct.unpack('>I', chunk_data[0:4])[0]
                self.height = struct.unpack('>I', chunk_data[4:8])[0]
                self.fmt = self._get_meta_format(filename) or self._get_format_from_png(chunk_data[8], chunk_data[9])
            
            index += 8 + chunk_length + 4  # Move to next chunk (including CRC)
        
        if self.fmt == 'FMT_CI4' or self.fmt == 'FMT_CI8':
            self.palette = self._get_palette_from_png(palette, transparency)
        else:
            self.palette = None

    def _get_palette_from_png(self, palette, transparency):
        if not palette or len(palette) == 0:
            return None
        
        result = []

        for index, entry in enumerate(palette):
            r = entry[0]
            g = entry[1]
            b = entry[2]
            a = transparency[index] if index < len(transparency) else 255

            r5 = (r >> 3) & 0x1F  # 5 bits
            g5 = (g >> 3) & 0x1F  # 5 bits
            b5 = (b >> 3) & 0x1F  # 5 bits
            a1 = (a >> 7) & 0x01  # 1 bit
            
            result.append((r5 << 11) | (g5 << 6) | (b5 << 1) | a1)

        return result

    # Color    Allowed    Interpretation
    # Type    Bit Depths
    # 0       1,2,4,8,16  Each pixel is a grayscale sample.
    # 2       8,16        Each pixel is an R,G,B triple.
    # 3       1,2,4,8     Each pixel is a palette index;
    #                     a PLTE chunk must appear.
    # 4       8,16        Each pixel is a grayscale sample,
    #                     followed by an alpha sample.
    # 6       8,16        Each pixel is an R,G,B triple,
    #                     followed by an alpha sample.

    def _get_format_from_png(self, bit_depth, color_type):
        if color_type == 0:
            return 'FMT_I4' if bit_depth <= 4 else 'FMT_I8'
        if color_type == 2:
            return 'FMT_RGBA16'
        if color_type == 3:
            return 'FMT_CI4' if bit_depth <= 4 else 'FMT_CI8'
        if color_type == 4:
            return 'FMT_IA16'
        if color_type == 6:
            return 'FMT_RGBA16'
        
        return 'FMT_RGBA16'

    def _get_meta_format(self, filename):
        json_filename = filename.replace('.png', '.json')
        if not os.path.isfile(json_filename):
            return None
        with open(json_filename, 'r') as file:
            metadata = json.load(file)
        return 'FMT_' + metadata['format']

class Color():
    def __init__(self, r, g, b, a):
        self.r = r
        self.g = g
        self.b = b
        self.a = a

    def copy(self):
        return Color(self.r, self.g, self.b, self.a)
    
    def write(self, file):
        file.write(struct.pack('>BBBB', self.r, self.g, self.b, self.a))

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, Color):
            return False
        
        return self.r == value.r and self.g == value.g and self.b == value.b and self.a == value.a
    
    def __str__(self):
        return f"Color({self.r} {self.g} {self.b} {self.a})"
    
def color_from_vec(color) -> Color:
    return Color(
        int((color[0] ** 0.454545) * 255),
        int((color[1] ** 0.454545) * 255),
        int((color[2] ** 0.454545) * 255),
        int(color[3] * 255)
    )
    
class CombineA(Enum):
    COMBINED = 0
    TEX0 = 1
    TEX1 = 2
    PRIM = 3
    SHADE = 4
    ENV = 5
    _1 = 6
    NOISE = 7
    _0 = 8

class CombineB(Enum):
    COMBINED = 0
    TEX0 = 1
    TEX1 = 2
    PRIM = 3
    SHADE = 4
    ENV = 5
    KEYCENTER = 6
    K4 = 7
    _0 = 8

class CombineC(Enum):
    COMBINED = 0
    TEX0 = 1
    TEX1 = 2
    PRIM = 3
    SHADE = 4
    ENV = 5
    KEYSCALE = 6
    COMBINED_ALPHA = 7
    TEX0_ALPHA = 8
    TEX1_ALPHA = 9
    PRIM_ALPHA = 10
    SHADE_ALPHA = 11
    ENV_ALPHA = 12
    LOD_FRAC = 13
    PRIM_LOD_FRAC = 14
    K5 = 15
    _0 = 16

class CombineD(Enum):
    COMBINED = 0
    TEX0 = 1
    TEX1 = 2
    PRIM = 3
    SHADE = 4
    ENV = 5
    
    _1 = 6
    _0 = 7

class ACombine(Enum):
    COMBINED = 0
    TEX0 = 1
    TEX1 = 2
    PRIM = 3
    SHADE = 4
    ENV = 5
    
    _1 = 6
    _0 = 7

class ACombineMul(Enum):
    LOD_FRAC = 0
    TEX0 = 1
    TEX1 = 2
    PRIM = 3
    SHADE = 4
    ENV = 5
    PRIM_LOD_FRAC = 6
    _0 = 7

class CombineModeCycle():
    def __init__(self, a: CombineA, b: CombineB, c: CombineC, d: CombineD, aa: ACombine, ab: ACombine, ac: ACombineMul, ad: ACombine):
        self.a: CombineA = a
        self.b: CombineB = b
        self.c: CombineC = c
        self.d: CombineD = d

        self.aa: ACombine = aa
        self.ab: ACombine = ab
        self.ac: ACombineMul = ac
        self.ad: ACombine = ad

    def copy(self):
        return CombineModeCycle(
            self.a,
            self.b,
            self.c,
            self.d,

            self.aa,
            self.ab,
            self.ac,
            self.ad
        )

    def __str__(self):
        return f"({self.a} {self.b} {self.c} {self.d}) ({self.aa} {self.ab} {self.ac} {self.ad})"
    
    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, CombineModeCycle):
            return False
        
        return self.a == value.a and self.b == value.b and self.c == value.c and self.d == value.d and \
            self.aa == value.aa and self.ab == value.ab and self.ac == value.ac and self.ad == value.ad

    def uses(self, attr: str):
        return self.a.name == attr or self.b.name == attr or self.c.name == attr or self.c.name == f'{attr}_ALPHA' or self.d.name == attr or \
            self.aa.name == attr or self.ab.name == attr or self.ac.name == attr or self.ad.name == attr

class CombineMode():
    def __init__(self, cyc1: CombineModeCycle | None, cyc2: CombineModeCycle | None):
        self.cyc1: CombineModeCycle | None = cyc1
        self.cyc2: CombineModeCycle | None = cyc2

    def copy(self):
        return CombineMode(self.cyc1.copy() if self.cyc1 else None, self.cyc2.copy() if self.cyc2 else None)

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, CombineMode):
            return False

        return self.cyc1 == value.cyc1 and self.cyc2 == value.cyc2

    def __str__(self):
        if self.cyc2:
            return f"2 cycle {self.cyc1} {self.cyc2}"
        
        return f"1 cycle {self.cyc1}"
    
    def uses(self, attr: str) -> bool:
        if attr == 'TEX1':
            return bool((self.cyc1 and self.cyc1.uses(attr)) or (self.cyc2 and self.cyc2.uses('TEX0')))
        if attr == 'TEX0':
            return bool(self.cyc1 and self.cyc1.uses(attr) or (self.cyc2 and self.cyc2.uses('TEX1')))

        return bool((self.cyc1 and self.cyc1.uses(attr)) or (self.cyc2 and self.cyc2.uses(attr)))

class CycleType(Enum):
    CYCLE_1 = 0
    CYCLE_2 = 1
    CYCLE_COPY = 2
    CYCLE_FILL = 3

class TlutType(Enum):
    TLUT_RGBA = 0
    TLUTIA = 1
    
class SampleType(Enum):
    POINT = 0
    MEDIAN = 1
    BILINEAR = 2

class RgbDitherSel(Enum):
    MAGIC_SQUARE = 0
    STANDARD = 1
    NOISE = 2
    NONE = 3

class AlphaDitherSel(Enum):
    PATTERN = 0
    INV_PATTERN = 1
    NOISE = 2
    NONE = 3

class ZMode(Enum):
    OPAQUE = 0
    INTER = 1
    TRANSPARENT = 2
    DECAL = 3

class CvgDest(Enum):
    CLAMP = 0
    WRAP = 1
    FULL = 2
    SAVE = 3

class ZSourceSel(Enum):
    PIXEL = 0
    PRIM = 1

class AlphaCompare(Enum):
    NONE = 0
    THRESHOLD = 1
    DITHER = 2

class TextureDetail(Enum):
    CLAMP = 0
    SHARPEN = 1
    DETAIL = 2

class BlendColor(Enum):
    IN = 0
    MEMORY = 1
    BLEND = 2
    FOG = 3

class BlendAlpha(Enum):
    IN_A = 0
    FOG_A = 1
    SHADE_A = 2
    _0 = 3

class BlendMix(Enum):
    INV_MUX_A = 0
    MEM_A = 1
    _1 = 2
    _0 = 3
    
class BlendModeCycle():
    def __init__(self, a1: BlendColor, b1: BlendAlpha, a2: BlendColor, b2: BlendMix):
        self.a1: BlendColor = a1
        self.b1: BlendAlpha = b1
        self.a2: BlendColor = a2
        self.b2: BlendMix = b2

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, BlendModeCycle):
            return False
        
        return self.a1 == value.a1 and self.b1 == value.b1 and self.a2 == value.a2 and self.b2 == value.b2

    def needs_read(self):
        return self.a1 == 'MEMORY' or self.a2 == 'MEMORY' or self.b1 == 'MEMORY_CVG' or self.b2 == 'MEMORY_CVG'
    
    def __str__(self):
        return f"({self.a1} {self.b1} {self.a2} {self.b2})"
    
    def copy(self):
        return BlendModeCycle(self.a1, self.b1, self.a2, self.b2)
    
    def should_blend(self):
        return self.a1 != 'IN' or self.a2 != 'IN'

class OtherModes():
    def __init__(
            self,
            cyc1: BlendModeCycle = BlendModeCycle(BlendColor.IN, BlendAlpha._0, BlendColor.IN, BlendMix._1), 
            cyc2: BlendModeCycle | None = None,
            atomic_prim: bool = False,
            cycle_type: CycleType = CycleType.CYCLE_1,
            persp_tex_en = False,
            tex_detail: TextureDetail = TextureDetail.CLAMP,
            tex_lod_en = False,
            en_tlut = False,
            tlut_type: TlutType = TlutType.TLUT_RGBA,
            sample_type: SampleType = SampleType.POINT,
            yuv_en = False,
            key_en = False,
            rgb_dither_sel: RgbDitherSel = RgbDitherSel.NONE,
            alpha_dither_sel: AlphaDitherSel = AlphaDitherSel.NONE,
            force_blend = False,
            alpha_coverage = False,
            x_coverage_alpha = False,
            z_mode: ZMode = ZMode.OPAQUE,
            coverage_dest: CvgDest = CvgDest.CLAMP,
            color_on_coverage = False,
            image_read = False,
            z_write = True,
            z_compare = True,
            aa = False,
            z_source_sel: ZSourceSel = ZSourceSel.PIXEL,
            alpha_compare: AlphaCompare = AlphaCompare.NONE
        ):
        self.cyc1: BlendModeCycle = cyc1
        self.cyc2: BlendModeCycle | None = cyc2

        self.atomic_prim: bool = atomic_prim
        self.cycle_type: CycleType = cycle_type
        self.persp_tex_en: bool = persp_tex_en
        self.tex_detail: TextureDetail = tex_detail
        self.tex_lod_en: bool = tex_lod_en
        self.en_tlut: bool = en_tlut
        self.tlut_type: TlutType = tlut_type
        self.sample_type: SampleType = sample_type
        self.yuv_en: bool = yuv_en
        self.key_en: bool = key_en
        self.rgb_dither_sel: RgbDitherSel = rgb_dither_sel
        self.alpha_dither_sel: AlphaDitherSel = alpha_dither_sel
        self.force_blend: bool = force_blend
        self.alpha_coverage: bool = alpha_coverage
        self.x_coverage_alpha: bool = x_coverage_alpha
        self.z_mode: ZMode = z_mode
        self.coverage_dest: CvgDest = coverage_dest
        self.color_on_coverage: bool = color_on_coverage
        self.image_read: bool = image_read
        self.z_write: bool = z_write
        self.z_compare: bool = z_compare
        self.aa: bool = aa
        self.z_source_sel: ZSourceSel = z_source_sel
        self.alpha_compare: AlphaCompare = alpha_compare
        
    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, OtherModes):
            return False
        
        return self.cyc1 == value.cyc1 and self.cyc2 == value.cyc2 and \
            self.atomic_prim == value.atomic_prim and \
            self.cycle_type == value.cycle_type and \
            self.persp_tex_en == value.persp_tex_en and \
            self.tex_detail == value.tex_detail and \
            self.tex_lod_en == value.tex_lod_en and \
            self.en_tlut == value.en_tlut and \
            self.tlut_type == value.tlut_type and \
            self.sample_type == value.sample_type and \
            self.yuv_en == value.yuv_en and \
            self.key_en == value.key_en and \
            self.rgb_dither_sel == value.rgb_dither_sel and \
            self.alpha_dither_sel == value.alpha_dither_sel and \
            self.force_blend == value.force_blend and \
            self.alpha_coverage == value.alpha_coverage and \
            self.x_coverage_alpha == value.x_coverage_alpha and \
            self.z_mode == value.z_mode and \
            self.coverage_dest == value.coverage_dest and \
            self.color_on_coverage == value.color_on_coverage and \
            self.image_read == value.image_read and \
            self.z_write == value.z_write and \
            self.z_compare == value.z_compare and \
            self.aa == value.aa and \
            self.z_source_sel == value.z_source_sel and \
            self.alpha_compare == value.alpha_compare

    def __str__(self):
        rest = f"{self.z_mode.name} {self.cycle_type.name} {self.tlut_type.name}"

        if self.atomic_prim:
            rest = f"{rest} atm"

        if self.persp_tex_en:
            rest = f"{rest} tpersp"

        if self.image_read:
            rest = f"{rest} im_read"

        if self.en_tlut:
            rest = f"{rest} en_tlut"

        if self.cyc2:
            return f"2 cycle {self.cyc1} {self.cyc2} {rest}"
        
        return f"1 cycle {self.cyc1} {rest}"
    
    def copy(self):
        return OtherModes(
            self.cyc1 and self.cyc1.copy() or None,
            self.cyc2 and self.cyc2.copy() or None,
            self.atomic_prim,
            self.cycle_type,
            self.persp_tex_en,
            self.tex_detail,
            self.tex_lod_en,
            self.en_tlut,
            self.tlut_type,
            self.sample_type,
            self.yuv_en,
            self.key_en,
            self.rgb_dither_sel,
            self.alpha_dither_sel,
            self.force_blend,
            self.alpha_coverage,
            self.x_coverage_alpha,
            self.z_mode,
            self.coverage_dest,
            self.color_on_coverage,
            self.image_read,
            self.z_write,
            self.z_compare,
            self.aa,
            self.z_source_sel,
            self.alpha_compare,
        )
    
    def enable_fog(self):
        result = self.copy()

        result.cyc1 = BlendModeCycle(
            BlendColor.IN,
            BlendAlpha.SHADE_A,
            BlendColor.FOG,
            BlendMix.INV_MUX_A
        )

        return result
    
    def should_blend(self) -> bool:
        return self.cyc1.should_blend() or self.cyc2 != None and self.cyc2.should_blend()
    
class Palette():
    def __init__(self):
        self.colors: list[Color] = []
    
class TexAxis():
    def __init__(self):
        self.min = 0 # were translate
        self.max = 0
        self.shift = 0 # scale_log
        self.clamp = False # was repeats
        self.mask = 0 # was repeats
        self.mirror = False
        self.scroll = 0

    def __eq__(self, value: object) -> bool:
        if not isinstance(value, TexAxis):
            return False

        return self.min == value.min and \
            self.max == value.max and \
            self.shift == value.shift and \
            self.clamp == value.clamp and \
            self.mask == value.mask and \
            self.mirror == value.mirror

_fmt_bit_size = {
    'FMT_NONE': 0,
    'FMT_RGBA16': 16,
    'FMT_RGBA32': 32,
    'FMT_YUV16': 16,
    'FMT_CI4': 4,
    'FMT_CI8': 8,
    'FMT_IA4': 4,
    'FMT_IA8': 8,
    'FMT_IA16': 16,
    'FMT_I4': 4,
    'FMT_I8': 8,
}
    
class Tex():
    def __init__(self):
        self.filename: str | None = None
        self.tmem_addr = 0
        self.palette = 0
        self.min_filter = "linear"
        self.mag_filter = "linear"
        self.s = TexAxis()
        self.t = TexAxis()
        self.frames: list[str] | None = None
        self.palette_data = None
        self.fmt = 'FMT_NONE'
        self.width = 128
        self.height = 128

        self._png_metadata: PngMetadata | None = None

    def copy(self):
        result = Tex()
        result.filename = self.filename
        result.tmem_addr = self.tmem_addr
        result.palette = self.palette
        result.min_filter = self.min_filter
        result.mag_filter = self.mag_filter
        result.s = self.s
        result.t = self.t
        result.frames = self.frames
        result.palette_data = self.palette_data
        result.fmt = self.fmt
        result.width = self.width
        result.height = self.height
        result._png_metadata = self._png_metadata
        return result
    
    def does_scroll(self):
        return self.s.scroll != 0 or self.t.scroll != 0
    
    def set_filename(self, filename):
        self.filename = filename

        if not filename:
            self._png_metadata = None

        self._png_metadata = PngMetadata(filename)
        self.fmt = self._png_metadata.fmt
        self.width = self._png_metadata.width
        self.height = self._png_metadata.height
        self.palette_data = self._png_metadata.palette

    def set_frames(self, frames: list[str]):
        self.frames = frames

        self._png_metadata = PngMetadata(frames[0])
        self.fmt = self._png_metadata.fmt
        self.width = self._png_metadata.width
        self.height = self._png_metadata.height
        self.palette_data = self._png_metadata.palette

    def get_palette_group(self):
        if not self.filename:
            return None

        result = re.fullmatch("(.*_)palette\\d+\\.png", self.filename)

        if not result:
            return result
        
        return result.group(1)

    def does_share_image_data(self, other):
        self_palette_info = self.get_palette_group()
        other_palette_info = other.get_palette_group()
        return bool(self_palette_info) and self_palette_info == other_palette_info
    
    def has_only_palette(self):
        return bool(self.palette_data) and not self.filename and self.frames

    def rom_filename(self) -> str:
        if not self.filename:
            return ""
        
        if not self.filename.startswith('assets'):
            raise Exception('filename should be relative')

        return f"rom:{self.filename[len('assets'):-len('.png')]}.sprite"
        
    def frame_filenames(self) -> list[str]:
        if not self.frames:
            return []

        return list(map(lambda x: f"rom:{x[len('assets'):-len('.png')]}.sprite", self.frames))

    def byte_size(self) -> int:
        return _fmt_bit_size[self.fmt] * self.width * self.height // 8
    

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, Tex):
            return False

        return self.filename == value.filename and \
            self.palette_data == value.palette_data and \
            self.palette == value.palette and \
            self.fmt == value.fmt and \
            self.width == value.width and \
            self.height == value.height and \
            self.s == value.s and \
            self.t == value.t and \
            self.frames == value.frames and \
            self.tmem_addr == value.tmem_addr
                

    def __str__(self):
        result = f"{self.filename} palette({len(self.palette_data) if self.palette_data else 0}) fmt={self.fmt}"

        if self.tmem_addr != 0:
            result = f"{result} tmem={self.tmem_addr}"

        if self.palette != 0:
            result = f"{result} palette={self.palette}"

        return result
    
class Fog():
    def __init__(self):
        self.enabled: bool = False

    def copy(self):
        result = Fog()
        result.enabled = self.enabled
        return result

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, Fog):
            return False
        
        return self.enabled == value.enabled
    
    def __str__(self):
        return f"enabled {str(self.enabled)}"

class VtxEffectType(Enum):
    VTX_EFFECT_NONE = 0
    VTX_EFFECT_SPHERICAL = 1

class VtxEffect:
    def __init__(self, type: VtxEffectType, width: int | None = None, height: int | None = None):
        self.type: VtxEffectType = type
        self.width: int | None = width
        self.height: int | None = height

    def serialize(self, file):
        file.write(self.type.value.to_bytes(1, 'big'))
        if self.type == VtxEffectType.VTX_EFFECT_SPHERICAL:
            if not self.width or not self.height:
                raise Exception('cannot have spherical effect without texture width or height')
            file.write(struct.pack('>BB', self.width, self.height))

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, VtxEffect):
            return False

        return self.type == value.type and self.width == value.width and self.height == value.height
    
    def __str__(self) -> str:
        return str(self.type)
    
class Flags(Enum):
    T3D_FLAG_DEPTH      = 0
    T3D_FLAG_TEXTURED   = 1
    T3D_FLAG_SHADED     = 2
    T3D_FLAG_CULL_FRONT = 3
    T3D_FLAG_CULL_BACK  = 4
    
class Microcode(Enum):
    T3D = 0
    MENU = 1

class Material():
    def __init__(self, name: str = "Unknown"):
        self.microcode: Microcode = Microcode.T3D
        self.name: str = name
        self.combine_mode: CombineMode | None = None
        self.other_modes: OtherModes | None = None
        self.env_color: Color | None = None
        self.prim_color: Color | None = None
        self.blend_color: Color | None = None
        self.lighting: bool | None = None
        self.tex0: Tex | None = None
        self.tex1: Tex | None = None
        self.palette: int = 0
        self.culling: bool | None = None
        self.vtx_effect: VtxEffect | None = None
        self.fog: Fog | None = None
        self.light_count: int | None = None
        self.priority: int | None = None
        self.flags: set[Flags] | None = None

    def copy(self):
        result = Material(self.name)
        result.combine_mode = self.combine_mode.copy() if self.combine_mode else None
        result.other_modes = self.other_modes.copy() if self.other_modes else None
        result.env_color = self.env_color.copy() if self.env_color else None
        result.prim_color = self.prim_color.copy() if self.prim_color else None
        result.blend_color = self.blend_color.copy() if self.blend_color else None
        result.lighting = self.lighting
        result.tex0 = self.tex0.copy() if self.tex0 else None
        result.tex1 = self.tex1.copy() if self.tex1 else None
        result.palette = self.palette
        result.culling = self.culling
        result.vtx_effect = self.vtx_effect
        result.fog = self.fog.copy() if self.fog else None
        result.light_count = self.light_count
        result.priority = self.priority
        result.flags = set(self.flags) if self.flags else None
        return result
    
    def determine_flags(self):
        result = set()

        if self.other_modes and (self.other_modes.z_write or self.other_modes.z_compare):
            result.add(Flags.T3D_FLAG_DEPTH)

        if self.tex0 or self.tex1:
            result.add(Flags.T3D_FLAG_TEXTURED)

        if self.combine_mode and self.combine_mode.uses('SHADE'):
            result.add(Flags.T3D_FLAG_SHADED)

        if self.fog and self.fog.enabled:
            result.add(Flags.T3D_FLAG_SHADED)

        if self.culling == 'front':
            result.add(Flags.T3D_FLAG_CULL_FRONT)
        elif self.culling == True:
            result.add(Flags.T3D_FLAG_CULL_BACK)

        self.flags = result
    
    def does_scroll(self) -> bool:
        return bool((self.tex0 and self.tex0.does_scroll()) or (self.tex1 and self.tex1.does_scroll()))

    def combine_mode_uses(self, attr: str) -> bool:
        if not self.combine_mode:
            return False
        
        return self.combine_mode.uses(attr)

    def layout_textures(self):
        if self.tex0 and self.tex1:
            if self.tex0.does_share_image_data(self.tex1):
                self.tex1.filename = None
            else:
                self.tex1.tmem_addr += _align_memory(self.tex0.byte_size())
            if self.tex0.palette_data and len(self.tex0.palette_data):
                self.tex1.palette = 1

    def get_image_size(self) -> tuple[int, int]:
        if self.tex0:
            return (self.tex0.width, self.tex0.height)
        if self.tex1:
            return (self.tex1.width, self.tex1.height)
        
        return 128, 128
    
    def get_render_layer(self) -> int:
        if self.priority != None:
            return self.priority

        if not self.other_modes:
            return 10
        
        if self.other_modes.z_mode == 'OPAQUE':
            return 10
        if self.other_modes.z_mode == 'INTER':
            return 10
        if self.other_modes.z_mode == 'TRANSPARENT':
            return 20
        if self.other_modes.z_mode == 'DECAL':
            return 30
        
        return 10
    
    def get_palette_data(self) -> tuple[list[int] | None, int]:
        if not self.tex0 and not self.tex1:
            return None, 0
        
        if self.tex0 and not self.tex1:
            return self.tex0.palette_data, self.palette * 16
        
        if self.tex1 and not self.tex0:
            return self.tex1.palette_data, self.palette * 16
        
        result = []

        for tex in [self.tex0, self.tex1]:
            if not tex:
                continue

            palette_data = tex.palette_data

            if not palette_data:
                continue

            index_start = tex.palette * 16

            while len(result) < index_start + len(palette_data):
                result.append(None)

            for idx, value in enumerate(palette_data):
                result[index_start + idx] = value

        index_start = 0

        while index_start < len(result) and result[index_start] == None:
            index_start += 1

        return list(map(lambda x: x or 0, result[index_start:])), index_start

    def is_empty(self):
        return self.combine_mode == None and self.other_modes == None and self.env_color == None and self.prim_color == None and \
            self.blend_color == None and self.lighting == None and self.tex0 == None and self.tex1 == None and self.culling == None and \
            self.vtx_effect == None and self.fog == None and self.flags == None

    def __str__(self):
        return f"""Material {self.name}: 
    microcode = {self.microcode.name}
    combine_mode = {self.combine_mode}
    other_modes = {self.other_modes}
    env_color = {self.env_color}
    prim_color = {self.prim_color}
    blend_color = {self.blend_color}
    culling = {self.culling}
    lighting = {self.lighting}
    tex0 = {self.tex0}
    tex1 = {self.tex1}
    fog = {self.fog}
    vtx_effect = {self.vtx_effect}
    flags = {self.flags}
"""

def _check_is_enum(value, key_path, enum_list):
    if value in enum_list:
        return
    
    raise Exception(f"{key_path} is not a valid value. got '{value}' expected {', '.join(enum_list)}")

def _check_is_int(value, key_path, min, max):
    if not isinstance(value, int) or value < min or value > max:
        raise Exception(f"{key_path} must be an int between {min} and {max}")

def _check_is_number(value, key_path):
    if not isinstance(value, numbers.Real):
        raise Exception(f"{key_path} must be a number")
    
def _optional_number(json_data, key, key_path, defaultValue):
    if key in json_data:
        _check_is_number(json_data[key], f"{key_path}.{key}")
        return json_data[key]
    
    return defaultValue

def _check_is_boolean(value, key_path):
    if not isinstance(value, bool):
        raise Exception(f"{key_path} must be a boolean")
    
def _optional_boolean(json_data, key, key_path, defaultValue):
    if key in json_data:
        _check_is_boolean(json_data[key], f"{key_path}.{key}")
        return json_data[key]
    
    return defaultValue

def _check_is_string(value, key_path):
    if not isinstance(value, str):
        raise Exception(f"{key_path} must be a string")
    
def _optional_string(json_data, key, key_path, defaultValue):
    if key in json_data:
        _check_is_string(json_data[key], f"{key_path}.{key}")
        return json_data[key]
    
    return defaultValue

def _parse_combine_mode_cycle(json_data, key_path):
    color = json_data['color']

    if not color:
        raise Exception(f"{key_path}.color must be defined")
    
    if not isinstance(color, list) or len(color) != 4:
        raise Exception(f"{key_path}.color must be an array of length 4")
    
    alpha = json_data['alpha']

    if not alpha:
        alpha = ["_0", "_0", "_0", "_1"]

    if not isinstance(alpha, list) or len(alpha) != 4:
        raise Exception(f"{key_path}.alpha must be an array of length 4")

    return CombineModeCycle(
        getattr(CombineA, color[0]),
        getattr(CombineB, color[1]),
        getattr(CombineC, color[2]),
        getattr(CombineD, color[3]),

        getattr(ACombine, alpha[0]),
        getattr(ACombine, alpha[1]),
        getattr(ACombineMul, alpha[2]),
        getattr(ACombine, alpha[3]),
    )

def _parse_combine_mode(result: Material, json_data):
    if not 'combineMode' in json_data:
        return
    
    combine_mode = json_data['combineMode']
    
    if isinstance(combine_mode, list):
        if len(combine_mode) > 2:
            raise Exception('combineMode can only have up to two cycles')

        result.combine_mode = CombineMode(
            _parse_combine_mode_cycle(combine_mode[0], 'combineMode[0]'),
            _parse_combine_mode_cycle(combine_mode[1], 'combineMode[1]') if len(combine_mode) == 2 else None,
        )
    else:
        result.combine_mode = CombineMode(
            _parse_combine_mode_cycle(combine_mode, 'combineMode'),
            None
        )

def _parse_blend_mode_cycle(json_data, key_path):
    if json_data == 'OPAQUE':
        return BlendModeCycle(BlendColor.IN, BlendAlpha._0, BlendColor.IN, BlendMix._1)
    if json_data == 'TRANSPARENT':
        return BlendModeCycle(BlendColor.IN, BlendAlpha.IN_A, BlendColor.MEMORY, BlendMix.INV_MUX_A)

    if not isinstance(json_data, list) or len(json_data) != 4:
        raise Exception(f"{key_path} must be an array of length 4")

    return BlendModeCycle(
        getattr(BlendColor, json_data[0]), 
        getattr(BlendAlpha, json_data[1]), 
        getattr(BlendColor, json_data[2]), 
        getattr(BlendMix, json_data[3])
    )

def _parse_blend_mode(result: Material, json_data):
    if not 'blendMode' in json_data:
        return
    
    blend_mode = json_data['blendMode']

    if blend_mode == 'OPAQUE':
        result.other_modes = OtherModes(BlendModeCycle(BlendColor.IN, BlendAlpha._0, BlendColor.IN, BlendMix._1), None)
        return

    if blend_mode == 'TRANSPARENT':
        result.other_modes = OtherModes(BlendModeCycle(BlendColor.IN, BlendAlpha.IN_A, BlendColor.MEMORY, BlendMix.INV_MUX_A), None)
        result.other_modes.z_write = False
        result.other_modes.z_mode = ZMode.TRANSPARENT
        result.other_modes.image_read = True
        return
    
    if blend_mode == 'ALPHA_CLIP':
        result.other_modes = OtherModes(BlendModeCycle(BlendColor.IN, BlendAlpha._0, BlendColor.IN, BlendMix._1), None)
        result.other_modes.alpha_compare = AlphaCompare.THRESHOLD
        result.blend_color = Color(0, 0, 0, 128)
        return
    
    if blend_mode == 'ADD':
        result.other_modes = OtherModes(BlendModeCycle(BlendColor.IN, BlendAlpha.IN_A, BlendColor.MEMORY, BlendMix._1), None)
        result.other_modes.z_write = False
        result.other_modes.z_mode = ZMode.TRANSPARENT
        result.other_modes.image_read = True
        return

    if not 'cyc1' in blend_mode:
        raise Exception(f"blendMode must have at least 1 cycle")
    
    cyc1 = _parse_blend_mode_cycle(blend_mode['cyc1'], 'blendMode.cyc1')
    cyc2 = None

    if 'cyc2' in blend_mode:
        cyc2 = _parse_blend_mode_cycle(blend_mode['cyc2'], 'blendMode.cyc2')

    result.other_modes = OtherModes(cyc1, cyc2)

    if 'zMode' in blend_mode:
        _check_is_enum(blend_mode['zMode'], 'blendMode.zMode', ['OPAQUE', 'INTER', 'TRANSPARENT', 'DECAL'])
        result.other_modes.z_mode = ZMode[blend_mode['zMode']]

    if 'zWrite' in blend_mode:
        _check_is_boolean(blend_mode['zWrite'], 'blendMode.zWrite')
        result.other_modes.z_write = blend_mode['zWrite']

    if 'zCompare' in blend_mode:
        _check_is_boolean(blend_mode['zCompare'], 'blendMode.zCompare')
        result.other_modes.z_compare = blend_mode['zCompare']

    if 'alphaCompare' in blend_mode:
        _check_is_enum(blend_mode['alphaCompare'], 'blendMode.alphaCompare', ['NONE', 'THRESHOLD', 'NOISE'])
        result.other_modes.alpha_compare = AlphaCompare[blend_mode['alphaCompare']]

    if 'texPersp' in blend_mode:
        _check_is_boolean(blend_mode['texPersp'], 'blendMode.texPersp')
        result.other_modes.persp_tex_en = blend_mode['texPersp']
    else:
        result.other_modes.persp_tex_en = True

    if blend_mode['cyc1'] == 'TRANSPARENT':
        result.other_modes.z_write = False
        result.other_modes.z_mode = ZMode.TRANSPARENT
        result.other_modes.image_read = True


def _parse_color(json_data, key_path):
    if not json_data:
        return None
    
    if not isinstance(json_data, list) or len(json_data) != 4:
        raise Exception(f"{key_path} should be an array of 4 number [0,255]")
    
    _check_is_int(json_data[0], f"{key_path}[0]", 0, 255)
    _check_is_int(json_data[1], f"{key_path}[1]", 0, 255)
    _check_is_int(json_data[2], f"{key_path}[2]", 0, 255)
    _check_is_int(json_data[3], f"{key_path}[3]", 0, 255)

    return Color(json_data[0], json_data[1], json_data[2], json_data[3])

def log_pow_2(value) -> int:
    result = 0
    while (1 << result) < value:
        result += 1
    return result

def _parse_tex_axis(json_data, image_size, into: TexAxis, key_path):
    into.min = 0
    into.max = image_size << 2
    into.shift = _optional_number(json_data, 'shift', key_path, into.shift)
    into.mirror = _optional_boolean(json_data, 'mirror', key_path, False)
    into.scroll = _optional_number(json_data, 'scroll', key_path, into.scroll)

    repeats = _optional_number(json_data, 'repeats', key_path, 2048)
    into.mask = log_pow_2(image_size)
    if repeats:
        into.clamp = False
    else:
        into.clamp = True

def _resolve_tex(filename: str, relative_to: str) -> str:
    combined_path = os.path.join(os.path.dirname(relative_to), filename)
    combined_path = os.path.normpath(combined_path)

    if not os.path.exists(combined_path):
        raise Exception(f"The image file {combined_path} does not exist")
    
    if not combined_path.startswith('assets'):
        raise Exception(f"The image {combined_path} must be in the assets folder")
    
    if not combined_path.endswith('.png'):
        raise Exception(f"The image {combined_path} must be a png file")
    
    return combined_path

def _parse_tex(json_data, key_path, relative_to):
    if not json_data:
        return None
    
    result = Tex()

    if isinstance(json_data, str):
        result.set_filename(_resolve_tex(json_data, relative_to))

        result.s.max = result.width << 2
        result.s.mask = log_pow_2(result.width)
        result.t.max = result.height << 2
        result.t.mask = log_pow_2(result.height)
    else:
        if not 'filename' in json_data and not 'frames' in json_data:
            raise Exception(f"{key_path}.filename or .frames must be defined")
        
        if 'filename' in json_data:
            _check_is_string(json_data['filename'], f"{key_path}.filename")
            result.set_filename(_resolve_tex(json_data['filename'], relative_to))

        if 'tmemAddr' in json_data:
            _check_is_int(json_data['tmemAddr'], f"{key_path}.tmemAddr", 0, 4096)
            result.tmem_addr = json_data['tmemAddr']

        if 'minFilter' in json_data:
            _check_is_enum(json_data['minFilter'], f"{key_path}.minFilter", ['nearest', 'linear'])
            result.tmem_addr = json_data['minFilter']

        if 'magFilter' in json_data:
            _check_is_enum(json_data['magFilter'], f"{key_path}.magFilter", ['nearest', 'linear'])
            result.tmem_addr = json_data['magFilter']

        if 's' in json_data:
            _parse_tex_axis(json_data['s'], result.width, result.s, f"{key_path}.s")

        if 't' in json_data:
            _parse_tex_axis(json_data['t'], result.height, result.t, f"{key_path}.t")

        if 'frames' in json_data:
            result.set_frames(list(map(lambda x: _resolve_tex(x, relative_to), json_data['frames'])))

    return result

def _align_memory(value:int) -> int:
    return (value + 7) & ~7

def parse_material(filename: str):
    if not os.path.exists(filename):
        raise Exception(f"The file {filename} does not exist")

    with open(filename, 'r') as json_file:
        json_data = json.load(json_file)

    result = Material(filename)

    result.tex0 = _parse_tex(json_data['tex0'], 'tex0', filename) if 'tex0' in json_data else None
    result.tex1 = _parse_tex(json_data['tex1'], 'tex1', filename) if 'tex1' in json_data else None

    result.layout_textures()

    _parse_combine_mode(result, json_data)
    _parse_blend_mode(result, json_data)
    result.env_color = _parse_color(json_data['envColor'], 'envColor') if 'envColor' in json_data else None
    result.prim_color = _parse_color(json_data['primColor'], 'primColor') if 'primColor' in json_data else None
    result.blend_color = _parse_color(json_data['blendColor'], 'blendColor') if 'blendColor' in json_data else None

    result.lighting = json_data['lighting'] if 'lighting' in json_data else None

    result.culling = _optional_boolean(json_data, 'culling', 'culling', True)
    if not _optional_boolean(json_data, 'zBuffer', 'zBuffer', True) and result.other_modes:
        result.other_modes.z_compare = False
        result.other_modes.z_write = False
        
    use_tex = result.tex0 or result.tex1

    if use_tex:
        if (use_tex.fmt == 'FMT_CI8' or use_tex.fmt == 'FMT_CI4') and result.other_modes:
            result.other_modes.en_tlut = True

    vtx_effect = _optional_string(json_data, 'uvGen', 'uvGen', 'none')

    if vtx_effect == 'spherical':
        if not use_tex:
            raise Exception('need a texture to do spherical uv')

        result.vtx_effect = VtxEffect(VtxEffectType.VTX_EFFECT_SPHERICAL, use_tex.width, use_tex.height)

    result.priority = _optional_number(json_data, 'priority', 'priority', None)

    result.fog = Fog()

    result.determine_flags()

    return result
