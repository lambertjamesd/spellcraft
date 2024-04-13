import struct

from . import parser
from . import expresion_generator
from . import variable_layout

CUTSCENE_STEP_TYPE_DIALOG = 0
CUTSCENE_STEP_TYPE_SHOW_RUNE = 1
CUTSCENE_STEP_TYPE_PAUSE = 2
CUTSCENE_STEP_TYPE_PUSH_CONTEXT = 3
CUTSCENE_STEP_TYPE_POP_CONTEXT = 4
CUTSCENE_STEP_TYPE_EXPRESSION = 5
CUTSCENE_STEP_TYPE_IF_STATEMENT = 6

_step_args = {
    "say": ["tstr"]
}

_step_ids = {
    "say": CUTSCENE_STEP_TYPE_DIALOG
}

def _write_string(file, string: str):
    encoded = string.encode()
    length = len(encoded)

    if length < 128:
        file.write(struct.pack('>B', length))
    else:
        file.write(struct.pack('>B', (length & 127) | 128))
        file.write(struct.pack('>B', length // 128))

    file.write(encoded)


def _generate_function_step(file, step: parser.CutsceneStep, args: list[str], context:variable_layout.VariableContext):
    file.write(struct.pack('>B', _step_ids[step.name.value]))

    for idx, arg in enumerate(args):
        parameter = step.parameters[idx]

        if arg == 'tstr':
            if not isinstance(parameter, parser.String):
                raise Exception('Parameter should be a string')
            file.write(struct.pack('>B', len(parameter.replacements)))

            for replacment in parameter.replacements:
                expression = expresion_generator.generate_script(replacment, context)
                if not expression:
                    raise Exception(f"Could not generate expression {replacment}")
                expression.serialize(file)

            _write_string(file, '%v'.join(parameter.contents))
            
        elif arg == 'str':
            if not isinstance(parameter, parser.String):
                raise Exception('Parameter should be a string')
            _write_string(file, parameter.contents[0])


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
                        errors.append(parameter.at.format_message('template parameters not allowed for basic strings'))
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

        if not expresion_generator.is_numerical_type(value_type):
            errors.append(step.name.format_message(f'can only assign numerical types'))

        if not target_type:
            errors.append(step.name.format_message('variable not defined'))
            return
        
        if not target_type in expresion_generator.type_mapping:
            errors.append(step.name.format_message(f'cannot assign type {target_type}'))

def validate_steps(statements: list, errors: list[str], context: variable_layout.VariableContext):
    for statement in statements:
        _validate_step(statement, errors, context)

def _generate_step(file, step, context: variable_layout.VariableContext):
    if isinstance(step, parser.CutsceneStep):
        _generate_function_step(file, step, _step_args[step.name.value], context)

    if isinstance(step, parser.IfStatement):
        file.write(struct.pack('>B', CUTSCENE_STEP_TYPE_EXPRESSION))
        expression = expresion_generator.generate_script(step.condition, context)
        if not expression:
            raise Exception(f"Could not generate expression {step.condition}")
        expression.serialize(file)
        file.write(struct.pack('>B', CUTSCENE_STEP_TYPE_IF_STATEMENT))
        generate_steps(file, step.statements, context)

    if isinstance(step, parser.Assignment):
        file.write(struct.pack('>B', CUTSCENE_STEP_TYPE_EXPRESSION))
        expression = expresion_generator.generate_script(step.condition, context)
        if not expression:
            raise Exception(f"Could not generate expression {step.condition}")
        expression.serialize(file)


def generate_steps(file, statements: list, context: variable_layout.VariableContext):
    file.write('CTSN'.encode())
    file.write(struct.pack('>H', len(statements)))

    for statement in statements:
        _generate_step(file, statement, context)