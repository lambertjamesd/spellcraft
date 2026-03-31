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
        scene a: i32;

        func main()                                                
            if a * 3 < 10 then
                return;
            end      

            say "this is a {a} \\\" test";
        end                                       
                                   
    """, "example")

    globals_builder = mesh_export.cutscene.variable_layout.VariableLayoutBuilder()
    
    for global_var in result.globals:
        globals_builder.add_variable(global_var)

    locals_builder = mesh_export.cutscene.variable_layout.VariableLayoutBuilder()
    
    for var in result.scene_vars:
        locals_builder.add_variable(var)

    scene_builder = mesh_export.cutscene.variable_layout.VariableLayoutBuilder()

    for scene_var in result.scene_vars:
        scene_builder.add_variable(scene_var)

    globals = globals_builder.build()
    scene_vars = scene_builder.build()

    if_statement = result.functions[0].body[0]

    if not isinstance(if_statement, mesh_export.cutscene.parser.IfStatement):
        raise Exception('Expected an if statement')

    print(mesh_export.cutscene.expresion_generator.generate_script(
        if_statement, 
        mesh_export.cutscene.variable_layout.VariableContext(globals, scene_vars)
    ).final_expression)

def run_local_layout():
    result = mesh_export.cutscene.parser.parse("""
        func main()
            local a: i32 = 10;
            local b: i32 = a * a;
            local c: i32 = a + b;
            say "b is {b}";
        end
                                   
    """, "example")

    fn = result.functions[0]

    fn_locals = mesh_export.cutscene.local_layout.LocalLayout(fn)

    print("get_local_count", fn_locals.get_local_count())

if __name__ == "__main__":
    run_layout_test()
    run_local_layout()