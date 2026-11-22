# Spellcraft Script for VS Code

Language support for the cutscene `.script` files (the language implemented in
`tools/mesh_export/cutscene/tokenizer.py` and `parser.py`).

## Features

- **Syntax highlighting**: keywords, types, declarations, built-in steps, function
  calls, enum constants, numbers, comments, and strings with `{expr}` / `{expr:format}`
  interpolation.
- **Autocomplete** based on where the cursor is:
  - at the start of a statement: built-in steps (inserted with argument placeholders),
    script functions, variables and statement keywords
  - in expressions: variables in scope (globals, scene vars, locals, parameters),
    built-in expression functions such as `are_touching(...)`, script functions that
    return values, enum constants such as `ENTITY_ID_PLAYER`, and `and`/`or`/`not`/`true`/`false`
  - after `:` in a declaration or function signature: types
  - inside `{ }` in a string: expressions
- **Signature help** for steps (`npc_animate npc, animation, loop;`) and function calls.
- **Hover** information and **Go to Definition** for variables, functions, built-ins
  (jumps to the `// func` comment in the C source) and enum constants.
- **Outline / breadcrumbs** for functions and global/scene variables.
- Snippets: `func`, `main`, `if`, `ifelse`, `global`, `scene`, `local`, `saym`.

## Where the data comes from

The extension reads these files from the project on demand (re-reading when they change),
so new steps and enums show up without rebuilding the extension:

| Data                        | Source                                                    |
| --------------------------- | --------------------------------------------------------- |
| Built-in steps              | `// func` comments in `src/cutscene/cutscene_step_fn.c`   |
| Built-in expression funcs   | `// func` comments in `src/cutscene/expression_fn.c`      |
| Enum constants              | `src/scene/scene_definition.h`                            |
| Global variables            | `assets/scripts/globals.script`                           |

If the C sources are missing it falls back to `build/cutscene/function_defs.json`.

The project root is found by walking up from the open file until a folder containing
`src/cutscene` is found. Override it with the `spellcraftScript.projectRoot` setting.

## Installing

There is no build step. Either symlink the folder into your extensions directory and
reload VS Code:

```sh
# VS Code running against WSL (Remote - WSL)
ln -s "$(pwd)/tools/vscode-spellcraft-script" ~/.vscode-server/extensions/spellcraft.spellcraft-script-0.1.0

# Local VS Code on Linux / macOS
ln -s "$(pwd)/tools/vscode-spellcraft-script" ~/.vscode/extensions/spellcraft.spellcraft-script-0.1.0
```

or package it into a `.vsix` and install that:

```sh
cd tools/vscode-spellcraft-script
npx @vscode/vsce package --allow-missing-repository
code --install-extension spellcraft-script-0.1.0.vsix
```

To work on the extension itself, open this folder in VS Code and press F5 to launch an
Extension Development Host.
