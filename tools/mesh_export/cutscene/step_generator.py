import struct
import io

from . import parser
from . import expresion_generator
from . import variable_layout

CUTSCENE_STEP_TYPE_DIALOG = 0
CUTSCENE_STEP_TYPE_SHOW_RUNE = 1
CUTSCENE_STEP_TYPE_PAUSE = 2
CUTSCENE_STEP_TYPE_PUSH_CONTEXT = 3
CUTSCENE_STEP_TYPE_POP_CONTEXT = 4
CUTSCENE_STEP_TYPE_EXPRESSION = 5
CUTSCENE_STEP_TYPE_JUMP_IF_NOT = 7
CUTSCENE_STEP_TYPE_JUMP = 7
CUTSCENE_STEP_TYPE_SET_LOCAL = 8
CUTSCENE_STEP_TYPE_SET_GLOBAL = 9

class ParameterType():
    def __init__(self, name: str, is_static: bool):
        self.name: str = name
        self.is_static: bool = is_static

_step_args = {
    "say": [ParameterType("tstr", True)],
    "pause": [ParameterType("bool", True), ParameterType("bool", True)],
}

_step_ids = {
    "say": CUTSCENE_STEP_TYPE_DIALOG,
    "pause": CUTSCENE_STEP_TYPE_PAUSE,
}

def _encode_string(string: str) -> bytes:
    encoded = string.encode()
    length = len(encoded)

    if length < 128:
        return struct.pack('>B', length) + encoded
    else:
        return struct.pack('>BB', (length & 127) | 128, length // 128) + encoded

class CutsceneStep():
    def __init__(self, command: int, data: bytes):
        self.command: int = command
        self.data: bytes = data

class JumpCutsceneStep():
    def __init__(self, command: int):
        self.command: int = command
        self.offset: int = 0

class Cutscene():
    def __init__(self):
        self.steps: list[CutsceneStep | JumpCutsceneStep] = []

def _generate_function_step(cutscene: Cutscene, step: parser.CutsceneStep, args: list[ParameterType], context:variable_layout.VariableContext):
    pre_expression: expresion_generator.ExpressionScript | None = None

    for idx, arg in enumerate(args):
        parameter = step.parameters[idx]

        if arg.name == 'tstr':
            if not isinstance(parameter, parser.String):
                raise Exception('Parameter should be a string')

            for replacment in parameter.replacements:
                expression = expresion_generator.generate_script(replacment, context)
                if not expression:
                    raise Exception(f"Could not generate expression {replacment}")
                

                if pre_expression:
                    pre_expression = pre_expression.concat(expression)
                else:
                    pre_expression = expression

        if arg.is_static:
            continue

        expression = None

        if arg.name == 'int' or arg.name == 'bool':
            expression = expresion_generator.generate_script(parameter, context, 'int')
        elif arg.name == 'float':
            expression = expresion_generator.generate_script(parameter, context, 'float')

        if not expression:
            raise Exception(f"Could not generate expression {parameter}")
        
        if pre_expression:
            pre_expression = pre_expression.concat(expression)
        else:
            pre_expression = expression
                
    if pre_expression:
        cutscene.steps.append(CutsceneStep(CUTSCENE_STEP_TYPE_EXPRESSION, pre_expression.to_bytes()))


    data = io.BytesIO()

    for idx, arg in enumerate(args):
        parameter = step.parameters[idx]

        if arg.name == 'tstr':
            data.write(struct.pack('>B', len(parameter.replacements)))
            data.write(_encode_string('%v'.join(parameter.contents)))
            continue
            
        if arg.name == 'str':
            data.write(_encode_string(parameter.contents[0]))
            continue

        if not arg.is_static:
            continue

        static_evaluator = expresion_generator.StaticEvaluator()
        static_value = static_evaluator.check_for_literals(parameter)

        if arg.name == 'bool':
            data.write(struct.pack('>B', 1 if static_value else 0))
        if arg.name == 'int':
            data.write(struct.pack('>i', int(static_value)))
        if arg.name == 'float':
            data.write(struct.pack('>f', float(static_value)))

    cutscene.steps.append(CutsceneStep(_step_ids[step.name.value], data.getvalue()))
            

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

            type_info = expresion_generator.TypeChecker(context)
            parameter_type = type_info.determine_type(parameter)

            errors += type_info.errors

            if arg_type.name == 'tstr':
                if not isinstance(parameter, parser.String):
                    errors.append(parameter.at.format_message('expected string'))
                continue
            elif arg_type.name == 'str':
                if isinstance(parameter, parser.String):
                    if len(parameter.replacements):
                        errors.append(parameter.at.format_message('template parameters not allowed for basic strings'))
                else:
                    errors.append(parameter.at.format_message('expected string for parameter'))
                continue
            
            if arg_type.name == 'bool':
                if parameter_type != 'int':
                    errors.append(parameter.at.format_message(f'expected int got {parameter_type}'))
            else:
                raise Exception(f'unknown type {arg_type.name}')
            
            if arg_type.is_static:
                static_evaluator = expresion_generator.StaticEvaluator()
                static_value = static_evaluator.check_for_literals(parameter)

                if static_value == None:
                    errors.append(parameter.at.format_message(f'parameter must be a literal'))
            
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

def _generate_step(cutscene: Cutscene, step, context: variable_layout.VariableContext):
    if isinstance(step, parser.CutsceneStep):
        _generate_function_step(cutscene, step, _step_args[step.name.value], context)

    if isinstance(step, parser.IfStatement):
        expression = expresion_generator.generate_script(step.condition, context)
        if not expression:
            raise Exception(f"Could not generate expression {step.condition}")
        cutscene.steps.append(CutsceneStep(CUTSCENE_STEP_TYPE_EXPRESSION, expression.to_bytes()))

        if_step = JumpCutsceneStep(CUTSCENE_STEP_TYPE_JUMP_IF_NOT)

        cutscene.steps.append(if_step)

        size_before = len(cutscene.steps)
        
        for statement in step.statements:
            _generate_step(cutscene, statement, context)

        # update the correct jump offset
        if_step.offset = len(cutscene.steps) - size_before

    if isinstance(step, parser.Assignment):
        expression = expresion_generator.generate_script(step.value, context)
        if not expression:
            raise Exception(f"Could not generate expression {step.value}")
        cutscene.steps.append(CutsceneStep(CUTSCENE_STEP_TYPE_EXPRESSION, expression.to_bytes()))

        step_type = CUTSCENE_STEP_TYPE_SET_LOCAL if context.is_local(step.name.value) else CUTSCENE_STEP_TYPE_SET_GLOBAL
        var_type = context.get_variable_type(step.name.value)
        bit_offset = context.get_variable_offset(step.name.value)

        cutscene.steps.append(CutsceneStep(
            step_type, 
            expresion_generator.generate_variable_address(var_type, bit_offset)
        ))

def generate_steps(file, statements: list, context: variable_layout.VariableContext):
    cutscene = Cutscene()
    
    for statement in statements:
        _generate_step(cutscene, statement, context)

    file.write('CTSN'.encode())
    file.write(struct.pack('>H', len(cutscene.steps)))

    for step in cutscene.steps:
        file.write(struct.pack('>B', step.command))
        if isinstance(step, CutsceneStep):
            file.write(step.data)
        elif isinstance(step, JumpCutsceneStep):
            file.write(struct.pack('>h', step.offset))
