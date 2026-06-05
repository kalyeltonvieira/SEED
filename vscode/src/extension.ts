import * as vscode from "vscode";
import * as fs from "fs";
import * as path from "path";
import { spawn } from "child_process";

const DEFAULT_COMPILER = "D:\\SEED\\seed.exe";
const DEFAULT_TESTS = "D:\\SEED\\tests\\seed_selftest.seed";
const DREAM_LOG = "D:\\SEED\\dreams\\dream.log";

type SeverityCounts = {
    errors: number;
    warnings: number;
};

let diagnostics: vscode.DiagnosticCollection;
let output: vscode.OutputChannel;
let status: vscode.StatusBarItem;
let compilerDiagnostics = new Map<string, vscode.Diagnostic[]>();
let compilerOnline = false;
let compilerVersion = "SEED offline";

// --- Module Indexing System ---

interface SymbolInfo {
    name: string;
    kind: vscode.SymbolKind;
    signature: string;
    description: string;
    line: number;
    character: number;
    filePath: string;
}

interface ModuleInfo {
    name: string; // e.g. "stdlib.io" or "examples.hello"
    filePath: string;
    symbols: Map<string, SymbolInfo>;
}

// Global indexes
let modulesIndex = new Map<string, ModuleInfo>();
let fileToModuleMap = new Map<string, string>();

function getStdlibPath(): string {
    const compilerPath = vscode.workspace.getConfiguration("seed").get<string>("compilerPath") || DEFAULT_COMPILER;
    const dir = path.dirname(compilerPath);
    let stdlib = path.join(dir, "stdlib");
    if (fs.existsSync(stdlib)) {
        return stdlib;
    }
    if (process.env.SEED_HOME) {
        stdlib = path.join(process.env.SEED_HOME, "stdlib");
        if (fs.existsSync(stdlib)) {
            return stdlib;
        }
    }
    return "D:\\SEED\\stdlib";
}

function indexFile(filePath: string) {
    try {
        if (!fs.existsSync(filePath)) return;
        const content = fs.readFileSync(filePath, "utf8");
        const lines = content.split(/\r?\n/);
        let docCommentBuffer: string[] = [];
        let moduleName = "";

        // Find module declaration
        for (const line of lines) {
            const modMatch = line.match(/^\s*module\s+([A-Za-z0-9_\.\:]+)/);
            if (modMatch) {
                moduleName = modMatch[1].replace(/::/g, ".");
                break;
            }
        }

        if (!moduleName) {
            // Fallback: guess from filename
            const base = path.basename(filePath, ".seed");
            moduleName = base;
        }

        const symbols = new Map<string, SymbolInfo>();

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i].trim();
            if (line.startsWith("///")) {
                docCommentBuffer.push(line.substring(3).trim());
                continue;
            }
            if (line.startsWith("//") || line.length === 0) {
                continue;
            }

            // Check functions: pub fn or fn
            const fnMatch = line.match(/^(?:pub\s+)?fn\s+([A-Za-z0-9_]+)\s*\((.*?)\)(?:\s*->\s*([A-Za-z0-9_\[\],\s\:]+))?/);
            if (fnMatch) {
                const name = fnMatch[1];
                const params = fnMatch[2];
                const ret = fnMatch[3] ? fnMatch[3].trim() : "Unit";
                const signature = `fn ${name}(${params}) -> ${ret}`;
                const description = docCommentBuffer.join("\n");
                symbols.set(name, {
                    name,
                    kind: vscode.SymbolKind.Function,
                    signature,
                    description,
                    line: i,
                    character: lines[i].indexOf(name),
                    filePath
                });
                docCommentBuffer = [];
                continue;
            }

            // Check struct/enum/trait/type
            const typeMatch = line.match(/^(?:pub\s+)?(struct|enum|trait|type)\s+([A-Za-z0-9_]+)/);
            if (typeMatch) {
                const kindStr = typeMatch[1];
                const name = typeMatch[2];
                let kind = vscode.SymbolKind.Struct;
                if (kindStr === "enum") kind = vscode.SymbolKind.Enum;
                else if (kindStr === "trait") kind = vscode.SymbolKind.Interface;
                else if (kindStr === "type") kind = vscode.SymbolKind.Class;

                const signature = `${kindStr} ${name}`;
                const description = docCommentBuffer.join("\n");
                symbols.set(name, {
                    name,
                    kind,
                    signature,
                    description,
                    line: i,
                    character: lines[i].indexOf(name),
                    filePath
                });
                docCommentBuffer = [];
                continue;
            }

            // Check constants
            const constMatch = line.match(/^(?:pub\s+)?const\s+([A-Za-z0-9_]+)/);
            if (constMatch) {
                const name = constMatch[1];
                const signature = `const ${name}`;
                const description = docCommentBuffer.join("\n");
                symbols.set(name, {
                    name,
                    kind: vscode.SymbolKind.Constant,
                    signature,
                    description,
                    line: i,
                    character: lines[i].indexOf(name),
                    filePath
                });
                docCommentBuffer = [];
                continue;
            }

            docCommentBuffer = [];
        }

        modulesIndex.set(moduleName, {
            name: moduleName,
            filePath,
            symbols
        });
        fileToModuleMap.set(filePath, moduleName);
    } catch (err) {
        console.error(`Error indexing file ${filePath}:`, err);
    }
}

async function indexAllFiles() {
    modulesIndex.clear();
    fileToModuleMap.clear();

    // 1. Index stdlib
    const stdlibPath = getStdlibPath();
    if (fs.existsSync(stdlibPath)) {
        const files = fs.readdirSync(stdlibPath).filter(f => f.endsWith(".seed"));
        for (const file of files) {
            indexFile(path.join(stdlibPath, file));
        }
    }

    // 2. Index workspace
    const workspaceFiles = await vscode.workspace.findFiles("**/*.seed", "**/node_modules/**");
    for (const uri of workspaceFiles) {
        indexFile(uri.fsPath);
    }
}

// --- Import Parsing Logic ---

interface FileImports {
    importedModules: string[];
    moduleAliases: Map<string, string>; // alias -> moduleName
    importedSymbols: Map<string, string>; // symbol -> moduleName
}

function parseFileImports(text: string): FileImports {
    const importedModules: string[] = [];
    const moduleAliases = new Map<string, string>();
    const importedSymbols = new Map<string, string>();
    const lines = text.split(/\r?\n/);

    for (const line of lines) {
        const trimmed = line.trim();
        // Match braced imports: import std::math::{sqrt, PI} or import std::math{sqrt, PI}
        const bracedMatch = trimmed.match(/^\s*(?:import|use)\s+([A-Za-z0-9_:\.]+?)(?=\s*(?:::)?\s*\{|\s*\{)\s*(?:::)?\s*\{\s*([A-Za-z0-9_,\s]+)\s*\}/);
        if (bracedMatch) {
            let modPath = bracedMatch[1].replace(/::/g, ".");
            if (modPath.startsWith("std.")) {
                modPath = "stdlib." + modPath.substring(4);
            }
            const symbols = bracedMatch[2].split(",").map(s => s.trim()).filter(s => s.length > 0);
            // For braced imports, the entire path is the module
            importedModules.push(modPath);
            const alias = modPath.split(".").pop() || "";
            moduleAliases.set(alias, modPath);
            for (const sym of symbols) {
                importedSymbols.set(sym, modPath);
            }
            continue;
        }

        // Match normal imports: import std::io or import app::auth::login as auth
        const normalMatch = trimmed.match(/^\s*(?:import|use)\s+([A-Za-z0-9_:\.]+)(?:\s+as\s+([A-Za-z0-9_]+))?/);
        if (normalMatch) {
            let modPath = normalMatch[1].replace(/::/g, ".");
            if (modPath.startsWith("std.")) {
                modPath = "stdlib." + modPath.substring(4);
            }
            const alias = normalMatch[2] ? normalMatch[2].trim() : (modPath.split(".").pop() || "");
            
            // Check if this is a qualified import (e.g., std::collections::List)
            // If it has multiple segments, the last segment is the symbol, the rest is the module
            const segments = modPath.split(".");
            if (segments.length > 1 && !modulesIndex.has(modPath)) {
                // The module is all segments except the last, the last segment is the symbol
                const moduleName = segments.slice(0, -1).join(".");
                const symbolName = segments[segments.length - 1];
                importedModules.push(moduleName);
                moduleAliases.set(alias, moduleName);
                importedSymbols.set(symbolName, moduleName);
            } else {
                // Simple module import
                importedModules.push(modPath);
                moduleAliases.set(alias, modPath);
            }
            continue;
        }
    }

    return { importedModules, moduleAliases, importedSymbols };
}

function getWordAndQualifier(document: vscode.TextDocument, position: vscode.Position): { word: string, qualifier?: string } {
    const line = document.lineAt(position.line).text;
    const wordRange = document.getWordRangeAtPosition(position);
    if (!wordRange) return { word: "" };
    const word = document.getText(wordRange);

    const startChar = wordRange.start.character;
    if (startChar >= 2 && line.substring(startChar - 2, startChar) === "::") {
        const beforeQual = line.substring(0, startChar - 2);
        const qualMatch = beforeQual.match(/([A-Za-z0-9_]+)$/);
        if (qualMatch) {
            return { word, qualifier: qualMatch[1] };
        }
    } else if (startChar >= 1 && line.substring(startChar - 1, startChar) === ".") {
        const beforeQual = line.substring(0, startChar - 1);
        const qualMatch = beforeQual.match(/([A-Za-z0-9_]+)$/);
        if (qualMatch) {
            return { word, qualifier: qualMatch[1] };
        }
    }
    return { word };
}

// --- Extension Activation ---

export function activate(context: vscode.ExtensionContext) {
    output = vscode.window.createOutputChannel("SEED");
    diagnostics = vscode.languages.createDiagnosticCollection("seed");
    status = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 90);
    status.command = "seed.showDreams";
    status.tooltip = "SEED compiler status";
    status.show();

    context.subscriptions.push(output, diagnostics, status);
    context.subscriptions.push(vscode.commands.registerCommand("seed.compileFile", () => compileActiveFile()));
    context.subscriptions.push(vscode.commands.registerCommand("seed.runProgram", () => runProgram()));
    context.subscriptions.push(vscode.commands.registerCommand("seed.runTests", () => runTests()));
    context.subscriptions.push(vscode.commands.registerCommand("seed.formatDocument", () => vscode.commands.executeCommand("editor.action.formatDocument")));
    context.subscriptions.push(vscode.commands.registerCommand("seed.showDreams", () => showDreamsPanel(context)));
    context.subscriptions.push(vscode.commands.registerCommand("seed.showWelcome", () => showWelcome(context, true)));

    context.subscriptions.push(vscode.languages.registerCompletionItemProvider("seed", new SeedCompletionProvider(), ".", ":", "_"));
    context.subscriptions.push(vscode.languages.registerDocumentFormattingEditProvider("seed", new SeedFormatter()));
    context.subscriptions.push(vscode.languages.registerHoverProvider("seed", new SeedHoverProvider()));
    context.subscriptions.push(vscode.languages.registerDefinitionProvider("seed", new SeedDefinitionProvider()));

    context.subscriptions.push(vscode.workspace.onDidOpenTextDocument(doc => {
        if (doc.languageId === "seed") {
            indexFile(doc.uri.fsPath);
            refreshDiagnostics(doc);
            maybeShowWelcome(context);
        }
    }));
    context.subscriptions.push(vscode.workspace.onDidChangeTextDocument(event => {
        if (event.document.languageId === "seed") {
            refreshDiagnostics(event.document);
        }
    }));
    context.subscriptions.push(vscode.workspace.onDidSaveTextDocument(doc => {
        if (doc.languageId === "seed") {
            indexFile(doc.uri.fsPath);
            refreshDiagnostics(doc);
        }
    }));
    context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(editor => {
        if (editor?.document.languageId === "seed") {
            refreshDiagnostics(editor.document);
            maybeShowWelcome(context);
        }
        updateStatusForActiveEditor();
    }));

    // Perform initial indexing
    indexAllFiles().then(() => {
        checkCompiler();
        if (vscode.window.activeTextEditor?.document.languageId === "seed") {
            refreshDiagnostics(vscode.window.activeTextEditor.document);
            maybeShowWelcome(context);
        }
        updateStatusForActiveEditor();
    });
}

export function deactivate() {
    diagnostics?.dispose();
    output?.dispose();
    status?.dispose();
}

// --- Completion Provider ---

class SeedCompletionProvider implements vscode.CompletionItemProvider {
    provideCompletionItems(document: vscode.TextDocument, position: vscode.Position): vscode.CompletionItem[] {
        const text = document.getText();
        const line = document.lineAt(position.line).text;
        const beforeCursor = line.substring(0, position.character);
        const imports = parseFileImports(text);

        // 1. Suggest after qualifier (e.g. io:: or io.)
        const qualMatch = beforeCursor.match(/([A-Za-z0-9_]+)(?::{2}|\.)$/);
        if (qualMatch) {
            const qualifier = qualMatch[1];
            const modName = imports.moduleAliases.get(qualifier);
            if (modName) {
                const mod = modulesIndex.get(modName);
                if (mod) {
                    const items: vscode.CompletionItem[] = [];
                    mod.symbols.forEach(sym => {
                        let kind = vscode.CompletionItemKind.Function;
                        if (sym.kind === vscode.SymbolKind.Struct) kind = vscode.CompletionItemKind.Struct;
                        else if (sym.kind === vscode.SymbolKind.Enum) kind = vscode.CompletionItemKind.Enum;
                        else if (sym.kind === vscode.SymbolKind.Constant) kind = vscode.CompletionItemKind.Constant;

                        const item = new vscode.CompletionItem(sym.name, kind);
                        item.detail = sym.signature;
                        item.documentation = new vscode.MarkdownString(sym.description);
                        items.push(item);
                    });
                    return items;
                }
            }
        }

        // 2. Suggest for import statements
        const importMatch = beforeCursor.match(/^\s*(?:import|use)\s+([A-Za-z0-9_:\.]*)$/);
        if (importMatch) {
            const items: vscode.CompletionItem[] = [];
            modulesIndex.forEach((mod, name) => {
                let displayName = name;
                if (name.startsWith("stdlib.")) {
                    displayName = "std::" + name.substring(7);
                }
                const item = new vscode.CompletionItem(displayName, vscode.CompletionItemKind.Module);
                item.detail = `Módulo ${name}`;
                item.insertText = displayName;
                items.push(item);
            });
            return items;
        }

        // 3. Suggest symbols inside braced imports (e.g., use std::io::{print, <cursor>})
        const bracedImportMatch = beforeCursor.match(/^\s*(?:import|use)\s+([A-Za-z0-9_:\.]+)\s*(?:::)?\s*\{\s*([A-Za-z0-9_,\s]*)$/);
        if (bracedImportMatch) {
            const modPath = bracedImportMatch[1].replace(/::/g, ".");
            let normalizedModPath = modPath;
            if (normalizedModPath.startsWith("std.")) {
                normalizedModPath = "stdlib." + normalizedModPath.substring(4);
            }
            const mod = modulesIndex.get(normalizedModPath);
            if (mod) {
                const items: vscode.CompletionItem[] = [];
                mod.symbols.forEach(sym => {
                    let kind = vscode.CompletionItemKind.Function;
                    if (sym.kind === vscode.SymbolKind.Struct) kind = vscode.CompletionItemKind.Struct;
                    else if (sym.kind === vscode.SymbolKind.Enum) kind = vscode.CompletionItemKind.Enum;
                    else if (sym.kind === vscode.SymbolKind.Constant) kind = vscode.CompletionItemKind.Constant;

                    const item = new vscode.CompletionItem(sym.name, kind);
                    item.detail = sym.signature;
                    item.documentation = new vscode.MarkdownString(sym.description);
                    items.push(item);
                });
                return items;
            }
        }

        // 4. Global completions
        const items: vscode.CompletionItem[] = [
            ...keywordCompletions(),
            ...typeCompletions(),
            ...snippetCompletions()
        ];

        // Local functions/variables
        items.push(...functionCompletions(text));
        items.push(...variableCompletions(document.getText(new vscode.Range(new vscode.Position(0, 0), position))));

        // Imported module aliases (e.g. io, math)
        imports.moduleAliases.forEach((modName, alias) => {
            const item = new vscode.CompletionItem(alias, vscode.CompletionItemKind.Module);
            item.detail = `Módulo importado: ${modName}`;
            item.insertText = alias + "::";
            items.push(item);
        });

        // Directly imported symbols (e.g. sqrt)
        imports.importedSymbols.forEach((modName, symName) => {
            const mod = modulesIndex.get(modName);
            const sym = mod?.symbols.get(symName);
            if (sym) {
                const item = new vscode.CompletionItem(symName, vscode.CompletionItemKind.Function);
                item.detail = `Importado de ${modName}: ${sym.signature}`;
                item.documentation = new vscode.MarkdownString(sym.description);
                items.push(item);
            }
        });

        return items;
    }
}

// --- Hover Provider ---

class SeedHoverProvider implements vscode.HoverProvider {
    provideHover(document: vscode.TextDocument, position: vscode.Position): vscode.Hover | undefined {
        const imports = parseFileImports(document.getText());
        const { word, qualifier } = getWordAndQualifier(document, position);
        if (!word) return undefined;

        let sym: SymbolInfo | undefined;

        if (qualifier) {
            const modName = imports.moduleAliases.get(qualifier);
            if (modName) {
                sym = modulesIndex.get(modName)?.symbols.get(word);
            }
        } else {
            if (imports.importedSymbols.has(word)) {
                const modName = imports.importedSymbols.get(word)!;
                sym = modulesIndex.get(modName)?.symbols.get(word);
            } else {
                for (const modName of imports.importedModules) {
                    const found = modulesIndex.get(modName)?.symbols.get(word);
                    if (found) {
                        sym = found;
                        break;
                    }
                }
            }
        }

        if (sym) {
            const md = new vscode.MarkdownString();
            md.appendCodeblock(sym.signature, "seed");
            if (sym.description) {
                md.appendMarkdown("\n---\n" + sym.description);
            }
            return new vscode.Hover(md);
        }

        const keywordDocs = getKeywordDocumentation(word);
        if (keywordDocs) {
            return new vscode.Hover(new vscode.MarkdownString(keywordDocs));
        }

        return undefined;
    }
}

// --- Definition Provider ---

class SeedDefinitionProvider implements vscode.DefinitionProvider {
    provideDefinition(document: vscode.TextDocument, position: vscode.Position): vscode.Location | undefined {
        const imports = parseFileImports(document.getText());
        const { word, qualifier } = getWordAndQualifier(document, position);
        if (!word) return undefined;

        // 1. Is it a module alias clicked directly? (e.g. 'io' in 'io::print')
        if (!qualifier && imports.moduleAliases.has(word)) {
            const modName = imports.moduleAliases.get(word)!;
            const mod = modulesIndex.get(modName);
            if (mod && fs.existsSync(mod.filePath)) {
                return new vscode.Location(vscode.Uri.file(mod.filePath), new vscode.Position(0, 0));
            }
        }

        // 2. Has a qualifier (e.g. io::print)
        if (qualifier) {
            const modName = imports.moduleAliases.get(qualifier);
            if (modName) {
                const mod = modulesIndex.get(modName);
                const sym = mod?.symbols.get(word);
                if (sym && fs.existsSync(sym.filePath)) {
                    return new vscode.Location(vscode.Uri.file(sym.filePath), new vscode.Position(sym.line, sym.character));
                }
            }
        }

        // 3. No qualifier
        // Local symbols
        const localLoc = findLocalDefinition(document, document.getText(), word);
        if (localLoc) return localLoc;

        // Explicitly imported symbols (e.g. import std::math::{sqrt})
        if (imports.importedSymbols.has(word)) {
            const modName = imports.importedSymbols.get(word)!;
            const mod = modulesIndex.get(modName);
            const sym = mod?.symbols.get(word);
            if (sym && fs.existsSync(sym.filePath)) {
                return new vscode.Location(vscode.Uri.file(sym.filePath), new vscode.Position(sym.line, sym.character));
            }
        }

        // Search through all imported modules
        for (const modName of imports.importedModules) {
            const mod = modulesIndex.get(modName);
            const sym = mod?.symbols.get(word);
            if (sym && fs.existsSync(sym.filePath)) {
                return new vscode.Location(vscode.Uri.file(sym.filePath), new vscode.Position(sym.line, sym.character));
            }
        }

        // Check if word is a known module itself
        const mod = modulesIndex.get(word) || modulesIndex.get("stdlib." + word);
        if (mod && fs.existsSync(mod.filePath)) {
            return new vscode.Location(vscode.Uri.file(mod.filePath), new vscode.Position(0, 0));
        }

        return undefined;
    }
}

function findLocalDefinition(document: vscode.TextDocument, text: string, word: string): vscode.Location | undefined {
    const functionRegex = new RegExp(`\\bfn\\s+${word}\\s*\\(`, "g");
    let match = functionRegex.exec(text);
    if (match) {
        return new vscode.Location(document.uri, document.positionAt(match.index));
    }

    const typeRegex = new RegExp(`\\b(struct|enum|trait|type)\\s+${word}\\b`, "g");
    match = typeRegex.exec(text);
    if (match) {
        return new vscode.Location(document.uri, document.positionAt(match.index));
    }

    const varRegex = new RegExp(`\\b(let|const|mut)\\s+${word}\\b`, "g");
    match = varRegex.exec(text);
    if (match) {
        return new vscode.Location(document.uri, document.positionAt(match.index));
    }

    return undefined;
}

// --- Formatter Provider ---

class SeedFormatter implements vscode.DocumentFormattingEditProvider {
    provideDocumentFormattingEdits(document: vscode.TextDocument): vscode.TextEdit[] {
        const formatted = formatSeed(document.getText());
        const fullRange = new vscode.Range(
            document.positionAt(0),
            document.positionAt(document.getText().length)
        );
        return [vscode.TextEdit.replace(fullRange, formatted)];
    }
}

// --- Completions & Documentation Utilities ---

function keywordCompletions(): vscode.CompletionItem[] {
    const keywords: Array<[string, string, string]> = [
        ["fn", "Declara uma função.", "fn name(args) -> Unit { }"],
        ["let", "Declara uma variável local.", "let value = expression"],
        ["mut", "Declara uma variável mutável.", "let mut value = expression"],
        ["const", "Declara uma constante.", "const PI = 3.14"],
        ["pub", "Modificador de visibilidade pública.", "pub fn run() {}"],
        ["priv", "Modificador de visibilidade privada.", "priv fn run() {}"],
        ["loop", "Loop infinito.", "loop { }"],
        ["if", "Fluxo condicional.", "if condition { } else { }"],
        ["else", "Ramo alternativo de if.", "else { }"],
        ["while", "Loop enquanto a condição for verdadeira.", "while condition { }"],
        ["for", "Loop sobre coleções.", "for item in items { }"],
        ["match", "Pattern matching exaustivo.", "match value { Ok(v) => v Err(e) => e }"],
        ["type", "Declara tipo registro ou alias.", "type User = { name: String }"],
        ["struct", "Declara uma struct.", "struct Point { x: Int, y: Int }"],
        ["enum", "Declara um enum.", "enum State { Ready, Busy }"],
        ["trait", "Declara comportamento/interface.", "trait Display { fn show(self) -> String }"],
        ["impl", "Implementa métodos para um tipo.", "impl User { fn name(self) -> String { } }"],
        ["module", "Declara o módulo atual.", "module my_module"],
        ["import", "Importa um módulo.", "import std::io"],
        ["use", "Importa símbolos.", "use std::io"],
        ["spawn", "Cria uma tarefa concorrente.", "spawn task()"],
        ["async", "Declara execução assíncrona.", "async fn load() { }"],
        ["await", "Aguarda uma tarefa assíncrona.", "let value = await task"],
        ["effect", "Declara capacidades usadas.", "fn main() effect io -> Int { }"],
        ["contract", "Declara propriedades verificáveis.", "contract valid(x: Int) { assert(x >= 0) }"],
        ["genitor", "Declara família de programas gerados.", "genitor Sorter { entrada: Array[Int] }"],
        ["return", "Retorna da função atual.", "return value"]
    ];
    return keywords.map(([label, detail, example]) => {
        const item = new vscode.CompletionItem(label, vscode.CompletionItemKind.Keyword);
        item.detail = detail;
        item.documentation = new vscode.MarkdownString(`\`${example}\``);
        return item;
    });
}

function typeCompletions(): vscode.CompletionItem[] {
    const types = ["Int", "Float", "String", "Bool", "Array", "Dict", "Result", "Option", "Error", "Unit", "int", "float", "string", "bool", "array", "dict", "void", "char", "i32", "i64", "u32", "u64", "f32", "f64"];
    return types.map(typeName => {
        const item = new vscode.CompletionItem(typeName, vscode.CompletionItemKind.TypeParameter);
        item.detail = "Tipo SEED";
        item.documentation = new vscode.MarkdownString(`Tipo \`${typeName}\` da linguagem SEED.`);
        return item;
    });
}

function snippetCompletions(): vscode.CompletionItem[] {
    const snippets: Array<[string, string, string]> = [
        ["fn", "Função completa", "fn ${1:name}(${2:args}) -> ${3:Unit} {\n    ${0:// body}\n}"],
        ["main", "Função principal", "fn main() -> Unit {\n    ${0:// body}\n}"],
        ["ifelse", "If/else completo", "if ${1:condition} {\n    ${2:// then}\n} else {\n    ${0:// else}\n}"],
        ["match", "Match Result completo", "match ${1:value} {\n    Ok(${2:ok}) => {\n        ${3:// success}\n    }\n    Err(${4:err}) => {\n        ${0:// error}\n    }\n}"],
        ["while", "Loop while", "while ${1:condition} {\n    ${0:// body}\n}"],
        ["for", "Loop for", "for ${1:item} in ${2:items} {\n    ${0:// body}\n}"],
        ["struct", "Estrutura struct", "struct ${1:Name} {\n    ${2:field}: ${3:String},\n}"],
        ["type", "Tipo registro", "type ${1:Name} = {\n    ${2:field}: ${3:String}\n}"],
        ["enum", "Tipo enum", "enum ${1:Name} {\n    ${2:Variant},\n}"],
        ["trait", "Definição de trait", "trait ${1:Name} {\n    fn ${2:method}(self) -> ${3:Unit}\n}"],
        ["impl", "Implementação impl", "impl ${1:Type} {\n    fn ${2:method}(self) -> ${3:Unit} {\n        ${0:// body}\n    }\n}"]
    ];
    return snippets.map(([label, detail, body]) => {
        const item = new vscode.CompletionItem(label, vscode.CompletionItemKind.Snippet);
        item.detail = detail;
        item.insertText = new vscode.SnippetString(body);
        item.documentation = new vscode.MarkdownString("Snippet SEED.");
        return item;
    });
}

function functionCompletions(text: string): vscode.CompletionItem[] {
    const items: vscode.CompletionItem[] = [];
    const seen = new Set<string>();
    const regex = /\bfn\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\)/g;
    let match: RegExpExecArray | null;
    while ((match = regex.exec(text))) {
        const name = match[1];
        if (seen.has(name)) {
            continue;
        }
        seen.add(name);
        const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Function);
        item.detail = `Função local: ${name}(${match[2]})`;
        item.insertText = `${name}($0)`;
        item.documentation = new vscode.MarkdownString(`Chama a função local \`${name}\`.`);
        items.push(item);
    }
    return items;
}

function variableCompletions(text: string): vscode.CompletionItem[] {
    const items: vscode.CompletionItem[] = [];
    const seen = new Set<string>();
    const patterns = [
        /\blet\s+(?:mut\s+)?([A-Za-z_][A-Za-z0-9_]*)/g,
        /\bconst\s+([A-Za-z_][A-Za-z0-9_]*)/g,
        /\bfor\s+([A-Za-z_][A-Za-z0-9_]*)\s+in\b/g,
        /\bfn\s+[A-Za-z_][A-Za-z0-9_]*\s*\(([^)]*)\)/g
    ];
    for (const pattern of patterns) {
        let match: RegExpExecArray | null;
        while ((match = pattern.exec(text))) {
            if (pattern === patterns[3]) {
                for (const param of match[1].split(",")) {
                    const name = param.trim().match(/^([A-Za-z_][A-Za-z0-9_]*)/)?.[1];
                    if (name && name !== "self") {
                        pushVariable(items, seen, name, "Parâmetro da função atual.");
                    }
                }
            } else {
                pushVariable(items, seen, match[1], "Variável no escopo.");
            }
        }
    }
    return items;
}

function pushVariable(items: vscode.CompletionItem[], seen: Set<string>, name: string, detail: string) {
    if (seen.has(name)) {
        return;
    }
    seen.add(name);
    const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Variable);
    item.detail = detail;
    item.documentation = new vscode.MarkdownString(`Variável \`${name}\`.`);
    items.push(item);
}

function getKeywordDocumentation(word: string): string | undefined {
    const keywordDocs: Record<string, string> = {
        "fn": "Declara uma função.\n\n```seed\nfn name(params) -> ReturnType {\n    body\n}\n```",
        "let": "Declara uma variável local.\n\n```seed\nlet name = value\n```",
        "mut": "Modificador para tornar uma variável mutável.\n\n```seed\nlet mut name = value\n```",
        "const": "Declara uma constante.\n\n```seed\nconst name = value\n```",
        "if": "Fluxo condicional.\n\n```seed\nif condition {\n    // then\n} else {\n    // else\n}\n```",
        "else": "Ramo alternativo de if.",
        "loop": "Loop infinito.\n\n```seed\nloop {\n    // body\n}\n```",
        "while": "Loop enquanto condição for verdadeira.\n\n```seed\nwhile condition {\n    body\n}\n```",
        "for": "Loop sobre coleções.\n\n```seed\nfor item in items {\n    body\n}\n```",
        "match": "Pattern matching exaustivo.\n\n```seed\nmatch value {\n    Pattern1 => result1\n    Pattern2 => result2\n}\n```",
        "type": "Declara tipo registro ou alias.\n\n```seed\ntype Name = {\n    field: Type\n}\n```",
        "struct": "Declara uma struct.",
        "enum": "Declara um enum.\n\n```seed\nenum Name {\n    Variant1,\n    Variant2\n}\n```",
        "trait": "Declara comportamento.\n\n```seed\ntrait Name {\n    fn method(self) -> ReturnType\n}\n```",
        "impl": "Implementa métodos para um tipo.\n\n```seed\nimpl Type {\n    fn method(self) -> ReturnType {\n        body\n    }\n}\n```",
        "import": "Importa um módulo.\n\n```seed\nimport module\n```",
        "module": "Declara o módulo atual.\n\n```seed\nmodule name\n```",
        "return": "Retorna da função atual.\n\n```seed\nreturn value\n```",
        "pub": "Modificador de visibilidade pública.",
        "priv": "Modificador de visibilidade privada."
    };
    return keywordDocs[word];
}

// --- Compiler Execution & Diagnostics ---

async function compileActiveFile() {
    const editor = vscode.window.activeTextEditor;
    if (!editor || editor.document.languageId !== "seed") {
        vscode.window.showWarningMessage("Abra um arquivo .seed para compilar.");
        return;
    }
    await editor.document.save();
    await runCompiler(editor.document.uri.fsPath, "compile", editor.document);
}

async function runProgram() {
    const editor = vscode.window.activeTextEditor;
    if (!editor || editor.document.languageId !== "seed") {
        vscode.window.showWarningMessage("Abra um arquivo .seed para executar.");
        return;
    }
    await editor.document.save();
    await runCompiler(editor.document.uri.fsPath, "run", editor.document);
}

async function runTests() {
    const testsPath = vscode.workspace.getConfiguration("seed").get<string>("testsPath") || DEFAULT_TESTS;
    if (!fs.existsSync(testsPath)) {
        vscode.window.showErrorMessage(`Arquivo de testes nao encontrado: ${testsPath}`);
        return;
    }
    const document = await vscode.workspace.openTextDocument(testsPath);
    await runCompiler(testsPath, "test", document);
}

async function runCompiler(filePath: string, mode: "compile" | "run" | "test", document?: vscode.TextDocument) {
    const compilerPath = resolveCompilerPath();
    if (!fs.existsSync(compilerPath)) {
        compilerOnline = false;
        compilerVersion = "SEED offline";
        updateStatusForActiveEditor();
        vscode.window.showErrorMessage(`Compilador SEED nao encontrado: ${compilerPath}`);
        return;
    }

    const args = buildCompilerArgs(filePath, mode);
    output.clear();
    output.appendLine(`> "${compilerPath}" ${args.map(a => `"${a}"`).join(" ")}`);
    output.show(true);

    const result = await spawnProcess(compilerPath, args, path.dirname(filePath));
    output.append(result.stdout);
    output.append(result.stderr);

    const doc = document || await vscode.workspace.openTextDocument(filePath);
    const parsed = parseCompilerDiagnostics(result.stdout + "\n" + result.stderr, doc);
    compilerDiagnostics.set(doc.uri.toString(), parsed);
    refreshDiagnostics(doc);

    if (result.code === 0) {
        vscode.window.setStatusBarMessage(`SEED ${mode === "test" ? "testes" : "compilação"} concluído`, 4000);
    } else {
        vscode.window.showErrorMessage(`SEED retornou código ${result.code}. Veja o canal de saída SEED.`);
    }
}

function buildCompilerArgs(filePath: string, mode: "compile" | "run" | "test"): string[] {
    if (mode === "run") {
        return ["run", filePath];
    }
    if (mode === "test") {
        return ["test", filePath];
    }
    return ["build", filePath];
}

function resolveCompilerPath(): string {
    return vscode.workspace.getConfiguration("seed").get<string>("compilerPath") || DEFAULT_COMPILER;
}

function spawnProcess(command: string, args: string[], cwd: string): Promise<{ code: number | null; stdout: string; stderr: string }> {
    return new Promise(resolve => {
        const child = spawn(command, args, { cwd, shell: false });
        let stdout = "";
        let stderr = "";
        child.stdout.on("data", data => {
            stdout += data.toString();
        });
        child.stderr.on("data", data => {
            stderr += data.toString();
        });
        child.on("error", error => {
            stderr += error.message;
            resolve({ code: -1, stdout, stderr });
        });
        child.on("close", code => resolve({ code, stdout, stderr }));
    });
}

function parseCompilerDiagnostics(text: string, document: vscode.TextDocument): vscode.Diagnostic[] {
    const found: vscode.Diagnostic[] = [];
    const lines = text.split(/\r?\n/);
    for (const line of lines) {
        const withPosition = line.match(/^(?:(.*?):)?(\d+):(\d+):\s*(error|warning|warn|note|info)\s*(?:\[?(SEED\d{3})\]?)?:?\s*(.*)$/iu);
        if (withPosition) {
            const lineNumber = Math.max(0, Number(withPosition[2]) - 1);
            const colNumber = Math.max(0, Number(withPosition[3]) - 1);
            const severity = severityFromText(withPosition[4]);
            found.push(makeDiagnostic(document, lineNumber, colNumber, withPosition[6] || line, severity, withPosition[5]));
            continue;
        }

        const seedCode = line.match(/\b(SEED\d{3})\b[:\s-]*(.*)$/u);
        if (seedCode && seedCode[1] !== "SEED000") {
            const lineNumber = guessLineForCode(document, seedCode[1]);
            const severity = seedCode[1] === "SEED100" ? vscode.DiagnosticSeverity.Error : vscode.DiagnosticSeverity.Warning;
            found.push(makeDiagnostic(document, lineNumber, 0, seedCode[2] || seedCode[1], severity, seedCode[1]));
        }
    }
    return found;
}

function severityFromText(value: string): vscode.DiagnosticSeverity {
    const lower = value.toLowerCase();
    if (lower === "error") {
        return vscode.DiagnosticSeverity.Error;
    }
    if (lower === "warning" || lower === "warn") {
        return vscode.DiagnosticSeverity.Warning;
    }
    if (lower === "info") {
        return vscode.DiagnosticSeverity.Information;
    }
    return vscode.DiagnosticSeverity.Hint;
}

function makeDiagnostic(document: vscode.TextDocument, line: number, col: number, message: string, severity: vscode.DiagnosticSeverity, code?: string): vscode.Diagnostic {
    const safeLine = Math.min(Math.max(line, 0), Math.max(document.lineCount - 1, 0));
    const textLine = document.lineAt(safeLine);
    const start = Math.min(col, textLine.text.length);
    const end = textLine.text.length === 0 ? start : Math.min(textLine.text.length, Math.max(start + 1, textLine.range.end.character));
    const diagnostic = new vscode.Diagnostic(
        new vscode.Range(safeLine, start, safeLine, end),
        message.trim(),
        severity
    );
    diagnostic.source = "SEED";
    if (code) {
        diagnostic.code = code;
    }
    return diagnostic;
}

function refreshDiagnostics(document: vscode.TextDocument) {
    if (document.languageId !== "seed") {
        return;
    }
    const local = analyzeSeedDocument(document);
    const compiler = compilerDiagnostics.get(document.uri.toString()) || [];
    diagnostics.set(document.uri, [...local, ...compiler]);
    updateStatusForActiveEditor();
}

function analyzeSeedDocument(document: vscode.TextDocument): vscode.Diagnostic[] {
    const text = document.getText();
    const result: vscode.Diagnostic[] = [];
    result.push(...braceDiagnostics(document));

    // Check for non-existent imports
    const imports = parseFileImports(text);
    for (const modPath of imports.importedModules) {
        if (!modulesIndex.has(modPath)) {
            const lines = text.split(/\r?\n/);
            let errorLine = 0;
            for (let i = 0; i < lines.length; i++) {
                if (lines[i].includes("import") && (lines[i].includes(modPath.replace("stdlib.", "std::")) || lines[i].includes(modPath))) {
                    errorLine = i;
                    break;
                }
            }
            result.push(makeDiagnostic(document, errorLine, 0, `SEED007: Módulo importado não encontrado: ${modPath}`, vscode.DiagnosticSeverity.Error, "SEED007"));
        }
    }

    // Check for non-existent symbols in imports
    for (const [symbolName, moduleName] of imports.importedSymbols) {
        const mod = modulesIndex.get(moduleName);
        if (mod && !mod.symbols.has(symbolName)) {
            const lines = text.split(/\r?\n/);
            let errorLine = 0;
            for (let i = 0; i < lines.length; i++) {
                if (lines[i].includes(symbolName)) {
                    errorLine = i;
                    break;
                }
            }
            result.push(makeDiagnostic(document, errorLine, 0, `SEED008: Símbolo '${symbolName}' não encontrado no módulo ${moduleName}`, vscode.DiagnosticSeverity.Error, "SEED008"));
        }
    }

    if (/fib\s*\(\s*n\s*-\s*1\s*\)\s*\+\s*fib\s*\(\s*n\s*-\s*2\s*\)/u.test(text)) {
        result.push(makeDiagnostic(document, findLine(document, /fib\s*\(\s*n\s*-\s*1\s*\)/u), 0, "SEED002: Fibonacci recursivo ingênuo O(2^n). Considere math.fib_matrix(n).", vscode.DiagnosticSeverity.Warning, "SEED002"));
    }

    if (/SELECT\s+\*/iu.test(text) && !/\bLIMIT\b/iu.test(text)) {
        result.push(makeDiagnostic(document, findLine(document, /SELECT\s+\*/iu), 0, "SEED003: consulta SELECT * sem LIMIT envelhece mal.", vscode.DiagnosticSeverity.Warning, "SEED003"));
    }

    if (hasNonExhaustiveMatch(text, "Result", ["Ok", "Err"])) {
        result.push(makeDiagnostic(document, findLine(document, /\bmatch\b/u), 0, "SEED005: match de Result precisa cobrir Ok e Err.", vscode.DiagnosticSeverity.Warning, "SEED005"));
    }

    if (hasNonExhaustiveMatch(text, "Option", ["Some", "None"])) {
        result.push(makeDiagnostic(document, findLine(document, /\bmatch\b/u), 0, "SEED005: match de Option precisa cobrir Some e None.", vscode.DiagnosticSeverity.Warning, "SEED005"));
    }

    if (hasUncheckedFallibleCall(text)) {
        result.push(makeDiagnostic(document, findLine(document, /\b(sql_query|fs\.read_text|parse_[A-Za-z0-9_]*)\s*\(/u), 0, "SEED006: chamada falhável sem match nem operador ?.", vscode.DiagnosticSeverity.Warning, "SEED006"));
    }

    return result;
}

function braceDiagnostics(document: vscode.TextDocument): vscode.Diagnostic[] {
    const text = document.getText();
    const stack: Array<{ char: string; offset: number }> = [];
    const pairs: Record<string, string> = { "}": "{", ")": "(", "]": "[" };
    const opens = new Set(["{", "(", "["]);
    const diagnosticsList: vscode.Diagnostic[] = [];
    let inString = false;
    let inLineComment = false;
    let inBlockComment = false;

    for (let i = 0; i < text.length; i++) {
        const ch = text[i];
        const next = text[i + 1];
        if (inLineComment) {
            if (ch === "\n") {
                inLineComment = false;
            }
            continue;
        }
        if (inBlockComment) {
            if (ch === "*" && next === "/") {
                inBlockComment = false;
                i++;
            }
            continue;
        }
        if (inString) {
            if (ch === "\\" && next) {
                i++;
            } else if (ch === "\"") {
                inString = false;
            }
            continue;
        }
        if (ch === "/" && next === "/") {
            inLineComment = true;
            i++;
            continue;
        }
        if (ch === "/" && next === "*") {
            inBlockComment = true;
            i++;
            continue;
        }
        if (ch === "\"") {
            inString = true;
            continue;
        }
        if (opens.has(ch)) {
            stack.push({ char: ch, offset: i });
        } else if (pairs[ch]) {
            const last = stack.pop();
            if (!last || last.char !== pairs[ch]) {
                const position = document.positionAt(i);
                diagnosticsList.push(new vscode.Diagnostic(new vscode.Range(position, position.translate(0, 1)), `Bracket '${ch}' sem par correspondente.`, vscode.DiagnosticSeverity.Error));
            }
        }
    }
    for (const item of stack) {
        const position = document.positionAt(item.offset);
        diagnosticsList.push(new vscode.Diagnostic(new vscode.Range(position, position.translate(0, 1)), `Bracket '${item.char}' sem fechamento.`, vscode.DiagnosticSeverity.Error));
    }
    for (const diagnostic of diagnosticsList) {
        diagnostic.source = "SEED";
    }
    return diagnosticsList;
}

function hasNonExhaustiveMatch(text: string, typeName: string, variants: string[]): boolean {
    const mentionsType = new RegExp(`\\b${typeName}\\s*\\[`, "u").test(text) || variants.some(v => new RegExp(`\\b${v}\\s*\\(`, "u").test(text));
    if (!mentionsType || !/\bmatch\b/u.test(text)) {
        return false;
    }
    if (/_\s*=>/u.test(text)) {
        return false;
    }
    return variants.some(variant => !new RegExp(`\\b${variant}\\s*\\(`, "u").test(text));
}

function hasUncheckedFallibleCall(text: string): boolean {
    const fallible = /\b(sql_query|fs\.read_text|parse_[A-Za-z0-9_]*)\s*\(/u;
    if (!fallible.test(text)) {
        return false;
    }
    const lines = text.split(/\r?\n/u);
    return lines.some(line => fallible.test(line) && !/\bfn\s+parse_[A-Za-z0-9_]*\s*\(/u.test(line) && !line.includes("?") && !/\bmatch\b/u.test(line));
}

function findLine(document: vscode.TextDocument, regex: RegExp): number {
    for (let i = 0; i < document.lineCount; i++) {
        if (regex.test(document.lineAt(i).text)) {
            return i;
        }
    }
    return 0;
}

function guessLineForCode(document: vscode.TextDocument, code: string): number {
    const patterns: Record<string, RegExp> = {
        SEED001: /\b(for|swap|trocar)\b/u,
        SEED002: /fib\s*\(/u,
        SEED003: /SELECT\s+\*/iu,
        SEED004: /\+/u,
        SEED005: /\bmatch\b/u,
        SEED006: /\b(sql_query|fs\.read_text|parse_[A-Za-z0-9_]*)\s*\(/u,
        SEED100: /\b(net|unsafe|syscall)\b/u
    };
    return findLine(document, patterns[code] || /\S/u);
}

function formatSeed(text: string): string {
    // Correct space formatting around brackets and brace blocks
    let formattedText = text;
    formattedText = formattedText.replace(/(\))\{/g, "$1 {");
    formattedText = formattedText.replace(/([a-zA-Z0-9_])\{/g, "$1 {");

    const lines = formattedText.replace(/\s+$/u, "").split(/\r?\n/u);
    let indent = 0;
    const formatted: string[] = [];

    for (const rawLine of lines) {
        const trimmed = rawLine.trim();
        if (trimmed.length === 0) {
            formatted.push("");
            continue;
        }
        if (/^[}\])]/u.test(trimmed)) {
            indent = Math.max(0, indent - 1);
        }
        formatted.push(`${" ".repeat(indent * 4)}${trimmed}`);
        const opens = countStructureChars(trimmed, "{([");
        const closes = countStructureChars(trimmed, "})]");
        if (!trimmed.startsWith("//")) {
            indent = Math.max(0, indent + opens - closes);
        }
    }
    return `${formatted.join("\n")}\n`;
}

function countStructureChars(line: string, chars: string): number {
    let count = 0;
    let inString = false;
    for (let i = 0; i < line.length; i++) {
        const ch = line[i];
        if (ch === "\"" && line[i - 1] !== "\\") {
            inString = !inString;
        }
        if (!inString && chars.includes(ch)) {
            count++;
        }
    }
    return count;
}

async function checkCompiler() {
    const compilerPath = resolveCompilerPath();
    if (!fs.existsSync(compilerPath)) {
        compilerOnline = false;
        compilerVersion = "SEED offline";
        updateStatusForActiveEditor();
        return;
    }
    const result = await spawnProcess(compilerPath, ["version"], path.dirname(compilerPath));
    compilerOnline = result.code === 0 || result.code === 1;
    const banner = (result.stdout + result.stderr).match(/SEED\s+[^\r\n]+/u);
    compilerVersion = banner ? banner[0] : "SEED online";
    updateStatusForActiveEditor();
}

function updateStatusForActiveEditor() {
    const editor = vscode.window.activeTextEditor;
    const counts = editor?.document.languageId === "seed" ? countDiagnostics(editor.document.uri) : { errors: 0, warnings: 0 };
    const state = compilerOnline ? compilerVersion : "SEED offline";
    status.text = `$(circle-filled) ${state}  E:${counts.errors} W:${counts.warnings}`;
    if (!compilerOnline || counts.errors > 0) {
        status.color = new vscode.ThemeColor("charts.red");
    } else if (counts.warnings > 0) {
        status.color = new vscode.ThemeColor("charts.yellow");
    } else {
        status.color = new vscode.ThemeColor("charts.green");
    }
}

function countDiagnostics(uri: vscode.Uri): SeverityCounts {
    const current = diagnostics.get(uri) || [];
    return current.reduce<SeverityCounts>((acc, item) => {
        if (item.severity === vscode.DiagnosticSeverity.Error) {
            acc.errors++;
        } else if (item.severity === vscode.DiagnosticSeverity.Warning) {
            acc.warnings++;
        }
        return acc;
    }, { errors: 0, warnings: 0 });
}

function maybeShowWelcome(context: vscode.ExtensionContext) {
    const enabled = vscode.workspace.getConfiguration("seed").get<boolean>("showWelcomeOnFirstSeedFile");
    if (!enabled || context.globalState.get<boolean>("seed.welcomeShown")) {
        return;
    }
    context.globalState.update("seed.welcomeShown", true);
    showWelcome(context, false);
}

function showWelcome(context: vscode.ExtensionContext, explicit: boolean) {
    const panel = vscode.window.createWebviewPanel("seedWelcome", "SEED", vscode.ViewColumn.One, {
        enableScripts: true
    });
    panel.webview.html = welcomeHtml();
    panel.webview.onDidReceiveMessage(async message => {
        if (message.command === "start") {
            panel.dispose();
            await vscode.workspace.openTextDocument({
                language: "seed",
                content: "module hello\n\nimport std::io\n\nfn main() effect io -> Int {\n    print(\"Hello SEED\\n\")\n    return 0\n}\n"
            }).then(doc => vscode.window.showTextDocument(doc));
        }
        if (message.command === "docs") {
            await openIfExists("D:\\SEED\\docs\\language.md");
        }
        if (message.command === "examples") {
            await openIfExists("D:\\SEED\\examples\\result_match.seed");
        }
    }, undefined, context.subscriptions);

    if (explicit) {
        context.globalState.update("seed.welcomeShown", true);
    }
}

async function openIfExists(filePath: string) {
    if (!fs.existsSync(filePath)) {
        vscode.window.showWarningMessage(`Arquivo nao encontrado: ${filePath}`);
        return;
    }
    const doc = await vscode.workspace.openTextDocument(filePath);
    await vscode.window.showTextDocument(doc);
}

function welcomeHtml(): string {
    return `<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body{background:#020402;color:#b8f7c1;font-family:Consolas,monospace;margin:0;padding:32px;line-height:1.5}
h1{color:#28e0d4;margin:0 0 8px;font-size:32px}
h2{color:#9be564;margin-top:28px}
code,pre{background:#071107;color:#ffd866;border:1px solid #174d2b;border-radius:6px}
pre{padding:16px;overflow:auto}
button{background:#133d24;color:#b8f7c1;border:1px solid #66ff99;border-radius:6px;padding:10px 14px;margin-right:8px;cursor:pointer}
button:hover{background:#174d2b}
a{color:#28e0d4}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:16px;margin-top:20px}
.card{border:1px solid #174d2b;background:#050805;border-radius:8px;padding:16px}
</style>
</head>
<body>
<h1>SEED 1.0.0 - A Linguagem Viva</h1>
<p>Linguagem experimental com efeitos explicitos, contratos, ADTs, <code>match</code> exaustivo e propagacao por <code>?</code>.</p>
<p>
<button onclick="send('start')">Comecar a Programar</button>
<button onclick="send('docs')">Abrir documentacao</button>
<button onclick="send('examples')">Abrir exemplos</button>
</p>
<div class="grid">
<div class="card"><h2>Compilar</h2><p><code>Ctrl+Shift+B</code> chama o compilador configurado em <code>seed.compilerPath</code>.</p></div>
<div class="card"><h2>Executar</h2><p><code>F5</code> executa o fluxo de run da SEED para o arquivo atual.</p></div>
<div class="card"><h2>Testar</h2><p><code>Ctrl+Shift+T</code> roda <code>D:\\SEED\\tests\\seed_selftest.seed</code>.</p></div>
<div class="card"><h2>Sonhos</h2><p><code>SEED: Mostrar painel de sonhos</code> abre o log evolutivo local.</p></div>
</div>
<h2>Exemplo rapido</h2>
<pre><code>fn load(path: String) effect fs -> Result[String, Error] {
    let body = fs.read_text(path)?
    return Ok(body)
}

match load("seed.toml") {
    Ok(body) => print(body)
    Err(e) => print(e.message)
}</code></pre>
<script>
const vscode = acquireVsCodeApi();
function send(command){vscode.postMessage({command});}
</script>
</body>
</html>`;
}

function showDreamsPanel(context: vscode.ExtensionContext) {
    const panel = vscode.window.createWebviewPanel("seedDreams", "SEED Dreams", vscode.ViewColumn.Beside, {
        enableScripts: false,
        retainContextWhenHidden: true
    });
    const log = fs.existsSync(DREAM_LOG) ? fs.readFileSync(DREAM_LOG, "utf8") : "Nenhum sonho registrado ainda.";
    const reviewDir = "D:\\SEED\\dreams";
    const reviews = fs.existsSync(reviewDir)
        ? fs.readdirSync(reviewDir).filter(file => file.endsWith(".seedreview"))
        : [];
    panel.webview.html = dreamsHtml(log, reviews);
    context.subscriptions.push(panel);
}

function dreamsHtml(log: string, reviews: string[]): string {
    const reviewItems = reviews.length === 0
        ? "<li>Nenhuma revisao pendente.</li>"
        : reviews.map(file => `<li>${escapeHtml(file)}</li>`).join("");
    return `<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<style>
body{background:#020402;color:#b8f7c1;font-family:Consolas,monospace;padding:24px}
h1{color:#28e0d4}
h2{color:#9be564}
pre{background:#071107;border:1px solid #174d2b;border-radius:6px;padding:16px;white-space:pre-wrap}
li{margin:6px 0}
</style>
</head>
<body>
<h1>SEED Dreams</h1>
<h2>Log noturno</h2>
<pre>${escapeHtml(log)}</pre>
<h2>Revisoes pendentes</h2>
<ul>${reviewItems}</ul>
</body>
</html>`;
}

function escapeHtml(value: string): string {
    return value
        .replace(/&/gu, "&amp;")
        .replace(/</gu, "&lt;")
        .replace(/>/gu, "&gt;")
        .replace(/"/gu, "&quot;");
}
