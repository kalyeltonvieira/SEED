# Documento de Requisitos — SEED Advanced Features

## Introdução

Este documento descreve os requisitos para um conjunto de features avançadas da linguagem experimental SEED (versão base `0.3-result-match`). As features abrangem cinco domínios principais:

1. **Sistema de tipos avançado** — IR completo, Union types, Generics, Traits e Type Constraints
2. **Closures com captura de variáveis**
3. **Modelo de memória** — definição clara (GC ou Ownership) com detecção de leaks
4. **Package manager** — lockfile, semver e registry funcional
5. **Interoperabilidade e diagnósticos** — FFI para WASM/JS, stack traces e sistema de erros detalhado

A base existente inclui: IR básico (`compiler/ir.seed`), typechecker parcial com `Union`/`TypeVar` (`compiler/typechecker.seed`), runtime C++23, sete primitivas (`ALLOC`, `FREE`, `READ`, `WRITE`, `CMP`, `JMP`, `SYSCALL`) e o dialeto `0.3-result-match` com `data`, `match` e propagação `?`.

---

## Glossário

- **IR (Intermediate Representation)**: Representação intermediária entre a AST gerada pelo parser e o backend de geração de código; nível de abstração independente de alvo.
- **SSA (Static Single Assignment)**: Forma do IR onde cada variável recebe exatamente um valor; simplifica análise de fluxo.
- **Union Type**: Tipo que pode assumir qualquer um de um conjunto fechado de tipos alternativos (`A | B | C`).
- **Generic**: Construção paramétrica em que um tipo ou função aceita parâmetros de tipo em tempo de compilação (`T`, `K`, `V`).
- **Trait**: Interface nominal que declara um conjunto de assinaturas de função que um tipo deve implementar.
- **Type Constraint (Where Clause)**: Restrição sobre um parâmetro de tipo que exige que ele implemente determinados Traits (`where T: Display`).
- **Closure**: Função de primeira classe que captura variáveis do escopo léxico circundante.
- **Upvalue**: Variável capturada por uma Closure a partir do escopo externo.
- **GC (Garbage Collector)**: Gerenciamento automático de memória que libera objetos não alcançáveis.
- **Ownership**: Modelo de gerenciamento de memória em que cada valor tem um único dono; liberação ocorre ao fim do escopo do dono.
- **Leak**: Alocação de memória que nunca é liberada, causando crescimento ilimitado do uso de memória.
- **Package Manager**: Ferramenta que instala, atualiza e resolve dependências de pacotes SEED.
- **Lockfile**: Arquivo gerado pelo Package Manager que registra as versões exatas de todas as dependências resolvidas.
- **SemVer**: Esquema de versionamento `MAJOR.MINOR.PATCH` conforme a especificação semver.org.
- **Registry**: Servidor ou repositório central de onde o Package Manager baixa pacotes publicados.
- **FFI (Foreign Function Interface)**: Mecanismo para chamar funções externas a partir de código SEED.
- **WASM (WebAssembly)**: Formato binário portável de baixo nível executável em navegadores e runtimes WASM.
- **Stack Trace**: Sequência de chamadas ativas no momento em que um erro ou pânico ocorre.
- **Diagnostic**: Mensagem estruturada emitida pelo compilador com severidade, código, localização e sugestão.
- **Compiler**: O compilador orquestrador descrito em `compiler/compiler.seed`.
- **Typechecker**: O verificador de tipos descrito em `compiler/typechecker.seed`.
- **IR_Generator**: O componente que transforma AST em IRModule, descrito em `compiler/ir.seed`.
- **Codegen**: O componente que transforma IRModule em código-alvo (C++23 ou bytecode).
- **Parser**: O componente que transforma tokens em AST.
- **Runtime**: O runtime C++23 em `runtime/seed_runtime.cpp`.
- **PackageManager**: O componente que gerencia dependências de projetos SEED.
- **Registry_Client**: Subcomponente do PackageManager responsável pela comunicação com o Registry.
- **ErrorReporter**: Componente centralizado de formatação e emissão de Diagnostics.
- **MemoryTracker**: Componente de rastreamento de alocações e detecção de leaks.

---

## Requisitos

---

### Requisito 1: IR Completo com SSA

**User Story:** Como desenvolvedor do compilador SEED, quero uma IR completa em forma SSA entre a AST e o Codegen, para que as otimizações sejam aplicadas de forma independente do alvo e os novos tipos avançados sejam representados corretamente.

#### Critérios de Aceitação

1. THE IR_Generator SHALL transformar qualquer ASTNode válido produzido pelo Parser em um IRModule bem formado, sem nós AST remanescentes no IRModule resultante.
2. WHEN a IR_Generator processa uma função, THE IR_Generator SHALL garantir que cada IRValue definido em um IRBlock seja definido exatamente uma vez (propriedade SSA).
3. THE IR_Generator SHALL representar Union types como uma instrução IR de tag (`OP_UNION_TAG`) seguida de instruções de acesso por variante (`OP_UNION_ACCESS`).
4. THE IR_Generator SHALL representar instâncias de Generics como IRFunction com os type parameters substituídos (monomorphization) antes do passo de Codegen.
5. THE IR_Generator SHALL representar Traits como despacho indireto via tabela de métodos virtuais (`vtable`) alocada como IRConstant.
6. THE IR_Generator SHALL representar Closures como um IRValue composto de ponteiro para função e registro de captura (`OP_MAKE_CLOSURE`), conforme os opcodes já declarados em `compiler/ir.seed`.
7. WHEN o IRModule contém um ciclo de CFG (Control Flow Graph), THE IR_Generator SHALL inserir nós PHI (`OP_PHI`) nos pontos de convergência de acordo com a construção SSA.
8. THE IR_Generator SHALL emitir um Diagnostic `SEED010` com localização de linha e coluna para cada ASTNode que não possui lowering definido.
9. FOR ALL IRModules M produzidos a partir de uma AST válida, serializar M e desserializar M SHALL produzir um IRModule estruturalmente equivalente a M (propriedade de round-trip).
10. WHEN a opção `--debug` está ativa, THE IR_Generator SHALL emitir uma representação textual legível do IRModule antes do passo de Codegen.

---

### Requisito 2: Union Types Completos no Type System

**User Story:** Como programador SEED, quero Union types verificados estaticamente, para que o compilador detecte em tempo de compilação quando uma variante não é tratada, sem necessidade de coerções manuais.

#### Critérios de Aceitação

1. THE Typechecker SHALL reconhecer a sintaxe `A | B | C` em posição de anotação de tipo e construir um `Type` com `kind = "Union"` e `params` listando cada tipo membro.
2. WHEN uma expressão `match` sobre um valor do tipo `Union[A, B, C]` omite pelo menos uma variante, THE Typechecker SHALL emitir o Diagnostic `SEED005` indicando as variantes ausentes.
3. WHEN todos os braços de um `match` sobre `Union` estão presentes, THE Typechecker SHALL aceitar a expressão sem emitir `SEED005`.
4. THE Typechecker SHALL unificar `Union[A, B]` com um tipo `T` se e somente se `T` for membro do union ou `T` for `Any`.
5. WHEN um valor do tipo `A` é atribuído a uma variável anotada como `A | B`, THE Typechecker SHALL aceitar a atribuição sem erro.
6. WHEN um valor do tipo `C` é atribuído a uma variável anotada como `A | B` e `C` não é subconjunto de `A | B`, THE Typechecker SHALL emitir um Diagnostic `SEED007` com sugestão de conversão.
7. THE Typechecker SHALL rejeitar definições de Union types circulares diretas (e.g., `type X = X | Int`) e emitir o Diagnostic `SEED008`.
8. FOR ALL tipos Union `U` e valores `v` de tipo `T` onde `T` pertence a `U`, verificar `v` como `U` e depois verificar cada braço do match SHALL resultar no mesmo tipo de retorno produzido pela verificação direta de `v` como `T` (propriedade metamórfica de exhaustividade).

---

### Requisito 3: Generics Completos com Type Parameters

**User Story:** Como programador SEED, quero funções e tipos genéricos com parâmetros de tipo, para que eu possa escrever código reutilizável sem duplicação e com verificação estática completa.

#### Critérios de Aceitação

1. THE Typechecker SHALL aceitar declarações de funções na forma `fn nome[T, K](a: T, b: K) -> T` e registrar `T` e `K` como TypeVars no `GenericContext`.
2. THE Typechecker SHALL aceitar declarações de tipos na forma `data Container[T] { Value(T) }` e instanciar `T` como TypeVar associado ao tipo declarado.
3. WHEN uma função genérica é chamada com argumentos concretos, THE Typechecker SHALL inferir os type arguments pelo algoritmo de unificação Hindley-Milner e produzir uma instância monomorfa.
4. WHEN os type arguments inferidos violam uma Type Constraint declarada (ver Requisito 5), THE Typechecker SHALL emitir o Diagnostic `SEED011` com o nome do parâmetro, a constraint violada e a localização da chamada.
5. THE IR_Generator SHALL produzir, para cada instância concreta de uma função ou tipo genérico, uma cópia monomorfa no IRModule com um nome derivado do nome base e dos type arguments (`nome__T__K`).
6. WHEN dois sítios de chamada instanciam a mesma função genérica com os mesmos type arguments concretos, THE IR_Generator SHALL reutilizar a mesma função monomorfa no IRModule (sem duplicação).
7. FOR ALL funções genéricas `f[T](x: T) -> T` e tipos concretos `A`, `B`, THE Typechecker SHALL aceitar `f[A]` e `f[B]` sem erro, e o resultado de `f[A](v)` SHALL ter tipo `A` (propriedade de identidade de tipo).

---

### Requisito 4: Traits e Interfaces Completos

**User Story:** Como programador SEED, quero declarar Traits que descrevem comportamentos e implementá-los para tipos concretos, para que o compilador verifique que um tipo satisfaz um contrato antes de ser usado em posição que exija aquele Trait.

#### Critérios de Aceitação

1. THE Typechecker SHALL aceitar a declaração `trait Display { fn format(self) -> String }` e registrar `Display` com suas assinaturas de método no ambiente de tipos.
2. THE Typechecker SHALL aceitar a declaração `impl Display for MeuTipo { fn format(self) -> String { ... } }` e verificar que cada assinatura implementada é compatível com a declarada no Trait.
3. WHEN um `impl` omite ao menos um método declarado no Trait, THE Typechecker SHALL emitir o Diagnostic `SEED012` listando os métodos ausentes.
4. WHEN um método implementado tem assinatura incompatível com o Trait, THE Typechecker SHALL emitir o Diagnostic `SEED013` com a assinatura esperada e a encontrada.
5. THE Typechecker SHALL aceitar um tipo `T` em qualquer posição anotada com `dyn Display` se e somente se `impl Display for T` estiver no escopo.
6. THE IR_Generator SHALL representar despacho de Trait como acesso a uma vtable: `OP_GET_FIELD` sobre o registro de vtable seguido de `OP_CALL` indireto.
7. THE Typechecker SHALL rejeitar a implementação de dois `impl` do mesmo Trait para o mesmo tipo no mesmo escopo e emitir o Diagnostic `SEED014`.
8. FOR ALL tipos `T` com `impl SomeTrait for T`, a vtable gerada SHALL conter ponteiros para todas as funções declaradas no Trait, sem exceção (propriedade de completude da vtable).

---

### Requisito 5: Type Constraints (Where Clauses)

**User Story:** Como programador SEED, quero expressar restrições sobre parâmetros de tipo usando `where`, para que funções genéricas possam garantir que os tipos fornecidos implementam os Traits necessários.

#### Critérios de Aceitação

1. THE Typechecker SHALL aceitar a sintaxe `fn serializar[T](v: T) -> String where T: Display` e registrar a constraint `T: Display` no `GenericContext` da função.
2. WHEN a função `serializar[T]` é instanciada com um tipo concreto `A` que não implementa `Display`, THE Typechecker SHALL emitir o Diagnostic `SEED011` antes de aceitar a instanciação.
3. WHEN a função `serializar[T]` é instanciada com um tipo concreto `A` que implementa `Display`, THE Typechecker SHALL aceitar a instanciação sem erro.
4. THE Typechecker SHALL aceitar múltiplas constraints sobre o mesmo type parameter (`where T: Display + Serializable`) e verificar cada constraint independentemente.
5. THE Typechecker SHALL aceitar constraints transitivas entre parâmetros (`where K: Into[V]`) e verificar a compatibilidade em tempo de instanciação.
6. WHEN uma where clause referencia um Trait inexistente, THE Typechecker SHALL emitir o Diagnostic `SEED015` indicando o nome do Trait não definido.
7. FOR ALL funções genéricas com `where T: TraitA + TraitB` instanciadas com um tipo `C`, THE Typechecker SHALL verificar ambas `TraitA` e `TraitB` para `C`, e apenas emitir erro se pelo menos uma falhar.

---

### Requisito 6: Closures com Captura de Variáveis

**User Story:** Como programador SEED, quero definir closures que capturam variáveis do escopo léxico, para que funções de ordem superior como `map`, `filter` e `reduce` sejam expressáveis de forma idiomática.

#### Critérios de Aceitação

1. THE Typechecker SHALL aceitar expressões closure na forma `fn(x: Int) -> Int { x + n }` onde `n` é uma variável do escopo externo e inferir o tipo da closure como `(Int) -> Int`.
2. THE Typechecker SHALL identificar cada variável capturada (upvalue) e verificar que ela existe e tem tipo definido no escopo externo no momento da criação da closure.
3. WHEN uma variável capturada é mutável no escopo externo e a closure a modifica, THE Typechecker SHALL verificar que o modelo de memória escolhido (Requisito 7) permite essa mutação e emitir o Diagnostic `SEED016` se não permitir.
4. THE IR_Generator SHALL emitir a instrução `OP_CAPTURE` para cada upvalue identificado pelo Typechecker, seguida de `OP_MAKE_CLOSURE` que agrupa o ponteiro de função e o registro de captura.
5. THE Codegen SHALL gerar, para cada closure, uma função auxiliar no output com um parâmetro adicional de ponteiro para o registro de captura.
6. WHEN uma closure é passada como argumento para uma função que aceita `fn(A) -> B`, THE Typechecker SHALL verificar que a assinatura da closure é compatível com `fn(A) -> B`.
7. FOR ALL closures `c` que capturam variáveis `v1, v2, ..., vn`, o registro de captura gerado pelo IR_Generator SHALL conter referências a todos os `n` upvalues e somente a eles (invariante de completude do capture record).

---

### Requisito 7: Modelo de Memória Definido

**User Story:** Como desenvolvedor do compilador SEED, quero um modelo de memória explicitamente escolhido e documentado, para que o comportamento de alocação e liberação seja previsível e auditável em todos os programas SEED.

#### Critérios de Aceitação

1. THE Compiler SHALL adotar o modelo de memória **Ownership com liberação determinística** (sem GC stop-the-world) como padrão para a versão `0.4`.
2. THE Typechecker SHALL rastrear o escopo de propriedade de cada valor alocado e verificar que `FREE` é emitido no IR exatamente uma vez por alocação, na saída do escopo do dono.
3. WHEN uma função retorna um valor alocado, THE Typechecker SHALL transferir a propriedade ao chamador e não emitir `FREE` dentro da função que retornou o valor.
4. WHEN um valor é movido (atribuído a outra variável ou passado como argumento não-referência), THE Typechecker SHALL invalidar o identificador original e emitir o Diagnostic `SEED017` se o identificador original for usado após o movimento.
5. THE Typechecker SHALL aceitar o modificador `ref` em parâmetros de função para empréstimo imutável e o modificador `ref mut` para empréstimo mutável exclusivo.
6. WHEN dois empréstimos `ref mut` do mesmo valor existem simultaneamente no mesmo escopo, THE Typechecker SHALL emitir o Diagnostic `SEED018`.
7. THE IR_Generator SHALL emitir `OP_FREE` para cada `OP_ALLOC` cujo escopo de ownership termina no bloco corrente, mantendo o invariante: para todo `OP_ALLOC` com id `N`, existe exatamente um `OP_FREE` com referência a `N` em algum bloco do CFG alcançável.

---

### Requisito 8: Leak Detection e Memory Tracking

**User Story:** Como desenvolvedor SEED, quero um mecanismo de detecção de leaks de memória em modo debug, para que alocações não liberadas sejam reportadas com localização de código ao término da execução.

#### Critérios de Aceitação

1. WHEN o programa é compilado com a flag `--debug`, THE MemoryTracker SHALL ser injetado pelo Codegen como código de instrumentação em torno de cada `OP_ALLOC` e `OP_FREE`.
2. THE MemoryTracker SHALL manter uma tabela em tempo de execução que associa cada ponteiro alocado a: endereço, tamanho em bytes, arquivo, linha e coluna da alocação.
3. WHEN um `OP_FREE` é executado, THE MemoryTracker SHALL remover a entrada correspondente da tabela de alocações ativas.
4. WHEN o programa termina normalmente (retorno de `main`) e a tabela de alocações ativas não está vazia, THE MemoryTracker SHALL imprimir no stderr cada entrada remanescente com o formato `[LEAK] <bytes> bytes em <arquivo>:<linha>:<coluna>`.
5. THE MemoryTracker SHALL operar com overhead de tempo de execução inferior a 10% em relação à execução sem instrumentação para programas com até 100.000 alocações por segundo.
6. WHEN a flag `--debug` não está ativa, THE Codegen SHALL não injetar código de instrumentação do MemoryTracker, preservando o desempenho do build de produção.
7. FOR ALL programas com N alocações e N frees correspondentes executados sob o MemoryTracker, o número de entradas na tabela ao término SHALL ser zero (invariante de balanço).

---

### Requisito 9: Lockfile no Package Manager

**User Story:** Como desenvolvedor SEED, quero que o Package Manager gere e consuma um lockfile, para que instalações repetidas do mesmo projeto produzam exatamente as mesmas versões de dependências em qualquer máquina.

#### Critérios de Aceitação

1. WHEN `seed install` é executado pela primeira vez em um projeto com um `seedfile.seed` válido, THE PackageManager SHALL resolver todas as dependências e escrever um arquivo `seed.lock` no diretório raiz do projeto.
2. THE `seed.lock` SHALL conter, para cada pacote resolvido: nome, versão exata (SemVer), hash SHA-256 do conteúdo baixado e URL de origem.
3. WHEN `seed install` é executado em um projeto onde `seed.lock` já existe, THE PackageManager SHALL instalar exatamente as versões listadas no lockfile, sem tentar resolver novamente.
4. WHEN o `seedfile.seed` é modificado de forma que uma dependência listada no lockfile é removida ou sua constraint de versão se torna incompatível com a versão travada, THE PackageManager SHALL emitir um aviso `SEED_PKG001` e solicitar confirmação antes de atualizar o lockfile.
5. THE PackageManager SHALL verificar o hash SHA-256 de cada pacote baixado contra o valor registrado no lockfile e, WHEN os hashes divergem, rejeitar a instalação e emitir o Diagnostic `SEED_PKG002`.
6. FOR ALL projetos onde o `seed.lock` é commitado e dois desenvolvedores executam `seed install` independentemente, os diretórios de dependências resultantes SHALL conter arquivos com conteúdo bit-a-bit idêntico (propriedade de reprodutibilidade).

---

### Requisito 10: Integração de SemVer no Package Manager

**User Story:** Como desenvolvedor SEED, quero que o Package Manager entenda constraints de versionamento SemVer, para que eu possa expressar intervalos de compatibilidade e o resolvedor encontre a melhor versão disponível.

#### Critérios de Aceitação

1. THE PackageManager SHALL aceitar as seguintes formas de constraint de versão no `seedfile.seed`: versão exata (`1.2.3`), prefixo patch-compatível (`~1.2.3` → `>=1.2.3 <1.3.0`), prefixo minor-compatível (`^1.2.3` → `>=1.2.3 <2.0.0`) e intervalo explícito (`>=1.0.0 <2.0.0`).
2. WHEN múltiplas dependências requerem versões diferentes do mesmo pacote, THE PackageManager SHALL aplicar o algoritmo de resolução de versão mais recente compatível com todas as constraints e, IF nenhuma versão satisfizer todas as constraints, emitir o Diagnostic `SEED_PKG003` listando os conflitos.
3. THE PackageManager SHALL rejeitar qualquer string de versão que não seja SemVer válida e emitir o Diagnostic `SEED_PKG004` indicando o campo inválido.
4. WHEN uma dependência é marcada como `pre-release` (e.g., `1.0.0-alpha.1`), THE PackageManager SHALL instalá-la apenas se a constraint do `seedfile.seed` explicitamente incluir pré-releases (`>=1.0.0-alpha`).
5. FOR ALL constraints `C` e versões `V`, THE PackageManager SHALL retornar `matches(C, V) == true` se e somente se `V` satisfaz `C` segundo a especificação SemVer 2.0.0, e `matches(C, V) == false` caso contrário (propriedade de corretude do matcher).

---

### Requisito 11: Registry Funcional para Pacotes

**User Story:** Como desenvolvedor SEED, quero publicar e baixar pacotes de um registry centralizado, para que a comunidade possa compartilhar bibliotecas e ferramentas SEED de forma padronizada.

#### Critérios de Aceitação

1. THE Registry_Client SHALL suportar o comando `seed publish` que empacota o projeto em um arquivo `.seed-pkg`, calcula seu hash SHA-256 e o envia para o endpoint configurado em `seed.lock` ou no registry padrão.
2. THE Registry_Client SHALL suportar o comando `seed search <termo>` que consulta o registry e exibe nome, versão mais recente e descrição dos pacotes que correspondem ao termo.
3. WHEN `seed install` necessita baixar um pacote, THE Registry_Client SHALL realizar no máximo 3 tentativas em caso de falha de rede antes de emitir o Diagnostic `SEED_PKG005` e interromper a instalação.
4. THE Registry_Client SHALL autenticar requisições de publicação via token API armazenado em `~/.seed/credentials` e, WHEN o token estiver ausente ou expirado, emitir o Diagnostic `SEED_PKG006` com instrução de login.
5. THE Registry SHALL rejeitar a publicação de uma versão já existente para o mesmo pacote com o mesmo nome e emitir HTTP 409, e THE Registry_Client SHALL traduzir esse erro para o Diagnostic `SEED_PKG007`.
6. WHEN a resposta do registry para qualquer requisição contém um payload inválido (não-JSON ou schema incorreto), THE Registry_Client SHALL emitir o Diagnostic `SEED_PKG008` sem abortar outros comandos em andamento.
7. FOR ALL pacotes publicados com nome `P` e versão `V`, um `seed install` subsequente de `P@V` SHALL baixar e desempacotar um artefato com hash SHA-256 idêntico ao registrado na publicação (propriedade de integridade do registry).

---

### Requisito 12: FFI para WebAssembly (WASM)

**User Story:** Como desenvolvedor SEED, quero compilar módulos SEED para WebAssembly e importar funções WASM de módulos externos, para que código SEED possa executar em navegadores e runtimes WASM.

#### Critérios de Aceitação

1. WHEN `seed compile --target wasm <arquivo.seed>` é invocado, THE Codegen SHALL produzir um arquivo `.wasm` binário válido segundo a especificação WebAssembly 2.0.
2. THE Compiler SHALL aceitar declarações de importação WASM na forma `extern wasm "modulo" fn nome(a: Int) -> Int` e registrar `nome` no TypeEnv com a assinatura declarada.
3. THE Codegen SHALL emitir, para cada função declarada com `extern wasm`, uma entrada na seção `import` do módulo WASM gerado.
4. THE Codegen SHALL emitir, para cada função SEED marcada com `#[export]`, uma entrada na seção `export` do módulo WASM gerado com o nome original da função.
5. WHEN os tipos de parâmetros ou retorno de uma declaração `extern wasm` não são mapeáveis para tipos WASM primitivos (i32, i64, f32, f64, externref), THE Typechecker SHALL emitir o Diagnostic `SEED020` listando o tipo não mapeável e sua localização.
6. THE IR_Generator SHALL representar chamadas a funções `extern wasm` como instrução `OP_CALL` com flag `ffi_wasm = true`, permitindo que o Codegen gere a sequência de call indireto correta.
7. FOR ALL funções SEED `f` exportadas para WASM com assinatura `(A, B) -> C`, a chamada de `f` a partir de um host WASM com os tipos correspondentes SHALL produzir resultado equivalente ao produzido pela execução nativa (propriedade de equivalência FFI).

---

### Requisito 13: FFI para JavaScript

**User Story:** Como desenvolvedor SEED, quero chamar funções JavaScript a partir de código SEED e expor funções SEED ao JavaScript, para que SEED possa integrar-se ao ecossistema JS em ambientes Node.js e navegadores.

#### Critérios de Aceitação

1. THE Compiler SHALL aceitar declarações `extern js "globalThis.console.log" fn log(msg: String) -> Unit` e registrar `log` no TypeEnv com a assinatura declarada.
2. THE Codegen SHALL gerar, para o target `js`, um módulo JavaScript (ESM ou CJS configurável) que inclui wrappers para todas as funções SEED exportadas.
3. THE Codegen SHALL emitir, para cada chamada a uma função `extern js`, código de coerção de tipos entre os tipos SEED e os tipos JavaScript correspondentes (e.g., `String` ↔ JS string, `Int` ↔ JS number).
4. WHEN um tipo SEED não tem mapeamento JS definido, THE Typechecker SHALL emitir o Diagnostic `SEED021` indicando o tipo e sugerindo o uso de `extern js` com tipo `Any`.
5. THE Codegen SHALL garantir que erros lançados pelo lado JS (exceções JavaScript) sejam capturados e convertidos para `Result[T, Error]` no lado SEED, preservando a mensagem e o stack trace JS.
6. FOR ALL funções SEED `f` com assinatura `(String) -> String` expostas ao JS, chamar `f` a partir de JS com uma string SHALL retornar uma string e nunca lançar exceção não tratada (propriedade de robustez da fronteira FFI).

---

### Requisito 14: Stack Traces Abrangentes

**User Story:** Como desenvolvedor SEED, quero que erros em tempo de execução exibam um stack trace completo com arquivo, linha e nome de função, para que a causa raiz de um crash seja identificada rapidamente.

#### Critérios de Aceitação

1. WHEN um pânico ocorre em tempo de execução (acesso fora de bounds, unwrap de `None`, divisão por zero), THE Runtime SHALL imprimir no stderr um stack trace com no mínimo os 20 frames mais recentes.
2. Cada frame do stack trace SHALL conter: índice do frame, nome da função, caminho do arquivo fonte, número de linha e número de coluna.
3. THE Codegen SHALL emitir metadados de debug (mapa de linha/coluna por instrução IR) em uma seção separada do output quando a flag `--debug` está ativa, e THE Runtime SHALL usar esses metadados para construir o stack trace.
4. WHEN um pânico ocorre dentro de uma closure, THE Runtime SHALL incluir no stack trace o frame da closure com o nome derivado da variável que a recebeu ou `<closure>` quando anônima.
5. WHEN um pânico ocorre dentro de um wrapper FFI (WASM ou JS), THE Runtime SHALL incluir um frame de fronteira identificado como `<ffi:wasm>` ou `<ffi:js>`, separando o stack SEED do stack externo.
6. WHEN a flag `--debug` não está ativa, THE Runtime SHALL exibir apenas o primeiro frame e a mensagem de erro, sem metadados de localização detalhados, para não expor informações internas em builds de produção.
7. FOR ALL sequências de chamadas `A → B → C` onde `C` entra em pânico, o stack trace SHALL listar os frames na ordem `C`, `B`, `A` (ordem inversa de chamada, frame mais recente primeiro).

---

### Requisito 15: Sistema de Erros com Mensagens Detalhadas

**User Story:** Como programador SEED, quero que o compilador emita mensagens de erro detalhadas com contexto, sugestão de correção e código de erro estruturado, para que eu possa entender e corrigir problemas sem recorrer à documentação externa.

#### Critérios de Aceitação

1. THE ErrorReporter SHALL formatar cada Diagnostic com: código de erro (`SEEDXXX` ou `SEED_PKGXXX`), severidade (`error`, `warning`, `proposal`), localização (`arquivo:linha:coluna`), mensagem principal, trecho de código-fonte relevante sublinhado e campo `suggestion` não vazio.
2. THE ErrorReporter SHALL suportar os níveis de severidade `error` (impede compilação), `warning` (permite compilação com aviso) e `proposal` (sugestão opcional de melhoria), e THE Compiler SHALL compilar apenas se não houver Diagnostics de nível `error`.
3. WHEN o ErrorReporter emite um Diagnostic de tipo `error`, THE ErrorReporter SHALL também exibir o trecho de código-fonte correspondente com o token problemático sublinhado por `^` no terminal.
4. WHEN múltiplos Diagnostics se referem ao mesmo intervalo de código, THE ErrorReporter SHALL agrupá-los e exibi-los juntos, sem repetir o trecho de código-fonte.
5. THE ErrorReporter SHALL suportar o formato de saída `--error-format json` que emite cada Diagnostic como um objeto JSON na stdout, compatível com ferramentas de integração contínua.
6. WHEN um Diagnostic possui um campo `suggestion` de substituição pontual (inserção, remoção ou troca de tokens), THE ErrorReporter SHALL calcular e exibir um diff unificado entre o código original e o código corrigido.
7. THE ErrorReporter SHALL incluir em cada Diagnostic um campo `related` com lista de Diagnostics secundários que fornecem contexto adicional (e.g., localização da declaração original conflitante).
8. FOR ALL Diagnostics emitidos pelo Compiler para um dado arquivo fonte, a serialização JSON de cada Diagnostic e sua desserialização SHALL produzir um objeto com campos e valores idênticos aos do Diagnostic original (propriedade de round-trip do Diagnostic).
