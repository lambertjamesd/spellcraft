
from . import parser
import typing

VariableType = typing.Union[parser.VariableDefinition, parser.FunctionDefinitionArg]

class VariableScope:
    def __init__(self):
        self.name_to_definition: dict[str, VariableType] = {}

class VariableScopeStack:
    def __init__(self):
        self.scopes: list[VariableScope] = []

    def define_variable(self, variable: VariableType):
        if len(self.scopes) == 0:
            raise Exception('Tried to define a variable without a scope')
        
        self.scopes[-1].name_to_definition[variable.name.value] = variable

    def lookup_variable(self, name: str) -> VariableType | None:
        for scope in reversed(self.scopes):
            if name in scope.name_to_definition:
                return scope.name_to_definition[name]
            
        return None

    def start_block(self):
        self.scopes.append(VariableScope())

    def end_block(self):
        self.scopes.pop()

class VariableRangeTracker:
    def __init__(self):
        self.variable_scopes = VariableScopeStack()
        self.all_variables: list[VariableType] = []
        self.variable_overlaps: dict[VariableType, set[VariableType]] = {}

    def start_block(self):
        self.variable_scopes.start_block()

    def end_block(self):
        self.variable_scopes.end_block()

    def define_variable(self, variable: VariableType):
        self.variable_scopes.define_variable(variable)
        self.all_variables.append(variable)
        self.variable_overlaps[variable] = set()

    def mark_use(self, variable_name: str):
        use_definition = self.variable_scopes.lookup_variable(variable_name)

        if not use_definition:
            return
        
        for other in reversed(self.all_variables):
            if other == use_definition:
                break

            self.variable_overlaps[other].add(use_definition)

    def layout_variables(self, argc: int) -> tuple[dict[VariableType, int], int]:
        result: dict[VariableType, int] = {}
        slots: list[VariableType] = []

        for index, var in enumerate(self.all_variables):
            if index < argc:
                result[var] = len(slots)
                slots.append(var)
                continue

            target_index = len(slots)

            for existing_index, existing in enumerate(slots):
                if not existing in self.variable_overlaps[var]:
                    target_index = existing_index
                    break

            result[var] = target_index
            if target_index == len(slots):
                slots.append(var)
            else:
                slots[target_index] = var

        return result, len(slots)


def _searchRangesInExpression(expr: parser.Expression, tracker: VariableRangeTracker):
    if isinstance(expr, parser.Identifier):
        tracker.mark_use(expr.name.value)
    elif isinstance(expr, parser.FunctionCall):
        for arg in expr.args:
            _searchRangesInExpression(arg, tracker)
    elif isinstance(expr, parser.UnaryOperator):
        _searchRangesInExpression(expr.operand, tracker)
    elif isinstance(expr, parser.BinaryOperator):
        _searchRangesInExpression(expr.a, tracker)
        _searchRangesInExpression(expr.b, tracker)

def _searchRangesInStep(step: parser.Statement, tracker: VariableRangeTracker):
    if isinstance(step, parser.CutsceneStep):
        for param in step.parameters:
            _searchRangesInExpression(param, tracker)
    elif isinstance(step, parser.IfStatement):
        _searchRangesInExpression(step.condition, tracker)

        _searchRangesInBlock(step.statements, tracker)

        if step.else_block:
            _searchRangesInBlock(step.else_block, tracker)
    elif isinstance(step, parser.Assignment):
        tracker.mark_use(step.name.value)
        _searchRangesInExpression(step.value, tracker)
    elif isinstance(step, VariableType):
        if step.initializer:
            _searchRangesInExpression(step.initializer, tracker)
        tracker.define_variable(step)
        

def _searchRangesInBlock(block: list[parser.Statement], tracker: VariableRangeTracker):
    tracker.start_block()

    for step in block:
        _searchRangesInStep(step, tracker)

    tracker.end_block()

def _determineRanges(block: list[parser.Statement]) -> tuple[dict[VariableType, int], int]:
    tracker = VariableRangeTracker()
    _searchRangesInBlock(block, tracker)
    return tracker.layout_variables(0)

class LocalLayout:
    def __init__(self, block: list[parser.Statement]):
        self.variable_scopes = VariableScopeStack()
        var_to_pos, stack_size = _determineRanges(block)
        self._var_to_pos: dict[VariableType, int] = var_to_pos
        self.local_stack_size = stack_size
        self.slots: list[VariableType | None] = [None] * stack_size

        self.current_stack_pos = stack_size

    def get_local_count(self) -> int:
        return self.local_stack_size # - self.argc

    def get_stack_size(self) -> int:
        return self.current_stack_pos
    
    def modify_stack(self, amount: int):
        self.current_stack_pos += amount

        if self.current_stack_pos < 0:
            raise Exception(f"stack underflow")

    def start_block(self):
        self.variable_scopes.start_block()

    def end_block(self):
        self.variable_scopes.end_block()

    def define_local(self, definition: VariableType):
        self.variable_scopes.define_variable(definition)
        self.slots[self._var_to_pos[definition]] = definition

    def get_local_stack_position(self, name: str) -> int | None:
        var_def = self.variable_scopes.lookup_variable(name)

        if var_def == None:
            return None

        if var_def in self._var_to_pos:
            return self._var_to_pos[var_def]

        return None
    
    def get_variable_type(self, name: str) -> str | None:
        var_def = self.variable_scopes.lookup_variable(name)

        if var_def == None:
            return None
        
        if isinstance(var_def, parser.VariableDefinition):
            return var_def.type.name.value
        if isinstance(var_def, parser.FunctionDefinitionArg):
            return var_def.type_name.name.value
        
        raise Exception(f"could not get_variable_type on {var_def}")
    
    def is_still_needed(self, name: str) -> bool:
        var_def = self.variable_scopes.lookup_variable(name)

        if var_def == None:
            return True
        
        return self.slots[self._var_to_pos[var_def]] == var_def
        

    
    