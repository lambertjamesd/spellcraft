// Language support for spellcraft cutscene .script files.
//
// Mirrors the language implemented by tools/mesh_export/cutscene/tokenizer.py
// and tools/mesh_export/cutscene/parser.py.
//
// Project data is read straight from the repo so it stays in sync:
//   - built-in steps:            src/cutscene/cutscene_step_fn.c  (`// func name(arg: type): ret` comments)
//   - built-in expr functions:   src/cutscene/expression_fn.c
//     (falls back to build/cutscene/function_defs.json if the sources are missing)
//   - enum constants:            src/scene/scene_definition.h
//   - global variables:          assets/scripts/globals.script

const vscode = require('vscode');
const fs = require('fs');
const path = require('path');

const LANGUAGE_ID = 'spellcraft-script';

const KEYWORDS = ['func', 'end', 'if', 'then', 'elif', 'else', 'return', 'local', 'global', 'scene', 'and', 'or', 'not', 'true', 'false'];
const STATEMENT_KEYWORDS = ['if', 'elif', 'else', 'end', 'return', 'local', 'global', 'scene', 'func'];
const EXPRESSION_KEYWORDS = ['and', 'or', 'not', 'true', 'false'];
const BLOCK_START = new Set(['func', 'if']);

const TYPES = {
    bool: 'boolean',
    i8: '8 bit signed integer',
    i16: '16 bit signed integer',
    i32: '32 bit signed integer',
    float: '32 bit float',
    char: 'character (use as an array, e.g. char[8], for fixed length strings)',
    str: 'string',
    entity_id: 'entity id (16 bit)',
    entity_spawner: 'entity spawner (32 bit)',
};

const IDENTIFIER = /[A-Za-z_][A-Za-z0-9_]*/;

// ---------------------------------------------------------------------------
// Tokenizer (simplified port of tokenizer.py, but strings are single tokens and
// comments run to the end of the line)
// ---------------------------------------------------------------------------

function tokenize(text) {
    const tokens = [];
    const n = text.length;
    let i = 0;

    const isIdentStart = (c) => /[A-Za-z_]/.test(c);
    const isIdent = (c) => /[A-Za-z0-9_]/.test(c);
    const isDigit = (c) => c >= '0' && c <= '9';

    while (i < n) {
        const c = text[i];

        if (/\s/.test(c)) {
            i++;
            continue;
        }

        const start = i;

        if ((c === '/' && text[i + 1] === '/') || c === '#') {
            let end = text.indexOf('\n', i);
            if (end === -1) end = n;
            tokens.push({ type: 'comment', value: text.slice(start, end), start, end });
            i = end;
            continue;
        }

        if (isIdentStart(c)) {
            while (i < n && isIdent(text[i])) i++;
            tokens.push({ type: 'identifier', value: text.slice(start, i), start, end: i });
            continue;
        }

        if (isDigit(c) || (c === '.' && isDigit(text[i + 1] || ''))) {
            let isFloat = false;
            while (i < n && (isDigit(text[i]) || (text[i] === '.' && !isFloat))) {
                if (text[i] === '.') isFloat = true;
                i++;
            }
            tokens.push({ type: isFloat ? 'float' : 'int', value: text.slice(start, i), start, end: i });
            continue;
        }

        if (c === '"') {
            i++;
            let terminated = false;
            let braceDepth = 0;
            while (i < n) {
                const ch = text[i];
                if (ch === '\\' && text[i + 1] === '"') {
                    i += 2;
                    continue;
                }
                if (ch === '{') braceDepth++;
                else if (ch === '}' && braceDepth > 0) braceDepth--;
                else if (ch === '"' && braceDepth === 0) {
                    i++;
                    terminated = true;
                    break;
                }
                i++;
            }
            tokens.push({ type: 'string', value: text.slice(start, i), start, end: i, terminated, braceDepth });
            continue;
        }

        const two = text.slice(i, i + 2);
        if (two === '==' || two === '!=' || two === '>=' || two === '<=') {
            i += 2;
            tokens.push({ type: two, value: two, start, end: i });
            continue;
        }

        i++;
        tokens.push({ type: c, value: c, start, end: i });
    }

    return tokens;
}

// ---------------------------------------------------------------------------
// Document analysis
// ---------------------------------------------------------------------------

function typeToString(type) {
    if (!type) return '?';
    return type.count != null ? `${type.name}[${type.count}]` : type.name;
}

function signatureToString(fn) {
    const args = fn.args.map((arg) => `${arg.name}: ${typeToString(arg.type)}`).join(', ');
    const ret = fn.returns.length ? `: ${fn.returns.map(typeToString).join(', ')}` : '';
    return `func ${fn.name}(${args})${ret}`;
}

function stepSignatureToString(fn) {
    const args = fn.args.map((arg) => `${arg.name}: ${typeToString(arg.type)}`).join(', ');
    const ret = fn.returns.length ? ` -> ${fn.returns.map(typeToString).join(', ')}` : '';
    return `${fn.name}${args ? ' ' + args : ''};${ret}`;
}

/**
 * Pulls declarations out of a script.
 * Returns { functions: Map<name, fn>, variables: [{ name, type, scope, start, end, fn }] }
 */
function analyze(text) {
    const tokens = tokenize(text).filter((t) => t.type !== 'comment');
    const functions = new Map();
    const variables = [];

    let i = 0;
    let depth = 0;
    let currentFn = null;

    const peek = (offset = 0) => tokens[i + offset];
    const isIdent = (tok, value) => tok && tok.type === 'identifier' && (value === undefined || tok.value === value);

    function parseType() {
        const name = peek();
        if (!isIdent(name)) return null;
        i++;
        let count = null;
        if (peek() && peek().type === '[') {
            i++;
            if (peek() && peek().type === 'int') {
                count = parseInt(peek().value, 10);
                i++;
            }
            if (peek() && peek().type === ']') i++;
        }
        return { name: name.value, count };
    }

    while (i < tokens.length) {
        const tok = peek();

        if (tok.type !== 'identifier') {
            i++;
            continue;
        }

        if ((tok.value === 'global' || tok.value === 'scene' || tok.value === 'local') && isIdent(peek(1)) && peek(2) && peek(2).type === ':') {
            const name = peek(1);
            i += 3;
            const type = parseType();
            variables.push({
                name: name.value,
                type,
                scope: tok.value,
                start: name.start,
                end: name.end,
                declStart: tok.start,
                fn: tok.value === 'local' ? currentFn : null,
            });
            continue;
        }

        if (tok.value === 'func' && isIdent(peek(1))) {
            const name = peek(1);
            const fn = { name: name.value, args: [], returns: [], start: name.start, end: name.end, declStart: tok.start, bodyEnd: text.length };
            i += 2;

            if (peek() && peek().type === '(') {
                i++;
                while (peek() && peek().type !== ')') {
                    if (isIdent(peek()) && peek(1) && peek(1).type === ':') {
                        const argName = peek();
                        i += 2;
                        const type = parseType();
                        fn.args.push({ name: argName.value, type });
                        variables.push({ name: argName.value, type, scope: 'param', start: argName.start, end: argName.end, declStart: argName.start, fn });
                    } else if (peek().type === 'identifier' && BLOCK_START.has(peek().value)) {
                        break; // malformed header, bail out
                    } else {
                        i++;
                    }
                }
                if (peek() && peek().type === ')') i++;
                if (peek() && peek().type === ':') {
                    i++;
                    for (;;) {
                        const type = parseType();
                        if (!type) break;
                        fn.returns.push(type);
                        if (peek() && peek().type === ',') i++;
                        else break;
                    }
                }
            }

            if (!functions.has(fn.name)) functions.set(fn.name, fn);
            currentFn = fn;
            depth = 1;
            continue;
        }

        if (tok.value === 'if') {
            depth++;
        } else if (tok.value === 'end') {
            depth--;
            if (depth <= 0) {
                depth = 0;
                if (currentFn) currentFn.bodyEnd = tok.end;
                currentFn = null;
            }
        }

        i++;
    }

    return { functions, variables };
}

const analysisCache = new Map();

function analyzeDocument(document) {
    const key = document.uri.toString();
    const cached = analysisCache.get(key);
    if (cached && cached.version === document.version) return cached.result;
    const result = analyze(document.getText());
    analysisCache.set(key, { version: document.version, result });
    return result;
}

// ---------------------------------------------------------------------------
// Project data (built-ins, enums, globals)
// ---------------------------------------------------------------------------

const fileCache = new Map();

/** Reads a file and parses it, re-parsing only when the mtime changes. */
function readCached(filePath, parseFn) {
    let stat;
    try {
        stat = fs.statSync(filePath);
    } catch (e) {
        fileCache.delete(filePath);
        return null;
    }
    const cached = fileCache.get(filePath);
    if (cached && cached.mtimeMs === stat.mtimeMs) return cached.value;
    let value = null;
    try {
        value = parseFn(fs.readFileSync(filePath, 'utf8'));
    } catch (e) {
        console.error(`spellcraft-script: failed to parse ${filePath}`, e);
    }
    fileCache.set(filePath, { mtimeMs: stat.mtimeMs, value });
    return value;
}

function isProjectRoot(dir) {
    try {
        return fs.statSync(path.join(dir, 'src', 'cutscene')).isDirectory();
    } catch (e) {
        return false;
    }
}

function findProjectRoot(document) {
    const configured = vscode.workspace.getConfiguration('spellcraftScript').get('projectRoot');
    if (configured) {
        const folder = vscode.workspace.workspaceFolders && vscode.workspace.workspaceFolders[0];
        return path.isAbsolute(configured) || !folder ? configured : path.join(folder.uri.fsPath, configured);
    }

    if (document && document.uri.scheme === 'file') {
        let dir = path.dirname(document.uri.fsPath);
        for (;;) {
            if (isProjectRoot(dir)) return dir;
            const parent = path.dirname(dir);
            if (parent === dir) break;
            dir = parent;
        }
    }

    for (const folder of vscode.workspace.workspaceFolders || []) {
        if (isProjectRoot(folder.uri.fsPath)) return folder.uri.fsPath;
    }

    return null;
}

function parseTypeString(str) {
    const match = /^\s*([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[\s*(\d+)\s*\])?\s*$/.exec(str);
    if (!match) return { name: str.trim(), count: null };
    return { name: match[1], count: match[2] ? parseInt(match[2], 10) : null };
}

/** Parses `// func name(arg: type, ...): ret, ret` comments, same as generate_function_defs.py */
function parseFuncComments(source, filePath) {
    const result = [];
    const pattern = /\/\/\s*func\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\)\s*(?::([^\n]*))?/g;
    let match;
    while ((match = pattern.exec(source))) {
        const args = match[2]
            .split(',')
            .map((s) => s.trim())
            .filter(Boolean)
            .map((arg) => {
                const colon = arg.indexOf(':');
                return colon === -1
                    ? { name: arg, type: null }
                    : { name: arg.slice(0, colon).trim(), type: parseTypeString(arg.slice(colon + 1)) };
            });
        const returns = (match[3] || '')
            .split(',')
            .map((s) => s.trim())
            .filter(Boolean)
            .map(parseTypeString);
        const line = source.slice(0, match.index).split('\n').length - 1;
        result.push({ name: match[1], args, returns, file: filePath, line });
    }
    return result;
}

function parseFunctionDefsJson(source, filePath) {
    const json = JSON.parse(source);
    const convert = (def) => ({
        name: def.name,
        args: def.args.map((arg) => ({ name: arg.name, type: arg.type_name })),
        returns: def.return_types,
        file: filePath,
        line: null,
    });
    return { steps: json.step_defs.map(convert), exprs: json.expr_defs.map(convert) };
}

function parseEnums(source, filePath) {
    const constants = new Map();
    const stripped = source.replace(/\/\*[\s\S]*?\*\//g, (m) => m.replace(/[^\n]/g, ' '));
    const pattern = /^enum\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{([\s\S]*?)\}/gm;
    let match;
    while ((match = pattern.exec(stripped))) {
        const enumName = match[1];
        const bodyOffset = match.index + match[0].indexOf('{') + 1;
        let next = 0;
        let offset = bodyOffset;
        for (const rawEntry of match[2].split(',')) {
            const entryOffset = offset;
            offset += rawEntry.length + 1;
            const entry = rawEntry.replace(/\/\/.*$/gm, '').trim();
            const entryMatch = /^([A-Za-z_][A-Za-z0-9_]*)\s*(?:=\s*(.+))?$/s.exec(entry);
            if (!entryMatch) continue;
            const name = entryMatch[1];
            let value = null;
            if (entryMatch[2] !== undefined) {
                const literal = entryMatch[2].trim();
                if (/^-?(0x[0-9a-fA-F]+|\d+)$/.test(literal)) value = Number(literal);
                else if (constants.has(literal)) value = constants.get(literal).value;
            } else if (next !== null) {
                value = next;
            }
            next = value === null ? null : value + 1;
            const nameIndex = entryOffset + rawEntry.indexOf(name);
            const line = source.slice(0, nameIndex).split('\n').length - 1;
            constants.set(name, { name, value, enumName, file: filePath, line });
        }
    }
    constants.set('true', { name: 'true', value: 1, enumName: null, file: null, line: null });
    constants.set('false', { name: 'false', value: 0, enumName: null, file: null, line: null });
    return constants;
}

function getProjectData(document) {
    const root = findProjectRoot(document);
    const empty = { root: null, steps: new Map(), exprs: new Map(), constants: new Map(), globals: [], globalsFile: null };
    if (!root) return empty;

    const stepFile = path.join(root, 'src', 'cutscene', 'cutscene_step_fn.c');
    const exprFile = path.join(root, 'src', 'cutscene', 'expression_fn.c');
    const defsFile = path.join(root, 'build', 'cutscene', 'function_defs.json');
    const enumFile = path.join(root, 'src', 'scene', 'scene_definition.h');
    const globalsFile = path.join(root, 'assets', 'scripts', 'globals.script');

    let steps = readCached(stepFile, (src) => parseFuncComments(src, stepFile));
    let exprs = readCached(exprFile, (src) => parseFuncComments(src, exprFile));
    if (!steps || !exprs) {
        const defs = readCached(defsFile, (src) => parseFunctionDefsJson(src, defsFile));
        if (defs) {
            steps = steps || defs.steps;
            exprs = exprs || defs.exprs;
        }
    }

    const toMap = (list) => new Map((list || []).map((fn) => [fn.name, fn]));

    const globalsAnalysis = readCached(globalsFile, (src) => ({ src, ...analyze(src) }));
    const globals = globalsAnalysis
        ? globalsAnalysis.variables
              .filter((v) => v.scope === 'global')
              .map((v) => ({ ...v, file: globalsFile, line: globalsAnalysis.src.slice(0, v.start).split('\n').length - 1 }))
        : [];

    return {
        root,
        steps: toMap(steps),
        exprs: toMap(exprs),
        constants: readCached(enumFile, (src) => parseEnums(src, enumFile)) || new Map(),
        globals,
        globalsFile,
    };
}

// ---------------------------------------------------------------------------
// Scope helpers
// ---------------------------------------------------------------------------

function functionAt(analysis, offset) {
    for (const fn of analysis.functions.values()) {
        if (offset >= fn.declStart && offset <= fn.bodyEnd) return fn;
    }
    return null;
}

/** All variables visible at offset, later declarations in the file win over globals.script */
function visibleVariables(analysis, project, offset) {
    const fn = functionAt(analysis, offset);
    const result = new Map();
    for (const v of project.globals) result.set(v.name, v);
    for (const v of analysis.variables) {
        if (v.scope === 'global' || v.scope === 'scene') result.set(v.name, v);
    }
    for (const v of analysis.variables) {
        if ((v.scope === 'local' || v.scope === 'param') && v.fn === fn && v.fn) result.set(v.name, v);
    }
    return result;
}

function describeVariable(v) {
    const scope = v.scope === 'param' ? 'parameter' : v.scope;
    return `(${scope}) ${v.name}: ${typeToString(v.type)}`;
}

// ---------------------------------------------------------------------------
// Cursor context
// ---------------------------------------------------------------------------

/**
 * Works out what kind of thing is being typed at `offset`.
 *  - 'none'        inside a comment, plain string text, or a declaration name
 *  - 'type'        after `name:` in a declaration / parameter list / return type list
 *  - 'statement'   at the start of a statement
 *  - 'expression'  anywhere else
 */
function cursorContext(text, offset) {
    const prefix = text.slice(0, offset);
    const tokens = tokenize(prefix);
    let last = tokens[tokens.length - 1];

    // the word currently being typed is not part of the context
    if (last && last.end === offset && (last.type === 'identifier' || last.type === 'int' || last.type === 'float')) {
        tokens.pop();
        last = tokens[tokens.length - 1];
    }

    if (last && last.type === 'comment' && !prefix.slice(last.end).includes('\n')) return { kind: 'none' };

    if (last && last.type === 'string' && !last.terminated) {
        if (last.braceDepth > 0) {
            const inner = last.value.slice(last.value.lastIndexOf('{') + 1);
            if (/:\s*[A-Za-z_0-9]*$/.test(inner)) return { kind: 'none' };
            return { kind: 'expression' };
        }
        return { kind: 'none' };
    }

    const code = tokens.filter((t) => t.type !== 'comment');
    const prev = code[code.length - 1];
    const prev2 = code[code.length - 2];

    if (!prev) return { kind: 'statement', topLevel: true };

    // naming something new
    if (prev.type === 'identifier' && ['func', 'global', 'scene', 'local'].includes(prev.value)) return { kind: 'none' };

    // types: `global x: |`, `(a: |`, `func f(): |`, `func f(): i32, |`
    if (prev.type === ':' || (prev.type === '[' && prev2 && prev2.type === 'identifier')) {
        return { kind: 'type' };
    }
    if (prev.type === ',' && isInReturnTypeList(code)) return { kind: 'type' };

    // statement start
    const fnLine = isAfterFunctionHeader(code, prefix);
    if (
        prev.type === ';' ||
        (prev.type === 'identifier' && ['then', 'else', 'end'].includes(prev.value)) ||
        fnLine
    ) {
        const topLevel = blockDepth(code) === 0;
        return { kind: 'statement', topLevel };
    }

    return { kind: 'expression' };
}

function blockDepth(code) {
    let depth = 0;
    for (const t of code) {
        if (t.type !== 'identifier') continue;
        if (BLOCK_START.has(t.value)) depth++;
        else if (t.value === 'end') depth = Math.max(0, depth - 1);
    }
    return depth;
}

/** True when the cursor is on a line after a `func name(...)[: types]` header with nothing typed since */
function isAfterFunctionHeader(code, prefix) {
    let i = code.length - 1;
    // walk back over a return type list
    while (i >= 0 && code[i].type !== ')') {
        const t = code[i];
        if (!(t.type === 'identifier' || t.type === ',' || t.type === ':' || t.type === '[' || t.type === ']' || t.type === 'int')) return false;
        i--;
    }
    if (i < 0) return false;
    // find matching `(` and check for `func name` before it
    let depth = 0;
    for (; i >= 0; i--) {
        if (code[i].type === ')') depth++;
        else if (code[i].type === '(') {
            depth--;
            if (depth === 0) break;
        }
    }
    if (i < 2 || code[i - 2].type !== 'identifier' || code[i - 2].value !== 'func') return false;
    // must be on a new line after the header
    return prefix.slice(code[code.length - 1].end).includes('\n');
}

function isInReturnTypeList(code) {
    let i = code.length - 1;
    while (i >= 0 && (code[i].type === 'identifier' || code[i].type === ',' || code[i].type === '[' || code[i].type === ']' || code[i].type === 'int')) i--;
    return i >= 1 && code[i].type === ':' && code[i - 1].type === ')';
}

// ---------------------------------------------------------------------------
// Completion
// ---------------------------------------------------------------------------

function functionDoc(fn, kind) {
    const md = new vscode.MarkdownString();
    md.appendCodeblock(kind === 'step' ? stepSignatureToString(fn) : signatureToString(fn), LANGUAGE_ID);
    if (kind === 'step') md.appendMarkdown('Built-in cutscene step');
    else if (kind === 'expr') md.appendMarkdown('Built-in expression function');
    else md.appendMarkdown('Script function');
    return md;
}

function stepSnippet(fn, addSemicolon) {
    const snippet = new vscode.SnippetString(fn.name);
    fn.args.forEach((arg, index) => {
        snippet.appendText(index === 0 ? ' ' : ', ');
        if (arg.type && arg.type.name === 'str') {
            snippet.appendText('"');
            snippet.appendPlaceholder(arg.name);
            snippet.appendText('"');
        } else {
            snippet.appendPlaceholder(arg.name);
        }
    });
    if (addSemicolon) snippet.appendText(';');
    return snippet;
}

function callSnippet(fn) {
    const snippet = new vscode.SnippetString(fn.name + '(');
    fn.args.forEach((arg, index) => {
        if (index > 0) snippet.appendText(', ');
        snippet.appendPlaceholder(arg.name);
    });
    snippet.appendText(')');
    return snippet;
}

const retriggerSignature = { command: 'editor.action.triggerParameterHints', title: 'Signature help' };

function provideCompletionItems(document, position) {
    const offset = document.offsetAt(position);
    const text = document.getText();
    const context = cursorContext(text, offset);
    if (context.kind === 'none') return [];

    const items = [];
    const add = (label, kind, detail, documentation, insertText, sortPrefix) => {
        const item = new vscode.CompletionItem(label, kind);
        if (detail) item.detail = detail;
        if (documentation) item.documentation = documentation;
        if (insertText) item.insertText = insertText;
        if (sortPrefix) item.sortText = sortPrefix + label;
        items.push(item);
        return item;
    };

    if (context.kind === 'type') {
        for (const [name, description] of Object.entries(TYPES)) {
            add(name, vscode.CompletionItemKind.TypeParameter, description);
        }
        return items;
    }

    const analysis = analyzeDocument(document);
    const project = getProjectData(document);

    if (context.kind === 'statement') {
        if (context.topLevel) {
            for (const kw of ['func', 'global', 'scene']) add(kw, vscode.CompletionItemKind.Keyword, null, null, null, '0');
            return items;
        }

        const lineRest = document.lineAt(position.line).text.slice(position.character);
        const addSemicolon = !/^\s*\S*\s*;/.test(lineRest) && /^\s*$/.test(lineRest.replace(/^[A-Za-z0-9_]*/, ''));

        for (const kw of STATEMENT_KEYWORDS) {
            if (kw === 'global' || kw === 'scene' || kw === 'func') continue;
            add(kw, vscode.CompletionItemKind.Keyword, null, null, null, '2');
        }

        for (const fn of project.steps.values()) {
            const item = add(fn.name, vscode.CompletionItemKind.Function, stepSignatureToString(fn), functionDoc(fn, 'step'), stepSnippet(fn, addSemicolon), '0');
            if (fn.args.length) item.command = retriggerSignature;
        }

        for (const fn of analysis.functions.values()) {
            const item = add(fn.name, vscode.CompletionItemKind.Method, signatureToString(fn), functionDoc(fn, 'user'), stepSnippet(fn, addSemicolon), '0');
            if (fn.args.length) item.command = retriggerSignature;
        }

        for (const v of visibleVariables(analysis, project, offset).values()) {
            add(v.name, vscode.CompletionItemKind.Variable, describeVariable(v), null, null, '1');
        }

        return items;
    }

    // expression
    for (const v of visibleVariables(analysis, project, offset).values()) {
        add(v.name, vscode.CompletionItemKind.Variable, describeVariable(v), null, null, '0');
    }

    for (const fn of project.exprs.values()) {
        const item = add(fn.name, vscode.CompletionItemKind.Function, signatureToString(fn), functionDoc(fn, 'expr'), callSnippet(fn), '1');
        if (fn.args.length) item.command = retriggerSignature;
    }

    for (const fn of analysis.functions.values()) {
        if (!fn.returns.length) continue;
        const item = add(fn.name, vscode.CompletionItemKind.Method, signatureToString(fn), functionDoc(fn, 'user'), callSnippet(fn), '1');
        if (fn.args.length) item.command = retriggerSignature;
    }

    for (const kw of EXPRESSION_KEYWORDS) add(kw, vscode.CompletionItemKind.Keyword, null, null, null, '2');
    for (const kw of ['then']) add(kw, vscode.CompletionItemKind.Keyword, null, null, null, '2');

    for (const c of project.constants.values()) {
        if (c.name === 'true' || c.name === 'false') continue;
        const detail = c.enumName ? `enum ${c.enumName}${c.value !== null ? ` = ${c.value}` : ''}` : null;
        add(c.name, vscode.CompletionItemKind.EnumMember, detail, null, null, '3');
    }

    return items;
}

// ---------------------------------------------------------------------------
// Signature help
// ---------------------------------------------------------------------------

function provideSignatureHelp(document, position) {
    const offset = document.offsetAt(position);
    const prefix = document.getText().slice(0, offset);
    const tokens = tokenize(prefix).filter((t) => t.type !== 'comment');
    const last = tokens[tokens.length - 1];

    const analysis = analyzeDocument(document);
    const project = getProjectData(document);

    // innermost unclosed `(` preceded by an identifier => function call
    let depth = 0;
    let commas = 0;
    let i = tokens.length - 1;
    for (; i >= 0; i--) {
        const t = tokens[i];
        if (t.type === ')') depth++;
        else if (t.type === '(') {
            if (depth === 0) break;
            depth--;
        } else if (t.type === ',' && depth === 0) commas++;
        else if (t.type === ';' || (t.type === 'identifier' && ['then', 'else', 'end', 'func'].includes(t.value))) {
            i = -1;
            break;
        }
    }

    let fn = null;
    let kind = null;
    let activeParameter = 0;

    if (i > 0 && tokens[i - 1].type === 'identifier') {
        const name = tokens[i - 1].value;
        fn = project.exprs.get(name) || analysis.functions.get(name);
        kind = project.exprs.has(name) ? 'expr' : 'user';
        activeParameter = commas;
    } else {
        // cutscene step: first token of the current statement
        let start = tokens.length;
        depth = 0;
        commas = 0;
        for (let j = tokens.length - 1; j >= 0; j--) {
            const t = tokens[j];
            if (t.type === ';' || (t.type === 'identifier' && ['then', 'else', 'end'].includes(t.value))) break;
            if (t.type === ')') depth++;
            else if (t.type === '(') depth--;
            else if (t.type === ',' && depth === 0) commas++;
            start = j;
        }
        let first = tokens[start];
        // skip a function header
        if (first && first.type === 'identifier' && first.value === 'func') {
            const headerLine = document.positionAt(first.start).line;
            commas = 0;
            first = null;
            depth = 0;
            for (let j = start; j < tokens.length; j++) {
                const t = tokens[j];
                if (document.positionAt(t.start).line <= headerLine) continue;
                if (!first) first = t;
                else if (t.type === '(') depth++;
                else if (t.type === ')') depth--;
                else if (t.type === ',' && depth === 0) commas++;
            }
        }
        if (!first || first.type !== 'identifier' || (first === last && last.end === offset)) return null;
        fn = project.steps.get(first.value) || analysis.functions.get(first.value);
        kind = project.steps.has(first.value) ? 'step' : 'user';
        activeParameter = commas;
    }

    if (!fn || !fn.args.length) return null;

    const label = kind === 'step' ? stepSignatureToString(fn) : signatureToString(fn);
    const signature = new vscode.SignatureInformation(label, functionDoc(fn, kind));
    let searchFrom = label.indexOf(kind === 'step' ? ' ' : '(');
    signature.parameters = fn.args.map((arg) => {
        const paramLabel = `${arg.name}: ${typeToString(arg.type)}`;
        const startIdx = label.indexOf(paramLabel, searchFrom);
        searchFrom = startIdx + paramLabel.length;
        return new vscode.ParameterInformation([startIdx, startIdx + paramLabel.length]);
    });

    const help = new vscode.SignatureHelp();
    help.signatures = [signature];
    help.activeSignature = 0;
    help.activeParameter = Math.min(activeParameter, fn.args.length - 1);
    return help;
}

// ---------------------------------------------------------------------------
// Hover / definition / symbols
// ---------------------------------------------------------------------------

function lookupSymbol(document, position) {
    const range = document.getWordRangeAtPosition(position, IDENTIFIER);
    if (!range) return null;
    const offset = document.offsetAt(position);
    const context = cursorContext(document.getText(), document.offsetAt(range.start));
    if (context.kind === 'none') {
        // allow declarations names (func foo / global foo) but not plain string text / comments
        const text = document.getText();
        const tokens = tokenize(text.slice(0, document.offsetAt(range.end)));
        const last = tokens[tokens.length - 1];
        if (!last || last.type !== 'identifier') return null;
    }

    const name = document.getText(range);
    const analysis = analyzeDocument(document);
    const project = getProjectData(document);

    const variables = visibleVariables(analysis, project, offset);
    if (variables.has(name)) return { range, kind: 'variable', value: variables.get(name) };
    if (analysis.functions.has(name)) return { range, kind: 'user', value: analysis.functions.get(name) };
    if (project.steps.has(name)) return { range, kind: 'step', value: project.steps.get(name) };
    if (project.exprs.has(name)) return { range, kind: 'expr', value: project.exprs.get(name) };
    if (project.constants.has(name)) return { range, kind: 'constant', value: project.constants.get(name) };
    if (TYPES[name]) return { range, kind: 'type', value: { name, description: TYPES[name] } };
    return null;
}

function provideHover(document, position) {
    const symbol = lookupSymbol(document, position);
    if (!symbol) return null;
    const md = new vscode.MarkdownString();
    switch (symbol.kind) {
        case 'variable':
            md.appendCodeblock(describeVariable(symbol.value), LANGUAGE_ID);
            if (symbol.value.file) md.appendMarkdown(`Declared in \`${path.basename(symbol.value.file)}\``);
            break;
        case 'user':
        case 'step':
        case 'expr':
            return new vscode.Hover(functionDoc(symbol.value, symbol.kind), symbol.range);
        case 'constant': {
            const c = symbol.value;
            md.appendCodeblock(`${c.name}${c.value !== null ? ` = ${c.value}` : ''}`, LANGUAGE_ID);
            if (c.enumName) md.appendMarkdown(`\`enum ${c.enumName}\``);
            break;
        }
        case 'type':
            md.appendCodeblock(symbol.value.name, LANGUAGE_ID);
            md.appendMarkdown(symbol.value.description);
            break;
    }
    return new vscode.Hover(md, symbol.range);
}

function provideDefinition(document, position) {
    const symbol = lookupSymbol(document, position);
    if (!symbol) return null;
    const v = symbol.value;

    if (symbol.kind === 'user' || (symbol.kind === 'variable' && !v.file)) {
        return new vscode.Location(document.uri, new vscode.Range(document.positionAt(v.start), document.positionAt(v.end)));
    }
    if (v.file && v.line !== null && v.line !== undefined) {
        return new vscode.Location(vscode.Uri.file(v.file), new vscode.Position(v.line, 0));
    }
    return null;
}

function provideDocumentSymbols(document) {
    const analysis = analyzeDocument(document);
    const symbols = [];
    const rangeOf = (start, end) => new vscode.Range(document.positionAt(start), document.positionAt(end));

    for (const v of analysis.variables) {
        if (v.scope !== 'global' && v.scope !== 'scene') continue;
        symbols.push(
            new vscode.DocumentSymbol(
                v.name,
                `${v.scope} ${typeToString(v.type)}`,
                v.scope === 'global' ? vscode.SymbolKind.Variable : vscode.SymbolKind.Field,
                rangeOf(v.declStart, v.end),
                rangeOf(v.start, v.end)
            )
        );
    }

    for (const fn of analysis.functions.values()) {
        const symbol = new vscode.DocumentSymbol(
            fn.name,
            signatureToString(fn).slice(5 + fn.name.length),
            vscode.SymbolKind.Function,
            rangeOf(fn.declStart, Math.max(fn.bodyEnd, fn.end)),
            rangeOf(fn.start, fn.end)
        );
        for (const v of analysis.variables) {
            if (v.fn !== fn || v.scope !== 'local') continue;
            symbol.children.push(new vscode.DocumentSymbol(v.name, typeToString(v.type), vscode.SymbolKind.Variable, rangeOf(v.declStart, v.end), rangeOf(v.start, v.end)));
        }
        symbols.push(symbol);
    }

    symbols.sort((a, b) => a.range.start.compareTo(b.range.start));
    return symbols;
}

// ---------------------------------------------------------------------------

function activate(context) {
    const selector = { language: LANGUAGE_ID };

    context.subscriptions.push(
        vscode.languages.registerCompletionItemProvider(selector, { provideCompletionItems }, ':', '{', '('),
        vscode.languages.registerSignatureHelpProvider(selector, { provideSignatureHelp }, { triggerCharacters: [' ', '(', ','], retriggerCharacters: [','] }),
        vscode.languages.registerHoverProvider(selector, { provideHover }),
        vscode.languages.registerDefinitionProvider(selector, { provideDefinition }),
        vscode.languages.registerDocumentSymbolProvider(selector, { provideDocumentSymbols }),
        vscode.workspace.onDidCloseTextDocument((doc) => analysisCache.delete(doc.uri.toString()))
    );
}

function deactivate() {}

module.exports = {
    activate,
    deactivate,
    // exported for tests
    _internal: { tokenize, analyze, cursorContext, parseFuncComments, parseEnums },
};
