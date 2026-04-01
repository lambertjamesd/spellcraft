import struct
import io
import sys
import typing

from . import parser
from . import expresion_generator
from . import variable_layout
from . import static_evaluator
from . import local_layout
from . import tokenizer

CUTSCENE_STEP_DIALOG = 0
CUTSCENE_STEP_SHOW_ITEM = 1
CUTSCENE_STEP_PAUSE = 2
CUTSCENE_STEP_EXPRESSION = 3
CUTSCENE_STEP_JUMP_IF_NOT = 4
CUTSCENE_STEP_JUMP = 5
CUTSCENE_STEP_SET_LOCAL = 6
CUTSCENE_STEP_SET_SCENE = 7
CUTSCENE_STEP_SET_GLOBAL = 8
CUTSCENE_STEP_DELAY = 9
CUTSCENE_STEP_INTERACT_WITH_NPC = 10
CUTSCENE_STEP_IDLE_NPC = 11
CUTSCENE_STEP_CAMERA_LOOK_AT_NPC = 12
CUTSCENE_STEP_CAMERA_FOLLOW = 13
CUTSCENE_STEP_CAMERA_RETURN = 14
CUTSCENE_STEP_CAMERA_ANIMATE = 15
CUTSCENE_STEP_CAMERA_MOVE_TO = 16
CUTSCENE_STEP_CAMERA_WAIT = 17
CUTSCENE_STEP_INTERACT_WITH_LOCATION = 18
CUTSCENE_STEP_FADE = 19
CUTSCENE_STEP_INTERACT_WITH_POSITION = 20
CUTSCENE_STEP_NPC_WAIT = 21
CUTSCENE_STEP_NPC_SET_SPEED = 22
CUTSCENE_STEP_SHOW_TITLE = 23
CUTSCENE_STEP_LOOK_AT_SUBJECT = 24
CUTSCENE_STEP_NPC_ANIMATE = 25
CUTSCENE_STEP_PRINT = 26
CUTSCENE_STEP_SPAWN = 27
CUTSCENE_STEP_CALLBACK = 28
CUTSCENE_STEP_SHOW_BOSS_HEALTH = 29
CUTSCENE_STEP_LOAD_SCENE = 30
CUTSCENE_STEP_DESPAWN = 31
CUTSCENE_STEP_START_TIMER = 32
CUTSCENE_STEP_CANCEL_TIMER = 33
CUTSCENE_STEP_ASK = 34
CUTSCENE_STEP_STOPWATCH_SHOW = 35
CUTSCENE_STEP_STOPWATCH_RUN = 36
CUTSCENE_STEP_AUDIO_PAUSE = 37
CUTSCENE_STEP_SHOW_IMAGE = 38
CUTSCENE_STEP_TEMPLATE_STRING = 39
CUTSCENE_STEP_FUNCTION_CALL = 40

class ParameterType():
    def __init__(self, name: str, is_static: bool):
        self.name: str = name
        self.is_static: bool = is_static

class Alias():
    def __init__(self, source: str, parameters: list[ParameterType] = []):
        self.source: str = source
        self.parameters: list[ParameterType] = parameters

_step_args = {
    "say": [ParameterType("tstr", True)],
    "ask": [ParameterType("tstr", True)],
    "pause": [ParameterType("bool", True), ParameterType("bool", True)],
    "delay": [ParameterType("float", True)],
    "interact_with_npc": [ParameterType("int", True), ParameterType("entity_id", False), ParameterType("entity_id", False)],
    "idle_npc": [ParameterType("entity_id", False)],
    "show_item": [ParameterType("int", True)],
    "cam_look_npc": [ParameterType("entity_id", False)],
    "cam_follow": [],
    "cam_return": [],
    "cam_animate": [ParameterType("str", True)],
    "cam_wait": [],
    "interact_with_location": [ParameterType("int", True), ParameterType("entity_id", False), ParameterType("str", True)],
    "fade": [ParameterType("int", True), ParameterType("float", True)],
    "interact_with_position": [
        ParameterType("int", True), 
        ParameterType("entity_id", False), 
        ParameterType("float", True), 
        ParameterType("float", True), 
        ParameterType("float", True)
    ],
    "npc_wait": [ParameterType("entity_id", False)],
    "npc_set_speed": [ParameterType("entity_id", False), ParameterType("float", True)],
    "show_title": [ParameterType("str", True)],
    "look_at_subject": [],
    "npc_animate": [ParameterType("entity_id", False), ParameterType("str", True), ParameterType("bool", True)],
    "print": [ParameterType("tstr", True)],
    "spawn": [ParameterType("entity_spawner", False)],
    "show_boss_health": [ParameterType("str", True), ParameterType("entity_id", False)],
    "despawn": [ParameterType("entity_id", False)],
    "load_scene": [ParameterType("str", True)],
    "start_timer": [ParameterType("float", True), ParameterType("str", True)],
    "cancel_timer": [],
    "show_main_menu": [],
    "stopwatch_show": [ParameterType("bool", True)],
    "stopwatch_run": [ParameterType("bool", True)],
    "audio_pause": [ParameterType("bool", True)],
    "show_image": [ParameterType("str", True)],
}

_step_ids = {
    "say": CUTSCENE_STEP_DIALOG,
    "ask": CUTSCENE_STEP_ASK,
    "pause": CUTSCENE_STEP_PAUSE,
    "delay": CUTSCENE_STEP_DELAY,
    "interact_with_npc": CUTSCENE_STEP_INTERACT_WITH_NPC,
    "idle_npc": CUTSCENE_STEP_IDLE_NPC,
    "show_item": CUTSCENE_STEP_SHOW_ITEM,
    "cam_look_npc": CUTSCENE_STEP_CAMERA_LOOK_AT_NPC,
    "cam_follow": CUTSCENE_STEP_CAMERA_FOLLOW,
    "cam_return": CUTSCENE_STEP_CAMERA_RETURN,
    "cam_animate": CUTSCENE_STEP_CAMERA_ANIMATE,
    "cam_wait": CUTSCENE_STEP_CAMERA_WAIT,
    "interact_with_location": CUTSCENE_STEP_INTERACT_WITH_LOCATION,
    "fade": CUTSCENE_STEP_FADE,
    "interact_with_position": CUTSCENE_STEP_INTERACT_WITH_POSITION,
    "npc_wait": CUTSCENE_STEP_NPC_WAIT,
    "npc_set_speed": CUTSCENE_STEP_NPC_SET_SPEED,
    "show_title": CUTSCENE_STEP_SHOW_TITLE,
    "look_at_subject": CUTSCENE_STEP_LOOK_AT_SUBJECT,
    "npc_animate": CUTSCENE_STEP_NPC_ANIMATE,
    "print": CUTSCENE_STEP_PRINT,
    "spawn": CUTSCENE_STEP_SPAWN,
    "show_boss_health": CUTSCENE_STEP_SHOW_BOSS_HEALTH,
    "load_scene": CUTSCENE_STEP_LOAD_SCENE,
    "despawn": CUTSCENE_STEP_DESPAWN,
    "start_timer": CUTSCENE_STEP_START_TIMER,
    "cancel_timer": CUTSCENE_STEP_CANCEL_TIMER,
    "stopwatch_show": CUTSCENE_STEP_STOPWATCH_SHOW,
    "stopwatch_run": CUTSCENE_STEP_STOPWATCH_RUN,
    "audio_pause": CUTSCENE_STEP_AUDIO_PAUSE,
    "show_image": CUTSCENE_STEP_SHOW_IMAGE,
}

_steps_that_need_idle = {
    "interact_with_npc",
    "interact_with_location",
    "interact_with_position",
}

_aliases: dict[str, Alias] = {
    "sign_start": Alias("pause true, false; look_at_subject; cam_look_npc ENTITY_ID_SUBJECT;"),
    "sign_end": Alias("pause false, false; cam_return;"),
    "world_pause": Alias("pause true, false;"),
    "world_unpause": Alias("pause false, false;"),
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

    def write_data(self, file):
        file.write(self.data)

class JumpCutsceneStep():
    def __init__(self, command: int, label: str | None = None, token: tokenizer.Token | None = None):
        self.command: int = command
        self.offset: int = 0
        self.label: str | None = label
        self.token: tokenizer.Token | None = token

    def write_data(self, file):
        file.write(struct.pack('>h', self.offset))

class ExpressionCutsceneStep():
    def __init__(self, expr: expresion_generator.ExpressionScript):
        self.command: int = CUTSCENE_STEP_EXPRESSION
        self.expr: expresion_generator.ExpressionScript = expr
        
    def write_data(self, file):
        self.expr.serialize(file)

class ExpressionFunctionCall():
    def __init__(self, fn_index: int, argc: int, retc: int):
        self.command: int = CUTSCENE_STEP_FUNCTION_CALL
        self.fn_index = fn_index
        self.argc: int = argc
        self.retc: int = retc

    def write_data(self, file):
        file.write(struct.pack('>HBB', self.fn_index, self.argc, self.retc))

class LabelStep():
    def __init__(self, name: str):
        self.name: str = name

StepTypes = typing.Union[
    CutsceneStep, 
    JumpCutsceneStep, 
    ExpressionCutsceneStep, 
    ExpressionCutsceneStep, 
    ExpressionFunctionCall, 
    LabelStep,
]

class Cutscene():
    def __init__(self):
        self.steps: list[StepTypes] = []
        self._label_count: int = 0

    def add_label(self, label: str):
        self.steps.append(LabelStep(label))
        self._label_count += 1

    def step_count(self) -> int:
        return len(self.steps) - self._label_count

    def apply_jump_labels(self) -> list[str]:
        errors = []

        labels: dict[str, int] = {}

        idx = 0
        while idx < len(self.steps):
            step = self.steps[idx]

            if isinstance(step, LabelStep):
                labels[step.name] = idx
                del self.steps[idx]
            else:
                idx += 1

        self._label_count = 0

        for idx, step in enumerate(self.steps):
            if not isinstance(step, JumpCutsceneStep):
                continue
            if not step.label:
                continue

            if not step.label in labels:
                if step.token:
                    errors.append(step.token.format_message(f'the label {step.label} was not found'))
                else:
                    errors.append(f'the label {step.label} was not found')

            step.offset = labels[step.label] - (idx + 1)

        return errors
    
    def simplify(self, start_idx: int):
        idx = start_idx

        while idx < len(self.steps):
            if idx == start_idx:
                idx += 1
                continue

            prev = self.steps[idx-1]
            curr = self.steps[idx]

            if isinstance(prev, ExpressionCutsceneStep) and isinstance(curr, ExpressionCutsceneStep):
                self.steps[idx-1] = ExpressionCutsceneStep(prev.expr.concat(curr.expr))
                del self.steps[idx]
            elif isinstance(prev, JumpCutsceneStep) and isinstance(curr, LabelStep) and prev.label == curr.name:
                del self.steps[idx-1]
            else:
                idx += 1
            
def build_template_string(string: parser.String, context: variable_layout.VariableContext):
    parts: list[str] = []

    for idx, replacement in enumerate(string.replacements):
        parts.append(string.contents[idx])

        type_checker = expresion_generator.TypeChecker(context)
        argument_type = type_checker.determine_type(replacement.expr)

        if replacement.format:
            if replacement.format.value == 'duration':
                if argument_type != 'float':
                    raise Exception(replacement.expr.at.format_message(f"Expected float got ${argument_type}"))
                parts.append('%t')
            else:
                raise Exception(replacement.format.format_message(f"Invalid string format"))
        elif argument_type == 'str':
            parts.append('%s')
        elif argument_type == 'int':
            parts.append('%d')
        elif argument_type == 'float':
            parts.append('%f')

    parts.append(string.contents[-1])
        
    return ''.join(parts)

def _is_numerical(type: parser.DataType) -> bool:
    return type.name.value in expresion_generator.type_mapping

def _can_assign(from_type: str, into: parser.DataType) -> bool:
    if _is_numerical(into) and (from_type == 'int' or from_type == 'float'):
        return True
    
    if from_type == 'str' and into.name.value == 'str':
        return True

    return False

def _generate_expression_collection(cutscene: Cutscene, collection: expresion_generator.ExpressionCollection, context: variable_layout.VariableContext, retc: int = 1):
    for chunk in collection.chunks:
        if chunk.script:
            cutscene.steps.append(ExpressionCutsceneStep(chunk.script))
        fn_index, fn = context.lookup_function(chunk.fn_call.name.value)

        if not fn:
            raise Exception(f'could not find function {chunk.fn_call.name.value}')

        cutscene.steps.append(ExpressionFunctionCall(fn_index, len(chunk.fn_call.args), retc if chunk == collection.chunks[-1] else 1))

    if collection.final_expression:
        cutscene.steps.append(ExpressionCutsceneStep(collection.final_expression))

def _generate_function_call(cutscene: Cutscene, step: parser.CutsceneStep, expected_count: int, context: variable_layout.VariableContext) -> bool:
    fn_index, fn = context.lookup_function(step.name.value)

    if not fn:
        return False
    
    pre_expression: expresion_generator.ExpressionCollection = expresion_generator.ExpressionCollection()

    for arg_index, arg in enumerate(step.parameters):
        generate_to = 'int'

        if fn.args[arg_index].type_name.name.value == 'float':
            generate_to = 'float'

        pre_expression = pre_expression.concat(expresion_generator.generate_script(arg, context, generate_to))

    _generate_expression_collection(cutscene, pre_expression, context)

    cutscene.steps.append(ExpressionFunctionCall(fn_index, len(step.parameters), expected_count))
    context.modify_stack_size(expected_count - len(step.parameters))

    return True


def _generate_function_step(cutscene: Cutscene, step: parser.CutsceneStep, args: list[ParameterType], context:variable_layout.VariableContext):
    pre_expression: expresion_generator.ExpressionCollection = expresion_generator.ExpressionCollection()
    pop_count = 0

    for idx, arg in enumerate(args):
        parameter = step.parameters[idx]

        if arg.name == 'tstr':
            if not isinstance(parameter, parser.String):
                raise Exception('Parameter should be a string')

            pop_count += len(parameter.replacements)
            for replacement in parameter.replacements:
                expression = expresion_generator.generate_script(replacement.expr, context)
                if not expression:
                    raise Exception(f"Could not generate expression {replacement}")   

                pre_expression = pre_expression.concat(expression)

        if arg.is_static:
            continue

        pop_count += 1

        expression = None

        if arg.name == 'int' or arg.name == 'bool' or arg.name == 'entity_spawner' or arg.name == 'entity_id':
            expression = expresion_generator.generate_script(parameter, context, 'int')
        elif arg.name == 'float':
            expression = expresion_generator.generate_script(parameter, context, 'float')

        if not expression:
            raise Exception(f"Could not generate expression {parameter}")
        
        pre_expression = pre_expression.concat(expression)
                
    _generate_expression_collection(cutscene, pre_expression, context)

    data = io.BytesIO()
    
    for idx, arg in enumerate(args):
        parameter = step.parameters[idx]

        if arg.name == 'tstr':
            if not isinstance(parameter, parser.String):
                raise Exception("Expected tstr but didn't get string")

            data.write(struct.pack('>B', len(parameter.replacements)))
            data.write(_encode_string(build_template_string(parameter, context)))
            continue
            
        if arg.name == 'str':
            if not isinstance(parameter, parser.String):
                raise Exception("Expected str but didn't get string")

            data.write(_encode_string(parameter.contents[0]))
            continue

        if not arg.is_static:
            continue

        eval = static_evaluator.StaticEvaluator()
        static_value = eval.check_for_literals(parameter)

        if arg.name == 'bool':
            data.write(struct.pack('>B', 1 if static_value else 0))
        elif arg.name == 'int':
            data.write(struct.pack('>i', int(static_value or 0)))
        elif arg.name == 'entity_id':
            data.write(struct.pack('>H', int(static_value or 0)))
        elif arg.name == 'float':
            data.write(struct.pack('>f', float(static_value or 0)))
        else:
            raise Exception(f"could not write arg of type {arg.name}")
        
    context.modify_stack_size(-pop_count)

    cutscene.steps.append(CutsceneStep(_step_ids[step.name.value], data.getvalue()))

def _generate_alias(cutscene: Cutscene, step: parser.CutsceneStep, alias: Alias, context:variable_layout.VariableContext, return_label: str):
    cutscene_alias = parser.parse_block(alias.source, "alias")

    for statementStep in cutscene_alias:
        _generate_step(cutscene, statementStep, context, return_label)

def _does_end_in_return(block: list[parser.Statement]) -> bool:
    if len(block) == 0:
        return False
    
    last_statement = block[-1]

    if isinstance(last_statement, parser.ReturnStatement):
        return True
    
    if isinstance(last_statement, parser.IfStatement):
        if not last_statement.else_block:
            return False
        
        return _does_end_in_return(last_statement.statements) and _does_end_in_return(last_statement.else_block)
    
    return False

def _validate_local_fn_call(step: parser.CutsceneStep, errors: list[str], context: variable_layout.VariableContext, local_fn: parser.FunctionDefinition):
    if len(step.parameters) != len(local_fn.args):
        errors.append(step.name.format_message(f'expected {len(local_fn.args)} got {len(step.parameters)} arguments'))

    type_info = expresion_generator.TypeChecker(context)
    for index in range(min(len(step.parameters), len(local_fn.args))):
        parameter_type = type_info.determine_type(step.parameters[index])

        if not _can_assign(parameter_type, local_fn.args[index].type_name):
            errors.append(step.parameters[index].at.format_message(f'Cannot assign {parameter_type} to type {local_fn.args[index].type_name}'))

    errors += type_info.errors
    
def _validate_system_call(step: parser.CutsceneStep, errors: list[str], context: variable_layout.VariableContext):
    args = None

    if step.name.value in _step_args:
        args = _step_args[step.name.value]
    elif step.name.value in _aliases:
        args = _aliases[step.name.value].parameters

    if args == None:
        errors.append(step.name.format_message(f'{step.name.value} is not a valid step name'))
        return

    if len(args) != len(step.parameters):
        errors.append(step.name.format_message(f'incorrect number of parameters got {len(step.parameters)} expected {len(args)}'))
    
    type_info = expresion_generator.TypeChecker(context)
    for i in range(min(len(args), len(step.parameters))):
        parameter = step.parameters[i]
        arg_type = args[i]

        parameter_type = type_info.determine_type(parameter)

        if arg_type.name == 'tstr':
            if not isinstance(parameter, parser.String):
                errors.append(parameter.at.format_message('expected string'))
            else:
                for replacement in parameter.replacements:
                    replacement_type = type_info.determine_type(replacement.expr)
                    
                    if replacement_type != 'str' and replacement_type != 'int' and replacement_type != 'float':
                        errors.append(replacement.expr.at.format_message(f'expected string, int or float but got {replacement_type}'))
                        
                    
            continue
        elif arg_type.name == 'str':
            if isinstance(parameter, parser.String):
                if len(parameter.replacements):
                    errors.append(parameter.at.format_message('template parameters not allowed for basic strings'))
            else:
                errors.append(parameter.at.format_message('expected string for parameter'))
            continue
        
        if arg_type.name == 'bool':
            if parameter_type != 'int' and parameter_type != 'bool':
                errors.append(parameter.at.format_message(f'expected bool got {parameter_type}'))
        elif arg_type.name == 'int' or arg_type.name == 'entity_id' or arg_type.name == 'entity_spawner':
            if parameter_type != 'int':
                errors.append(parameter.at.format_message(f'expected int got {parameter_type}'))
        elif arg_type.name == 'float':
            if parameter_type != 'float' and parameter_type != 'int':
                errors.append(parameter.at.format_message(f'expected float got {parameter_type}'))
        else:
            raise Exception(f'unknown type {arg_type.name}')
        
        if arg_type.is_static:
            eval = static_evaluator.StaticEvaluator()
            static_value = eval.check_for_literals(parameter)

            if static_value == None:
                errors.append(parameter.at.format_message(f'parameter must be a literal'))

    errors += type_info.errors

def _validate_fn_call(step: parser.CutsceneStep, errors: list[str], context: variable_layout.VariableContext):
    _idx, local_fn = context.lookup_function(step.name.value)

    if local_fn:
        _validate_local_fn_call(step, errors, context, local_fn)
    else:
        _validate_system_call(step, errors, context)


def _can_assign_into_variable(name: tokenizer.Token, from_type: str, errors: list[str], context: variable_layout.VariableContext) -> bool:
    target_type = context.get_variable_type(name.value)

    if not target_type:
        errors.append(name.format_message('variable not defined'))
        return False
    
    target_type = expresion_generator.type_mapping[target_type]
    
    if target_type == from_type or expresion_generator.is_numerical_type(target_type) and expresion_generator.is_numerical_type(from_type):
        return True

    errors.append(name.format_message(f'cannot assign from {from_type} to type {target_type}'))
    return False

def _validate_step(step: parser.Statement, errors: list[str], context: variable_layout.VariableContext):
    if isinstance(step, parser.ReturnStatement):
        if len(step.results) != len(context.fn_locals.results):
            errors.append(step.return_token.format_message(f'expected {len(context.fn_locals.results)} got {len(step.results)}'))

        type_info = expresion_generator.TypeChecker(context)
        for index, result in enumerate(step.results):
            parameter_type = type_info.determine_type(result)
            if index < len(context.fn_locals.results) and not _can_assign(parameter_type, context.fn_locals.results[index]):
                errors.append(result.at.format_message(f'cannot assign type {parameter_type} to {context.fn_locals.results[index]}'))
            

    if isinstance(step, parser.CutsceneStep):
        _validate_fn_call(step, errors, context)
            
    if isinstance(step, parser.IfStatement):
        type_info = expresion_generator.TypeChecker(context)
        condition_type = type_info.determine_type(step.condition)
        errors += type_info.errors

        if not expresion_generator.is_numerical_type(condition_type):
            errors.append('condition should be an int or float')

        validate_steps(step.statements, errors, context)

    if isinstance(step, parser.Assignment):
        first_right = step.right[0]
        if len(step.right) == 1 and isinstance(first_right, parser.FunctionCall):
            idx, fn = context.lookup_function(first_right.name.value)
            type_info = expresion_generator.TypeChecker(context)
            type_info.determine_type(first_right)

            if fn:
                if len(fn.return_types) < len(step.left):
                    errors.append(step.left[0].format_message(f'expected {len(step.left)} results but got {len(fn.return_types)}'))

                for idx in range(min(len(fn.return_types), len(step.left))):
                    from_type = expresion_generator.type_mapping[fn.return_types[idx].name.value]
                    _can_assign_into_variable(step.left[idx], from_type, errors, context)

            errors += type_info.errors
        else:
            if len(step.right) != len(step.left):
                errors.append(step.left[0].format_message(f'unbalanced assignment left has {len(step.left)} right has {len(step.right)}'))

            type_info = expresion_generator.TypeChecker(context)

            for idx in range(min(len(step.right), len(step.left))):
                name = step.left[idx]
                value = step.right[idx]

                value_type = type_info.determine_type(value)
                
                _can_assign_into_variable(name, value_type, errors, context)

            errors += type_info.errors

    if isinstance(step, parser.VariableDefinition):
        type_info = expresion_generator.TypeChecker(context)

        value_type = None
        if step.initializer:
            value_type = type_info.determine_type(step.initializer)

        context.fn_locals.define_local(step)
        
        if value_type:
            _can_assign_into_variable(step.name, value_type, errors, context)

        errors += type_info.errors

def validate_steps(statements: list[parser.Statement], errors: list[str], context: variable_layout.VariableContext):
    for statement in statements:
        _validate_step(statement, errors, context)

def validate_cutscene(cutscene: parser.Cutscene, errors: list[str], context: variable_layout.VariableContext):
    for fn in cutscene.functions:
        local_context = context.with_locals(local_layout.LocalLayout(fn))
        validate_steps(fn.body, errors, local_context)

        if len(fn.return_types) > 0 and not _does_end_in_return(fn.body):
            errors.append(fn.name.format_message('not all code paths have a return statement'))

def _generate_step_block(cutscene: Cutscene, block: list, context: variable_layout.VariableContext, return_label: str):
    if context.fn_locals:
        context.fn_locals.start_block()
    
    for statement in block:
        _generate_step(cutscene, statement, context, return_label)

    if context.fn_locals:
        context.fn_locals.end_block()
        
def _generate_step(cutscene: Cutscene, step, context: variable_layout.VariableContext, return_label: str):
    if isinstance(step, parser.ReturnStatement):
        return_expr: expresion_generator.ExpressionCollection = expresion_generator.ExpressionCollection()

        for index, result in enumerate(step.results):
            return_expr = return_expr.concat(expresion_generator.generate_script(result, context, expresion_generator.type_mapping[context.fn_locals.results[index].name.value]))

        _generate_expression_collection(cutscene, return_expr, context)

        context.modify_stack_size(-len(step.results))
        cutscene.steps.append(JumpCutsceneStep(CUTSCENE_STEP_JUMP, return_label, step.return_token))

    elif isinstance(step, parser.CutsceneStep):
        if _generate_function_call(cutscene, step, 0, context):
            return

        if step.name.value in _aliases:
            _generate_alias(cutscene, step, _aliases[step.name.value], context, return_label)
            return

        _generate_function_step(cutscene, step, _step_args[step.name.value], context)

    elif isinstance(step, parser.IfStatement):
        expression = expresion_generator.generate_script(step.condition, context)
        _generate_expression_collection(cutscene, expression, context)

        context.modify_stack_size(-1)
        if_step = JumpCutsceneStep(CUTSCENE_STEP_JUMP_IF_NOT)
        cutscene.steps.append(if_step)
        size_before = len(cutscene.steps)
        
        _generate_step_block(cutscene, step.statements, context, return_label)

        if step.else_block:
            # setup the jump that skips over the else block
            # after the end of the if block
            skip_else = JumpCutsceneStep(CUTSCENE_STEP_JUMP)
            cutscene.steps.append(skip_else)

            # if there is an else block, skipping the if content should 
            # jump to the start of the else block
            if_step.offset = len(cutscene.steps) - size_before

            size_before = len(cutscene.steps)

            _generate_step_block(cutscene, step.else_block, context, return_label)

            # update the correct jump offset
            skip_else.offset = len(cutscene.steps) - size_before
        else:
            # update the correct jump offset
            if_step.offset = len(cutscene.steps) - size_before

    elif isinstance(step, parser.Assignment):
        first_right = step.right[0]

        if len(step.right) == 1 and isinstance(first_right, parser.FunctionCall):
            expression = expresion_generator.generate_script(first_right, context, retc = len(step.left))
            _generate_expression_collection(cutscene, expression, context, len(step.left))
        else:
            expression = expresion_generator.ExpressionCollection()
            
            for value in step.right:
                expression = expression.concat(expresion_generator.generate_script(value, context))
                
            _generate_expression_collection(cutscene, expression, context)
            
        for name in step.left:
            stack_pos = context.get_fn_local_offset(name.value)

            if stack_pos != None:
                if context.fn_locals.is_still_needed(name.value):
                    cutscene.steps.append(ExpressionCutsceneStep(
                        expresion_generator.ExpressionScript([expresion_generator.ExpressionStore(stack_pos)])
                    ))
                else:
                    cutscene.steps.append(ExpressionCutsceneStep(
                        expresion_generator.ExpressionScript([expresion_generator.ExpressionRemove(1)])
                    ))
            else:
                step_type = CUTSCENE_STEP_SET_GLOBAL

                if context.is_global(name.value):
                    step_type = CUTSCENE_STEP_SET_GLOBAL
                elif context.is_scene_var(name.value):
                    step_type = CUTSCENE_STEP_SET_SCENE

                var_type = context.get_variable_type(name.value)
                bit_offset = context.get_variable_offset(name.value)

                if not var_type:
                    raise Exception(f'could not get variable {name.value}')

                cutscene.steps.append(CutsceneStep(
                    step_type, 
                    expresion_generator.generate_variable_address(var_type, bit_offset)
                ))
                
            context.modify_stack_size(-1)

    elif isinstance(step, parser.VariableDefinition) and context.fn_locals:
        if step.initializer:
            expression = expresion_generator.generate_script(step.initializer, context)
        else:
            expression = expresion_generator.ExpressionCollection()
            expression.final_expression = expresion_generator.ExpressionScript([expresion_generator.ExpressionScriptIntLiteral(0)])
            context.modify_stack_size(1)

        context.fn_locals.define_local(step)
        stack_pos = context.fn_locals.get_local_stack_position(step.name.value)

        if stack_pos == None:
            raise Exception('Could not generate variable position')
        
        expression.final_expression = expresion_generator.expression_concat(expression.final_expression, expresion_generator.ExpressionScript([expresion_generator.ExpressionStore(stack_pos)]))
        context.modify_stack_size(-1)
        _generate_expression_collection(cutscene, expression, context)
        


def _idle_find_effected_actors(statements: list, actors: set):
    for statement in statements:
        if isinstance(statement, parser.IfStatement):
            _idle_find_effected_actors(statement.statements, actors)

        if isinstance(statement, parser.CutsceneStep) and statement.name.value in _steps_that_need_idle:
            actors.add(static_evaluator.StaticEvaluator().check_for_literals(statement.parameters[1]))

def _idle_effected_actors(cutscene: Cutscene, statements: list):
    actors = set[int]()

    _idle_find_effected_actors(statements, actors)

    for actor in actors:
        if actor == None:
            continue
        expression = expresion_generator.ExpressionScript([
            expresion_generator.ExpressionScriptIntLiteral(actor),
        ])
        cutscene.steps.append(ExpressionCutsceneStep(expression))
        cutscene.steps.append(CutsceneStep(CUTSCENE_STEP_IDLE_NPC, b''))

def _generate_statement_list_steps(cutscene: Cutscene, statements: list[parser.Statement], context: variable_layout.VariableContext, function_def: parser.FunctionDefinition):
    fn_locals = local_layout.LocalLayout(function_def)
    context = context.with_locals(fn_locals)

    function_name = function_def.name.value if function_def else ''

    return_label = f'$return_{function_name}'

    local_count = fn_locals.get_local_count()
    start_stack_size = fn_locals.get_stack_size()
    start_step_count = len(cutscene.steps)

    if local_count:
        cutscene.steps.append(ExpressionCutsceneStep(
            expresion_generator.ExpressionScript(list([expresion_generator.ExpressionScriptIntLiteral(0)] * local_count))
        ))

    _generate_step_block(cutscene, statements, context, return_label)

    cutscene.add_label(return_label)

    _idle_effected_actors(cutscene, statements)

    if fn_locals.get_stack_size() != start_stack_size:
        print(parser.statement_list_str(statements))
        raise Exception(f"fn {function_name} mismatched stack size expected {start_stack_size} got {fn_locals.get_stack_size()}")
        
    cutscene.simplify(start_step_count)


def generate_steps(file, inputCutscene: parser.Cutscene, context: variable_layout.VariableContext):
    cutscene = Cutscene()

    fn_length: list[int] = []

    for fn in inputCutscene.functions:
        before = cutscene.step_count()
        _generate_statement_list_steps(cutscene, fn.body, context, fn)
        fn_length.append(cutscene.step_count() - before)

        print(f'func {fn.name.value} len {cutscene.step_count() - before}')

    errors = cutscene.apply_jump_labels()

    if len(errors) > 0:
        message = '\n\n'.join(errors)
        print(message)
        raise Exception(message)

    file.write('CTSN'.encode())

    file.write(struct.pack('>HH', cutscene.step_count(), len(inputCutscene.functions)))

    for step in cutscene.steps:
        if isinstance(step, LabelStep):
            continue
        print(step)
        file.write(struct.pack('>B', step.command))
        step.write_data(file)

    for length, fn in zip(fn_length, inputCutscene.functions):
        name_bytes = fn.name.value.encode()

        file.write(struct.pack('>HBBB', length, len(fn.args), len(fn.return_types), len(name_bytes)))
        file.write(name_bytes)

