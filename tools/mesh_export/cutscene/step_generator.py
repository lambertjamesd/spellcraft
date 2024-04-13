
from . import parser

def _generate_say(step: parser.CutsceneStep):
    pass


_step_functions = {
    "say": _generate_say,
}

_step_args = {
    "say": ["tstr"]
}

def validate_step(step, errors: list[str]):
    if isinstance(step, parser.CutsceneStep):
        if not step.name.value in _step_args:
            errors.append(step.name.format_message(f'{step.name.value} is not a valid step name'))
            return
        
        


def generate_step(step):

    pass