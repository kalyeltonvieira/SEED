# SEED Language for VS Code

Extensao oficial local da linguagem SEED.

## Recursos

- Reconhecimento automatico de arquivos `.seed`.
- Syntax highlighting com gramatica TextMate.
- Autocomplete para palavras-chave, tipos, funcoes do arquivo e variaveis locais.
- Snippets para `fn`, `if`, `while`, `for`, `match`, `type`, `trait`, `impl`, `contract`, `genitor` e testes.
- Tema escuro `SEED Dark`.
- Icone de arquivo SEED via tema de icones `SEED File Icons`.
- Diagnosticos no painel Problems.
- Comandos para compilar, executar, rodar testes, formatar e abrir o painel de sonhos.
- Status bar com estado do compilador, erros, warnings e versao detectada.
- Welcome page para o primeiro arquivo `.seed`.

## Instalacao por copia

Copie a pasta:

```powershell
Copy-Item -Recurse -Force D:\SEED\vscode $env:USERPROFILE\.vscode\extensions\seed-language
```

Reinicie o VS Code e abra um arquivo `.seed`.

Para usar o tema:

1. Abra `Ctrl+Shift+P`.
2. Execute `Preferences: Color Theme`.
3. Escolha `SEED Dark`.

Para usar o icone de arquivo:

1. Abra `Ctrl+Shift+P`.
2. Execute `Preferences: File Icon Theme`.
3. Escolha `SEED File Icons`.

## Comandos

- `SEED: Compilar arquivo` (`Ctrl+Shift+B`)
- `SEED: Executar programa` (`F5`)
- `SEED: Rodar testes` (`Ctrl+Shift+T`)
- `SEED: Formatar documento`
- `SEED: Mostrar painel de sonhos`
- `SEED: Mostrar boas-vindas`

## Configuracoes

```json
{
  "seed.compilerPath": "D:\\SEED\\seed.exe",
  "seed.testsPath": "D:\\SEED\\tests\\seed_selftest.seed",
  "seed.outputDirectory": "",
  "seed.showWelcomeOnFirstSeedFile": true
}
```

## Empacotar como VSIX

```powershell
cd D:\SEED\vscode
npm install
npm run compile
npm run package
code --install-extension seed-language-1.0.0.vsix
```

O arquivo `out/extension.js` ja esta incluido para permitir instalacao por copia sem compilar TypeScript.
