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

CUTSCENE_STEP_EXPRESSION = 0
CUTSCENE_STEP_JUMP_IF_NOT = 1
CUTSCENE_STEP_JUMP = 2
CUTSCENE_STEP_SET_SCENE = 3
CUTSCENE_STEP_SET_GLOBAL = 4
CUTSCENE_STEP_CALLBACK = 5
CUTSCENE_STEP_TEMPLATE_STRING = 6
CUTSCENE_STEP_FUNCTION_CALL = 7
CUTSCENE_STEP_BUILT_IN_FN = 8

class Alias():
    def __init__(self, source: str):
        self.source: str = source

_aliases: dict[str, Alias] = {
    "sign_start": Alias("pause; look_at_subject; cam_look_npc ENTITY_ID_SUBJECT;"),
    "sign_end": Alias("unpause; cam_return;"),
}

def _encode_string(string: str) -> bytes:
    encoded = string.encode()
    length = len(encoded)

    if length < 128:
        return struct.pack('>B', length) + encoded
    else:
        return struct.pack('>BB', (length & 127) | 128, length // 128) + encoded

_reverse_step_ids = {
    CUTSCENE_STEP_EXPRESSION: 'expr',
    CUTSCENE_STEP_JUMP_IF_NOT: 'jump if not',
    CUTSCENE_STEP_JUMP: 'jump',
    CUTSCENE_STEP_SET_SCENE: 'set scene var',
    CUTSCENE_STEP_SET_GLOBAL: 'set global',
    CUTSCENE_STEP_CALLBACK: 'callback',
    CUTSCENE_STEP_TEMPLATE_STRING: 'str',
    CUTSCENE_STEP_FUNCTION_CALL: 'fn',
    CUTSCENE_STEP_BUILT_IN_FN: 'built in fn',
}

class CutsceneStep():
    def __init__(self, command: int, data: bytes):
        self.command: int = command
        self.data: bytes = data

    def write_data(self, file):
        file.write(self.data)

    def __repr__(self):
        return f'<{_reverse_step_ids[self.command] if self.command in _reverse_step_ids else self.command} {self.data}>'

class JumpCutsceneStep():
    def __init__(self, command: int, label: str | None = None, token: tokenizer.Token | None = None):
        self.command: int = command
        self.offset: int = 0
        self.label: str | None = label
        self.token: tokenizer.Token | None = token

    def write_data(self, file):
        file.write(struct.pack('>h', self.offset))

    def __repr__(self):
        return f'<jump {self.label} ({self.offset})>'

class ExpressionTemplateString():
    def __init__(self, template: str, argc: int):
        self.command: int = CUTSCENE_STEP_TEMPLATE_STRING
        self.template: str = template
        self.argc: int = argc
        
    def write_data(self, file):
        file.write(self.argc.to_bytes(1, 'big'))
        file.write(_encode_string(self.template))
        
    def __repr__(self):
        return f'<str {self.template}>'

class ExpressionCutsceneStep():
    def __init__(self, expr: expresion_generator.ExpressionScript):
        self.command: int = CUTSCENE_STEP_EXPRESSION
        self.expr: expresion_generator.ExpressionScript = expr
        
    def write_data(self, file):
        self.expr.serialize(file)
        
    def __repr__(self):
        return f'<expr {repr(self.expr)}>'

class ExpressionFunctionCall():
    def __init__(self, fn_index: int, argc: int, retc: int, built_in: bool):
        self.command: int = CUTSCENE_STEP_BUILT_IN_FN if built_in else CUTSCENE_STEP_FUNCTION_CALL
        self.fn_index = fn_index
        self.argc: int = argc
        self.retc: int = retc

    def write_data(self, file):
        file.write(struct.pack('>HBB', self.fn_index, self.argc, self.retc))
    
    def __repr__(self):
        return f'<fn({self.fn_index}, {self.argc}, {self.retc})>'

class LabelStep():
    def __init__(self, name: str):
        self.name: str = name

StepTypes = typing.Union[
    CutsceneStep, 
    JumpCutsceneStep, 
    ExpressionTemplateString, 
    ExpressionCutsceneStep, 
    ExpressionFunctionCall, 
    LabelStep,
]

class Cutscene():
    def __init__(self):
        self.steps: list[StepTypes] = []
        self._label_count: int = 0

    def add_step(self, step: StepTypes):
        self.steps.append(step)

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
            
def build_template_string(string: parser.String, context: variable_layout.VariableContext) -> str:
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
            cutscene.add_step(ExpressionCutsceneStep(chunk.script))

        if isinstance(chunk.sync_step, parser.FunctionCall):
            fn_index, fn = context.lookup_function(chunk.sync_step.name.value)

            if not fn:
                raise Exception(f'could not find function {chunk.sync_step.name.value}')

            cutscene.add_step(ExpressionFunctionCall(
                fn_index, 
                len(chunk.sync_step.args), 
                retc if chunk == collection.chunks[-1] else 1,
                fn.built_in
            ))
        elif isinstance(chunk.sync_step, parser.String):
            cutscene.add_step(ExpressionTemplateString(build_template_string(chunk.sync_step, context), len(chunk.sync_step.replacements)))
        else:
            raise Exception(f'bad chunk sync_step {chunk.sync_step}')

    if collection.final_expression:
        cutscene.add_step(ExpressionCutsceneStep(collection.final_expression))

def _generate_function_call(cutscene: Cutscene, step: parser.CutsceneStep, expected_count: int, context: variable_layout.VariableContext) -> bool:
    fn_index, fn = context.lookup_function(step.name.value)

    if not fn:
        return False
    
    pre_expression: expresion_generator.ExpressionCollection = expresion_generator.ExpressionCollection()

    for arg_index, arg in enumerate(step.parameters):
        generate_to = 'int'
        expected_type = fn.args[arg_index].type_name.name.value

        if expected_type == 'float':
            generate_to = 'float'
        elif expected_type == 'str':
            generate_to = 'str'

        pre_expression = pre_expression.concat(expresion_generator.generate_script(arg, context, generate_to))

    _generate_expression_collection(cutscene, pre_expression, context)

    cutscene.add_step(ExpressionFunctionCall(
        fn_index, 
        len(step.parameters), 
        expected_count,
        fn.built_in
    ))
    context.modify_stack_size(expected_count - len(step.parameters))

    return True

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

    if step.name.value in _aliases:
        args = []

    if args == None:
        errors.append(step.name.format_message(f'{step.name.value} is not a valid step name'))
        return

    if len(args) != len(step.parameters):
        errors.append(step.name.format_message(f'incorrect number of parameters got {len(step.parameters)} expected {len(args)}'))

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
    
    if not target_type in expresion_generator.type_mapping:
        errors.append(name.format_message(f'unknown variable type {target_type}'))
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
        cutscene.add_step(JumpCutsceneStep(CUTSCENE_STEP_JUMP, return_label, step.return_token))

    elif isinstance(step, parser.CutsceneStep):
        if _generate_function_call(cutscene, step, 0, context):
            return

        if step.name.value in _aliases:
            _generate_alias(cutscene, step, _aliases[step.name.value], context, return_label)
            return

        raise Exception(f'could not find function {step.name.value}')

    elif isinstance(step, parser.IfStatement):
        expression = expresion_generator.generate_script(step.condition, context)
        _generate_expression_collection(cutscene, expression, context)

        context.modify_stack_size(-1)
        if_step = JumpCutsceneStep(CUTSCENE_STEP_JUMP_IF_NOT)
        cutscene.add_step(if_step)
        size_before = cutscene.step_count()
        
        _generate_step_block(cutscene, step.statements, context, return_label)

        if step.else_block:
            # setup the jump that skips over the else block
            # after the end of the if block
            skip_else = JumpCutsceneStep(CUTSCENE_STEP_JUMP)
            cutscene.add_step(skip_else)

            # if there is an else block, skipping the if content should 
            # jump to the start of the else block
            if_step.offset = cutscene.step_count() - size_before

            size_before = cutscene.step_count()

            _generate_step_block(cutscene, step.else_block, context, return_label)

            # update the correct jump offset
            skip_else.offset = cutscene.step_count() - size_before
        else:
            # update the correct jump offset
            if_step.offset = cutscene.step_count() - size_before

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
            
        # start at the top of the stack
        for name in reversed(step.left):
            context.modify_stack_size(-1)
            stack_pos = context.get_fn_local_offset(name.value)

            if stack_pos != None:
                if context.fn_locals.is_still_needed(name.value):
                    cutscene.add_step(ExpressionCutsceneStep(
                        expresion_generator.ExpressionScript([expresion_generator.ExpressionStore(stack_pos)])
                    ))
                else:
                    cutscene.add_step(ExpressionCutsceneStep(
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

                cutscene.add_step(CutsceneStep(
                    step_type, 
                    expresion_generator.generate_variable_address(var_type, bit_offset)
                ))

    elif isinstance(step, parser.VariableDefinition) and context.fn_locals:
        if step.initializer:
            expression = expresion_generator.generate_script(step.initializer, context)
        else:
            expression = expresion_generator.ExpressionCollection()
            expression.final_expression = expresion_generator.ExpressionScript([expresion_generator.ExpressionScriptIntLiteral(0)])
            context.modify_stack_size(1)

        context.modify_stack_size(-1)
        context.fn_locals.define_local(step)
        stack_pos = context.get_fn_local_offset(step.name.value)

        if stack_pos == None:
            raise Exception('Could not generate variable position')
        
        expression.final_expression = expresion_generator.expression_concat(expression.final_expression, expresion_generator.ExpressionScript([expresion_generator.ExpressionStore(stack_pos)]))
        _generate_expression_collection(cutscene, expression, context)

def _generate_statement_list_steps(cutscene: Cutscene, statements: list[parser.Statement], context: variable_layout.VariableContext, function_def: parser.FunctionDefinition):
    fn_locals = local_layout.LocalLayout(function_def)
    context = context.with_locals(fn_locals)

    function_name = function_def.name.value if function_def else ''

    return_label = f'$return_{function_name}'

    local_count = fn_locals.get_local_count()
    start_stack_size = fn_locals.get_stack_size()
    start_step_count = cutscene.step_count()

    if local_count:
        cutscene.add_step(ExpressionCutsceneStep(
            expresion_generator.ExpressionScript(list([expresion_generator.ExpressionScriptIntLiteral(0)] * local_count))
        ))

    _generate_step_block(cutscene, statements, context, return_label)

    cutscene.add_label(return_label)

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
        file.write(struct.pack('>B', step.command))
        step.write_data(file)

    for length, fn in zip(fn_length, inputCutscene.functions):
        name_bytes = fn.name.value.encode()

        file.write(struct.pack('>HBBB', length, len(fn.args), len(fn.return_types), len(name_bytes)))
        file.write(name_bytes)

