import struct

from . import parser
from . import expresion_generator
from . import variable_layout

def _generate_say(file, step: parser.CutsceneStep, context: variable_layout.VariableContext):
    pass


_step_functions = {
    "say": _generate_say,
}

_step_args = {
    "say": ["tstr"]
}

def _validate_step(step, errors: list[str], context: variable_layout.VariableContext):
    if isinstance(step, parser.CutsceneStep):
        if not step.name.value in _step_args:
            errors.append(step.name.format_message(f'{step.name.value} is not a valid step name'))
            return
        
        args = _step_args[step.name.value]

        if len(args) != len(step.parameters):
            errors.append(step.name.format_message(f'incorrect number of parameters got {len(step.parameters)} expected {len(args)}'))
        
        for i in range(min(len(args), len(step.parameters))):
            parameter = step.parameters[i]
            arg_type = args[i]

            if arg_type == 'tstr':
                if not isinstance(parameter, parser.String):
                    errors.append(parameter.at.format_message('expected string'))
            elif arg_type == 'str':
                if isinstance(parameter, parser.String):
                    if len(parameter.replacements):
                        errors.append(parameter.at.format_message('template parameters not allowed for string'))
                else:
                    errors.append(parameter.at.format_message('expected string for parameter'))
            else:
                raise Exception(f'unknown type {arg_type}')
            
    if isinstance(step, parser.IfStatement):
        type_info = expresion_generator.TypeChecker(context)
        condition_type = type_info.determine_type(step.condition)
        errors += type_info.errors

        if not expresion_generator.is_numerical_type(condition_type):
            errors.append('condition should be an int or float')

        validate_steps(step.statements, errors, context)

    if isinstance(step, parser.Assignment):
        type_info = expresion_generator.TypeChecker(context)
        value_type = type_info.determine_type(step.value)
        errors += type_info.errors

        target_type = context.get_variable_type(step.name.value)

        if not target_type:
            errors.append(step.name.format_message('variable not defined'))
            return
        
        if not target_type in expresion_generator.type_mapping:
            errors.append(step.name.format_message(f'cannot assign type {target_type}'))
            return

CUTSCENE_STEP_TYPE_DIALOG = 0
CUTSCENE_STEP_TYPE_SHOW_RUNE = 1
CUTSCENE_STEP_TYPE_PAUSE = 2
CUTSCENE_STEP_TYPE_PUSH_CONTEXT = 3
CUTSCENE_STEP_TYPE_POP_CONTEXT = 4
CUTSCENE_STEP_TYPE_EXPRESSION = 5
CUTSCENE_STEP_TYPE_IF_STATEMENT = 6

def validate_steps(statements: list, errors: list[str], context: variable_layout.VariableContext):
    for statement in statements:
        _validate_step(statement, errors, context)

def _generate_step(file, step, context: variable_layout.VariableContext):
    if isinstance(step, parser.CutsceneStep):
        _step_functions[step.name.value](file, step, context)

    if isinstance(step, parser.IfStatement):
        file.write(struct.pack('>B', CUTSCENE_STEP_TYPE_EXPRESSION))
        # todo write expression
        file.write(struct.pack('>B', CUTSCENE_STEP_TYPE_IF_STATEMENT))

        file.write(struct.pack('>H', len(step.statements)))
        for statement in step.statements:
            _generate_step(file, statement, context)

    if isinstance(step, parser.Assignment):
        file.write(struct.pack('>B', CUTSCENE_STEP_TYPE_EXPRESSION))
        # todo write expression


def gemerate_steps(file, statements: list, context: variable_layout.VariableContext):
    file.write(struct.pack('>H', len(statements)))

    for statement in statements:
        _generate_step(file, statement, context)