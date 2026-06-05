# Guia de Referência e Especificação Formal da Linguagem SEED

Este documento fornece a documentação oficial da linguagem de programação **SEED**, cobrindo desde conceitos introdutórios e de fundação até a especificação formal do compilador e de seu sistema de tipos.

---

## 1. Introdução

### O que é Seed
SEED é uma linguagem de programação focada em **auditoria de efeitos, otimização evolucionária (mutação) e corretude por contratos**. Projetada para ser altamente legível e segura, ela compila diretamente para C++23 nativo, permitindo um runtime enxuto e de alta performance.

### Filosofia da linguagem
- **Transparência de Efeitos:** Todo efeito colateral (disco, rede, etc.) é explícito na assinatura da função.
- **Auditoria de Otimização:** O compilador sugere reescritas automáticas otimizadas baseadas em complexidade e fresh (frescor do código), mas o programador tem controle total sobre o aceite.
- **Programação Orientada a Propriedades:** Genitores descrevem famílias de implementações que sobrevivem com base em testes automatizados e fitness.

### Casos de uso
- APIs REST e servidores web seguros e de alta concorrência.
- CLI Tools portáveis e utilitários de sistema.
- Programas interativos com REPL embutido.
- Compilação direcionada para WebAssembly (WASM).

### Comparação com Rust, Go, Zig e TypeScript
| Característica | SEED | Rust | Go | Zig | TypeScript |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Segurança** | Controle de Efeitos | Ownership & Borrowing | GC & Safe Runtime | Manual (Safe mode) | GC & Dynamic |
| **Velocidade** | Compilação C++ | Nativa (LLVM) | Compilação Rápida | Nativa (LLVM) | JIT (V8) |
| **Concorrência** | Threads & Canais | Async & Thread-safety | Goroutines (CSP) | Async (Sem corotinas) | Event Loop (Single) |
| **Metaprogramação** | Genitores & Macros | Macros Declarativas/Proc | Reflection | `comptime` | Reflection limitada |
| **Sintaxe** | Simples, intuitiva | Complexa | Muito simples | Minimalista | Fluida, dinâmica |

### Roadmap
- **SEED 0.1:** Compilador inicial com 7 instruções primitivas de runtime.
- **SEED 0.2:** Adicionado REPL, suporte a classes em C++ e CLI.
- **SEED 0.3:** Dialeto de result-match (ADTs fechadas, operador `?`, auditoria de efeitos e contratos).
- **SEED 0.4:** Concorrência nativa melhorada e suporte a monorepo workspace.

---

## 2. Instalação

### Windows
Para instalar o SEED no Windows, clone o repositório em `D:\SEED` (ou diretório de sua escolha) e execute o script de instalação no PowerShell:
```powershell
.\install.ps1
```
Isso adiciona o executável ao `PATH` do sistema e configura a associação de arquivos `.seed` no registro do Windows para permitir duplo clique executável.

### Linux
Clone o repositório e execute a instalação via script shell:
```bash
./install.sh
```
Isso compilará o `seed.exe` (ou link binário `seed`) para `/usr/local/bin`.

### macOS
```bash
make install
```
Garante a instalação dos compiladores dependentes (Clang++ com suporte a C++23) via Homebrew.

### Docker
```dockerfile
FROM alpine:latest
RUN apk add --no-image-cache g++ make
COPY . /app
WORKDIR /app
RUN make compiler
ENTRYPOINT ["/app/seed"]
```

### Compilação manual
Se você deseja compilar manualmente o runtime C++ do SEED:
```bash
g++ -std=c++23 -O2 -o seed.exe runtime/seed_runtime.cpp runtime/seed.res -luser32 -lkernel32 -lshlwapi
```

---

## 3. Primeiro Programa

Escreva o código abaixo em um arquivo chamado `hello.seed`:
```seed
fn main() {
    println("Hello, Seed!")
}
```

### Explicação:
- **Função Principal:** `fn main()` define o ponto de entrada da sua aplicação.
- **Entrada da Aplicação:** `println` imprime o texto no console seguido de uma quebra de linha.
- **Compilação:** Execute o comando `seed.exe build hello.seed` para gerar o arquivo C++ correspondente e compilá-lo para `hello.seed.exe`.
- **Execução:** Execute `hello.seed.exe` ou rode diretamente `seed.exe run hello.seed`.

---

## FUNDAMENTOS

## 4. Comentários
```seed
// Comentário de linha única

/*
 Comentário de múltiplas linhas
 para documentar blocos de código
*/
```

## 5. Variáveis
```seed
let name = "rlzy"       // Imutável por padrão
mut count = 0           // Variável mutável
const PI = 3.14159      // Constante
```

### Mutabilidade
```seed
mut score = 10
score = 20              // OK!
```

## 6. Tipos Primitivos

### Inteiros
- **Sinalizados:** `i8`, `i16`, `i32` (padrão `int`), `i64`, `i128`
- **Não-sinalizados:** `u8` (byte), `u16`, `u32`, `u64`, `u128`

### Decimais
- `f32`, `f64` (padrão `float`)

### Boolean
- `bool` (`true` ou `false`)

### Char
- `char` (representando um caractere único ASCII ou UTF-8 codificado)

### String
- `string` (tipo string nativo com alocação dinâmica e suporte a UTF-8)

## 7. Inferência de Tipos
SEED infere o tipo das variáveis baseando-se no valor de atribuição à direita:
```seed
let age = 18            // Inferido como i32/int
```

## 8. Conversões
A conversão de tipos é explícita utilizando o método genérico `.to[T]()` ou conversões estáticas:
```seed
let x = 10
let y = x.to<f64>()
```

---

## OPERADORES

## 9. Operadores Matemáticos
- `+` (Soma)
- `-` (Subtração)
- `*` (Multiplicação)
- `/` (Divisão)
- `%` (Módulo)
- `**` (Exponenciação)

## 10. Operadores Lógicos
- `&&` (E lógico)
- `||` (OU lógico)
- `!` (Negação)

## 11. Comparação
- `==` (Igual a)
- `!=` (Diferente de)
- `>` (Maior que)
- `<` (Menor que)
- `>=` (Maior ou igual a)
- `<=` (Menor ou igual a)

## 12. Operadores de Atribuição
- `+=` (Soma e atribui)
- `-=` (Subtrai e atribui)
- `*=` (Multiplica e atribui)
- `/=` (Divide e atribui)

---

## CONTROLE DE FLUXO

## 13. If
```seed
if age >= 18 {
    println("adult")
}
```

## 14. Else
```seed
if age >= 18 {
    println("adult")
} else {
    println("minor")
}
```

## 15. Else If
```seed
if score > 90 {
    println("Excellent")
} else if score > 70 {
    println("Good")
} else {
    println("Regular")
}
```

## 16. Match
Estrutura de ramificação condicional para ADTs e padrões literais. Deve ser exaustiva:
```seed
match status {
    "active" => println("ok"),
    "banned" => println("blocked"),
    _ => println("unknown")
}
```

---

## LOOPS

## 17. Loop Infinito
```seed
loop {
    // executa infinitamente até um "break"
}
```

## 18. While
```seed
while count < 10 {
    count += 1
}
```

## 19. For
```seed
for item in items {
    println(item)
}
```

## 20. Range
Utiliza `..` para intervalos semi-abertos e `..=` para intervalos fechados:
```seed
for i in 0..10 {
    // 0 a 9
}
```

---

## COLEÇÕES

## 21. Arrays
Estruturas de tamanho dinâmico baseadas em vetor contíguo de memória:
```seed
let nums = [1, 2, 3]
```

## 22. Tuplas
Conjunto imutável de valores heterogêneos indexados:
```seed
let user = ("rlzy", 18)
```

## 23. Vetores
```seed
let users = Vec[string]()
```

## 24. Maps
```seed
let users = Map[string, User]()
```

## 25. Sets
```seed
let tags = Set[string]()
```

---

## FUNÇÕES

## 26. Funções
```seed
fn greet(name: string) {
    println(name)
}
```

## 27. Retorno
A última expressão no corpo da função sem ponto e vírgula é implicitamente retornada:
```seed
fn sum(a: i32, b: i32) -> i32 {
    a + b
}
```

## 28. Argumentos Nomeados
```seed
createUser(
    name: "rlzy",
    age: 18
)
```

## 29. Valores Padrão
```seed
fn greet(name = "guest") {
    println("Hello " + name)
}
```

## 30. Closures
```seed
let add = |a, b| a + b
```

---

## ORIENTAÇÃO A DADOS

## 31. Structs
```seed
struct User {
    name: string,
    age: i32
}
```

## 32. Métodos
```seed
impl User {
    fn greet(self) {
        println("Olá, eu sou " + self.name)
    }
}
```

## 33. Enums
```seed
enum Status {
    Active,
    Banned,
    Pending
}
```

## 34. Union Types
Muito útil para o mapeamento de APIs modernas sem sobrecarga:
```seed
let id: string | int = 123
```

---

## TRAITS

## 35. Traits
```seed
trait Display {
    fn display(self) -> string
}
```

## 36. Implementação
```seed
impl Display for User {
    fn display(self) -> string {
        self.name
    }
}
```

## 37. Generics
```seed
struct Box[T] {
    value: T
}
```

## 38. Constraints
```seed
fn print[T: Display](v: T) {
    println(v.display())
}
```

---

## GERENCIAMENTO DE MEMÓRIA

## 39. Ownership
O SEED utiliza passagem por valor para tipos primitivos e estruturas pequenas. Para grandes objetos e coleções, é aplicado um modelo de **ownership linear** com referências imutáveis por padrão.

## 40. Borrowing
A passagem de referências mutáveis requer anotação explícita (`&mut`):
```seed
fn atualizar(mut user: &mut User) {
    user.age += 1
}
```

## 41. Lifetimes
As referências são invalidadas após o término do escopo de alocação de seu dono originário, garantindo a ausência de ponteiros soltos.

## 42. Garbage Collector
**O SEED não possui Garbage Collector.** Toda desalocação ocorre em tempo de compilação ou na saída do escopo do método, mapeando para as primitivas `FREE`.

---

## MÓDULOS

## 43. Imports
```seed
import stdlib.io
```

## 44. Exports
```seed
pub fn hello() {
    println("Hello World")
}
```

## 45. Pacotes
```bash
seed add http
```

---

## ERROS

## 46. Result
```seed
data Result[T, E] {
    Ok(T)
    Err(E)
}
```

## 47. Option
```seed
data Option[T] {
    Some(T)
    None
}
```

## 48. Try (`?`)
O operador `?` propaga implicitamente um erro ou valor nulo:
```seed
let user = getUser()?
```

---

## CONCORRÊNCIA

## 49. Threads
```seed
spawn {
    println("Executando em background")
}
```

## 50. Channels
```seed
let ch = channel()
ch.send("mensagem")
let msg = ch.receive()
```

## 51. Async/Await
```seed
async fn fetch() -> Result[string, Error] {
    // busca assíncrona
}
```

## 52. Tasks
```seed
let t = task {
    // tarefa assíncrona concorrente
}
```

---

## MACROS

## 53. Macros Simples
Utilizadas para expansão textual e geração estruturada de código como `println!`.

## 54. Derive
Decoradores de tipo para implementar comportamento padrão automaticamente:
```seed
#[derive(Debug, Clone)]
struct Cargo {}
```

## 55. Procedural Macros
Macros compiladas em tempo de compilação que manipulam a AST estruturada. Muito importante para frameworks de mapeamento ORM e injeção de dependências.

---

## REFLEXÃO E METAPROGRAMAÇÃO

## 56. Type Info
```seed
typeof(user)
```

## 57. AST Macros
Macros que inspecionam a sintaxe do programa em tempo de compilação.

---

## INTEROPERABILIDADE

## 58. C FFI
```seed
extern fn printf(format: string, arg: i32)
```

## 59. JavaScript FFI
Garante execução no browser usando a flag `@js`:
```seed
#[js]
fn alert(msg: string)
```

## 60. WASM
O compilador SEED compila de forma limpa para WebAssembly:
```bash
seed build --wasm
```

---

## ECOSSISTEMA

## 61. Gerenciador de Pacotes
```bash
seed add prisma
```

## 62. Lockfile
`seed.lock` guarda a árvore determinística com checksums de todos os pacotes.

## 63. Workspace
Monorepo nativo definido no diretório do projeto:
```
apps/
  web/
packages/
  ui/
```

---

## TESTES

## 64. Unit Tests
Definidos com a diretiva de bloco `test`:
```seed
test "soma dois numeros" {
    assert(sum(2, 3) == 5)
}
```

## 65. Integration Tests
Escritos em arquivos dedicados sob a pasta `tests/`.

## 66. Benchmarks
```bash
seed bench
```

---

## FERRAMENTAS

## 67. Formatter
```bash
seed fmt
```

## 68. Linter
```bash
seed lint
```

## 69. Language Server
Suporte a LSP via extensão do editor.

## 70. REPL
Interpretador interativo executado via `seed.exe` sem parâmetros adicionais ou com `seed.exe repl`.

---

## BUILD SYSTEM

## 71. Build
```bash
seed build app.seed
```

## 72. Run
```bash
seed run app.seed
```

## 73. Release
```bash
seed build --release app.seed
```

---

## PROJETOS

## 74. CLI
```bash
seed new cli
```

## 75. API REST
```bash
seed new api
```

## 76. Web Server
```bash
seed new web
```

## 77. Desktop
```bash
seed new desktop
```

## 78. Mobile
```bash
seed new mobile
```

---

## DOCUMENTAÇÃO AVANÇADA

## 79. Arquitetura do Compilador
O compilador do SEED funciona de acordo com as seguintes fases lógicas:
1. **Lexer:** Lê o código-fonte `.seed` e gera a sequência linear de tokens.
2. **Parser:** Estrutura os tokens em uma árvore de sintaxe abstrata (AST).
3. **AST:** Representação hierárquica das instruções.
4. **Type Checker:** Valida tipagem estática e resolve coerões negociadas.
5. **MIR (Medium Intermediate Representation):** Otimiza o controle de fluxo.
6. **Optimizer (Mutator):** Avalia regras evolucionárias para otimização assintótica.
7. **Codegen:** Emite o arquivo C++23 correspondente e chama o Clang++/G++ para compilar em binário executável.

---

## 80. Especificação Formal

### Gramática EBNF (Simplificada)
```ebnf
ModuleDef     ::= "module" Identifier
ImportDef     ::= "import" Identifier
FuncDef       ::= "fn" Identifier "(" [Params] ")" ["->" Type] Block
Block         ::= "{" { Statement } "}"
Statement     ::= LetStatement | IfStatement | ForStatement | ReturnStatement | ExprStatement
LetStatement  ::= "let" ["mut"] Identifier [":" Type] "=" Expr [";"]
ExprStatement ::= Expr [";"]
```

### Sistema de tipos
- Estático e nominal.
- Suporta ADTs (Algebraic Data Types) fechados com casamento de padrões (`match`) exaustivo.

### Regras de inferência
- O SEED infere tipos locais na inicialização. Conversões numéricas implícitas de string para int ou vice-versa emitem alertas controlados (`SEED004`).

### Modelo de memória
- Baseado em tempo de vida e escopo. Ausência de coletor de lixo dinâmico. Alocações explícitas de structs e arrays são traduzidas para as primitivas nativas de alocação de heap.

### ABI (Application Binary Interface)
- Mapeia diretamente para a ABI da plataforma C++ subjacente (MSVC no Windows, System V AMD64 no Linux/macOS).

### Bytecode ou IR specification
- O runtime define sete instruções primitivas fundamentais que executam operações nativas:
  - `ALLOC size` (Aloca memória heap de tamanho especificado)
  - `FREE ptr` (Desaloca ponteiro de memória)
  - `READ ptr offset` (Lê endereço de memória)
  - `WRITE ptr offset val` (Escreve valor no endereço)
  - `CMP val1 val2` (Compara dois valores)
  - `JMP label` (Pula para rótulo condicional ou incondicional)
  - `SYSCALL id` (Executa chamada ao sistema operacional)
