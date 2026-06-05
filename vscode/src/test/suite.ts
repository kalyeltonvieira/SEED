import * as fs from "fs";
import * as path from "path";
import * as assert from "assert";

console.log("=== SEED Language Extension Test Suite ===");

function testLanguageRegistration() {
    console.log("Testing Language Registration...");
    const pkgPath = path.join(__dirname, "../../package.json");
    const pkg = JSON.parse(fs.readFileSync(pkgPath, "utf8"));

    const lang = pkg.contributes.languages[0];
    assert.strictEqual(lang.id, "seed", "Language ID must be 'seed'");
    assert.ok(lang.aliases.includes("SEED"), "Alias SEED missing");
    assert.ok(lang.aliases.includes("seed"), "Alias seed missing");
    assert.ok(lang.extensions.includes(".seed"), "Extension .seed missing");
    assert.strictEqual(lang.configuration, "./language-config.json", "Language configuration path incorrect");
    console.log("[OK] Language Registration");
}

function testIcons() {
    console.log("Testing Icons Configuration...");
    const iconThemePath = path.join(__dirname, "../../icons/seed-icon-theme.json");
    const iconTheme = JSON.parse(fs.readFileSync(iconThemePath, "utf8"));

    assert.ok(iconTheme.iconDefinitions["seed-file"], "seed-file icon definition missing");
    
    // Check that relative path works and file exists
    const svgRelativePath = iconTheme.iconDefinitions["seed-file"].iconPath;
    assert.strictEqual(svgRelativePath, "./seed-icon.svg", "Icon SVG path must be relative to theme JSON (./seed-icon.svg)");
    
    const svgAbsolutePath = path.join(path.dirname(iconThemePath), svgRelativePath);
    assert.ok(fs.existsSync(svgAbsolutePath), `Icon SVG file does not exist at: ${svgAbsolutePath}`);
    
    assert.strictEqual(iconTheme.fileExtensions.seed, "seed-file", "Extension .seed should map to seed-file icon");
    assert.strictEqual(iconTheme.fileNames["seed.toml"], "seed-file", "seed.toml should show seed icon");
    assert.strictEqual(iconTheme.fileNames["seed.lock"], "seed-file", "seed.lock should show seed icon");
    assert.strictEqual(iconTheme.fileNames["Seedfile"], "seed-file", "Seedfile should show seed icon");
    assert.strictEqual(iconTheme.languageIds.seed, "seed-file", "Language seed should show seed icon");
    console.log("[OK] Icons Configuration");
}

function testSyntaxHighlighting() {
    console.log("Testing Syntax Highlighting...");
    const grammarPath = path.join(__dirname, "../../syntaxes/seed.tmLanguage.json");
    const grammarContent = fs.readFileSync(grammarPath, "utf8");
    const grammar = JSON.parse(grammarContent);

    assert.strictEqual(grammar.scopeName, "source.seed", "Scope name must be source.seed");
    
    // Since patterns are inside repository objects, let's verify using serialization string
    assert.ok(grammarContent.includes("pub"), "pub keyword missing from grammar");
    assert.ok(grammarContent.includes("mut"), "mut keyword missing from grammar");
    assert.ok(grammarContent.includes("loop"), "loop keyword missing from grammar");
    assert.ok(grammarContent.includes("fn"), "fn keyword missing from grammar");
    assert.ok(grammarContent.includes("struct"), "struct keyword missing from grammar");
    assert.ok(grammarContent.includes("i32"), "i32 type missing from grammar");
    assert.ok(grammarContent.includes("f64"), "f64 type missing from grammar");
    
    console.log("[OK] Syntax Highlighting Patterns");
}

function testIndentation() {
    console.log("Testing Indentation Rules...");
    const langConfigPath = path.join(__dirname, "../../language-config.json");
    const langConfig = JSON.parse(fs.readFileSync(langConfigPath, "utf8"));

    const incIndent = new RegExp(langConfig.indentationRules.increaseIndentPattern);
    const decIndent = new RegExp(langConfig.indentationRules.decreaseIndentPattern);

    assert.ok(incIndent.test("fn main() {"), "Should increase indent for brace");
    assert.ok(incIndent.test("let x = ["), "Should increase indent for bracket");
    assert.ok(incIndent.test("call("), "Should increase indent for paren");
    assert.ok(incIndent.test("match x {"), "Should increase indent for match brace");
    assert.ok(incIndent.test("struct Point {"), "Should increase indent for struct brace");

    assert.ok(decIndent.test("}"), "Should decrease indent for brace");
    assert.ok(decIndent.test("]"), "Should decrease indent for bracket");
    assert.ok(decIndent.test(")"), "Should decrease indent for paren");
    console.log("[OK] Indentation rules");
}

// Minimal implementation of extension features to run as standalone tests
function formatSeed(text: string): string {
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
        const countStructureChars = (line: string, chars: string): number => {
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
        };
        const opens = countStructureChars(trimmed, "{([");
        const closes = countStructureChars(trimmed, "})]");
        if (!trimmed.startsWith("//")) {
            indent = Math.max(0, indent + opens - closes);
        }
    }
    return `${formatted.join("\n")}\n`;
}

interface FileImports {
    importedModules: string[];
    moduleAliases: Map<string, string>;
    importedSymbols: Map<string, string>;
}

function parseFileImports(text: string): FileImports {
    const importedModules: string[] = [];
    const moduleAliases = new Map<string, string>();
    const importedSymbols = new Map<string, string>();
    const lines = text.split(/\r?\n/);

    for (const line of lines) {
        const trimmed = line.trim();
        // Match braced imports with or without ::: import std::math::{sqrt, PI} or import std::math{sqrt, PI}
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
            continue; // Skip normal import processing for braced imports
        }

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
            if (segments.length > 1) {
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

function testFormatting() {
    console.log("Testing Formatting...");
    const before = `fn main(){
print("oi")
}`;
    const after = formatSeed(before);
    const expected = `fn main() {
    print("oi")
}
`;
    assert.strictEqual(after, expected, `Formatting output incorrect.\nExpected:\n${expected}\nGot:\n${after}`);
    console.log("[OK] Formatting");
}

function testImports() {
    console.log("Testing Imports Parser...");
    const code = `
        import std::io
        import std::math::{sqrt, PI}
        import app::auth::login
        use std::io::{print, println}
        use std::io{print, println}
        use std::collections::List
    `;
    const res = parseFileImports(code);
    assert.ok(res.importedModules.includes("stdlib.io"), "stdlib.io missing");
    assert.ok(res.importedModules.includes("stdlib.math"), "stdlib.math missing");
    assert.ok(res.importedSymbols.has("sqrt"), "sqrt symbol missing");
    assert.ok(res.importedSymbols.has("PI"), "PI symbol missing");
    assert.ok(res.importedSymbols.has("print"), "print symbol missing");
    assert.ok(res.importedSymbols.has("println"), "println symbol missing");
    assert.ok(res.importedSymbols.has("List"), "List symbol missing");
    
    assert.strictEqual(res.moduleAliases.get("io"), "stdlib.io", "io alias incorrect");
    assert.strictEqual(res.moduleAliases.get("math"), "stdlib.math", "math alias incorrect");
    console.log("[OK] Imports Parser");
}

function runAllTests() {
    testLanguageRegistration();
    testIcons();
    testSyntaxHighlighting();
    testIndentation();
    testFormatting();
    testImports();
    console.log("All tests passed successfully!");
}

runAllTests();
