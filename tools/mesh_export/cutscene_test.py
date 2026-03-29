import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import mesh_export.cutscene.tokenizer
import mesh_export.cutscene.parser
import mesh_export.cutscene.variable_layout
import mesh_export.cutscene.expresion_generator
import mesh_export.cutscene.variable_layout
import mesh_export.cutscene.local_layout

def run_layout_test():
    result = mesh_export.cutscene.parser.parse("""
        local a: i32;

        if a * 3 < 10 then
            return;
        end      

        say "this is a {a} \\\" test";
                                   
    """, "example")

    globals_builder = mesh_export.cutscene.variable_layout.VariableLayoutBuilder()
    
    for global_var in result.globals:
        globals_builder.add_variable(global_var)

    locals_builder = mesh_export.cutscene.variable_layout.VariableLayoutBuilder()
    
    for local_var in result.locals:
        locals_builder.add_variable(local_var)

    scene_builder = mesh_export.cutscene.variable_layout.VariableLayoutBuilder()

    for scene_var in result.scene_vars:
        scene_builder.add_variable(scene_var)

    globals = globals_builder.build()
    locals = locals_builder.build()
    scene_vars = scene_builder.build()

    print(mesh_export.cutscene.expresion_generator.generate_script(
        result.statements[0].condition, 
        mesh_export.cutscene.variable_layout.VariableContext(globals, scene_vars, locals)
    ))

def run_local_layout():
    result = mesh_export.cutscene.parser.parse("""
        func fn_test()
            local a: i32 = 10;
            local b: i32 = a * a;
            local c: i32 = a + b;
            say "b is {b}";
        end
                                   
    """, "example")

    fn_locals = mesh_export.cutscene.local_layout.LocalLayout(result.functions[0].body)

    print("get_local_count", fn_locals.get_local_count())

if __name__ == "__main__":
    run_layout_test()
    run_local_layout()