import modulefinder
import os
import sys

script_name = sys.argv[-1]

cwd = os.getcwd()
finder = modulefinder.ModuleFinder()
finder.run_script(script_name)

lines = [script_name]

for name, mod in finder.modules.items():
    if mod.__file__ and mod.__file__.startswith(cwd):
        lines.append(os.path.relpath(mod.__file__, start = cwd))
        
with open(f'build/{script_name[0:-3]}.d.template', 'w') as output:
    output.write(f'FILENAME: \\\n    ')
    output.write(' \\\n    '.join(lines))
