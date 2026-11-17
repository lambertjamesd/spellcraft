import os
import os.path

from . import paths as asset_paths

# the exporters write their makefile dependency files next to their
# intermediate output in build/assets/
DEPENDENCY_DIR = os.path.join('build', 'assets')
OUTPUT_DIR_NAME = 'filesystem'

def _split_make_words(text: str) -> list[str]:
    words: list[str] = []
    current = ''
    index = 0

    while index < len(text):
        char = text[index]

        if char == '\\' and index + 1 < len(text) and text[index + 1] in ' #\\':
            current += text[index + 1]
            index += 2
            continue

        if char == '$' and index + 1 < len(text) and text[index + 1] == '$':
            current += '$'
            index += 2
            continue

        if char.isspace():
            if current:
                words.append(current)
                current = ''
        else:
            current += char

        index += 1

    if current:
        words.append(current)

    return words

# parses the rules in a .d file into (target, prerequisites) pairs
def parse_dependency_file(content: str) -> list[tuple[str, list[str]]]:
    # join line continuations
    content = content.replace('\\\r\n', ' ').replace('\\\n', ' ')
    result: list[tuple[str, list[str]]] = []

    for line in content.splitlines():
        if not line.strip() or line.lstrip().startswith('#'):
            continue

        # the first unescaped ':' separates the targets from the prerequisites
        separator = -1
        index = 0
        while index < len(line):
            if line[index] == '\\':
                index += 2
                continue
            if line[index] == ':':
                separator = index
                break
            index += 1

        if separator == -1:
            continue

        prerequisites = _split_make_words(line[separator + 1:])

        for target in _split_make_words(line[:separator]):
            result.append((target, prerequisites))

    return result

# returns the blend file that is exported to target, a path relative to root
# such as filesystem/meshes/crate.tmesh, or None if it doesn't come from one
def source_for_target(root: str, target: str) -> str | None:
    target = target.replace('\\', '/')
    prefix = OUTPUT_DIR_NAME + '/'

    if not target.startswith(prefix):
        return None

    without_ext = os.path.splitext(target[len(prefix):])[0]

    # materials keep the .mat extension in the source name, foo.mat -> foo.mat.blend
    for candidate in (f'{without_ext}.blend', f'{target[len(prefix):]}.blend'):
        filename = os.path.join(root, 'assets', candidate)

        if os.path.isfile(filename):
            return os.path.normpath(filename)

    return None

def is_exported_source(filename: str) -> bool:
    return asset_paths.is_material_source(filename) or \
        asset_paths.is_tmesh_source(filename) or \
        asset_paths.is_scene_source(filename)

def _same_file(a: str, b: str) -> bool:
    return os.path.normcase(os.path.normpath(a)) == os.path.normcase(os.path.normpath(b))

# finds the exported blend files whose dependency files list filename as a
# prerequisite. The result never includes filename itself
def find_dependents(filename: str) -> list[str]:
    filename = os.path.abspath(filename)

    try:
        root = os.path.dirname(asset_paths.find_assets_dir(filename))
    except Exception:
        return []

    dependency_dir = os.path.join(root, DEPENDENCY_DIR)
    result: set[str] = set()

    for dirpath, _, filenames in os.walk(dependency_dir):
        for name in filenames:
            if not name.endswith('.d'):
                continue

            try:
                with open(os.path.join(dirpath, name), 'r') as file:
                    rules = parse_dependency_file(file.read())
            except OSError:
                continue

            for target, prerequisites in rules:
                if not any(_same_file(os.path.join(root, prerequisite), filename) for prerequisite in prerequisites):
                    continue

                source = source_for_target(root, target)

                if source and not _same_file(source, filename) and is_exported_source(source):
                    result.add(source)

    return sorted(result)
