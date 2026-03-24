import modulefinder

finder = modulefinder.ModuleFinder()
finder.run_script("tools/mesh_export/scene.py")

for name, mod in finder.modules.items():
    if mod.__file__:
        print(mod.__file__)