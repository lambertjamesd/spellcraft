# features
the ablity to embed variables into text
global variables/local variables
simple computations
conditional branching/loops
show dialog
run arbitrary cutscene steps
animation

## the ablity to embed variables into text

```
dialog "A text block with a {variable} embedded"
```

## global variables

```
global variable: bool
global variable: i32
```

## local varaibles

```
local variable: bool
```

## setting variables

```
variable = expression
```

# simple computations

operators and, or, not, +, -, *, /, ==, !=, >, <, <=, >=

lisp? (and a b (or c d))
infix? a and b and (c or d)

# conditional

```
if condition then
    actions
end
```

# dialog

```
say "A message of text"
dialog "A message of text"

dialog """
    This is a how dialog that has mulitple lines will look

    This should show up after pressing a
"""

```

# cutscene steps

steps will look like function calls with comma separated values

```
look_at player, active_npc
run_animation intro
walk_to active_npc, exit

```