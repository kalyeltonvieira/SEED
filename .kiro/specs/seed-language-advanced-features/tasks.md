# Plano de Implementação: SEED Advanced Features (v0.4)

## Visão Geral

Implementação incremental das features avançadas da linguagem SEED versão `0.4`, cobrindo: IR completo com SSA, sistema de tipos avançado (Union types, Generics, Traits, Type Constraints), Closures com captura de variáveis, modelo de memória por Ownership, Package Manager (lockfile/SemVer/Registry), FFI para WASM e JavaScript, Stack Traces abrangentes e sistema de erros com Diagnostics detalhados.

Todos os novos módulos são criados em `compiler/` conforme a estrutura definida no design. Os testes ficam em `compiler/tests/`.

---

## Tarefas

- [x] 1. Criar o ErrorReporter — módulo base de diagnósticos
  - Criar `compiler/error_reporter.seed` com os tipos `Diagnostic`, `Severity`, `Location` e funções `format_diagnostic`, `format_diagnostics_json`, `parse_diagnostic_json`, `group_by_location`, `compute_diff` e `underline_source` conforme o design
  - Implementar os três níveis de severidade: `error` (impede compilação), `warning` e `proposal`
  - Implementar o sublinhamento do trecho de código com `^` para Diagnostics de nível `error`
  - Implementar o agrupamento de Diagnostics do mesmo intervalo de código
  - Implementar a geração de diff unificado quando `suggestion` é uma substituição pontual
  - Implementar o campo `related` com lista de Diagnostics secundários
  - Implementar o flag `--error-format json` que emite cada Diagnostic como JSON na stdout
  - _Requisitos: 15.1, 15.2, 15.3, 15.4, 15.5, 15.6, 15.7_

  - [ ]* 1.1 Escrever property test P16 — Round-trip de Diagnostic (JSON)
    - **Property 16: Diagnostic JSON round-trip**
    - Gerar Diagnostics arbitrários com `gen_diagnostic()`, serializar com `format_diagnostics_json`, desserializar com `parse_diagnostic_json` e verificar igualdade de campos
    - Criar `compiler/tests/error_reporter_test.seed`
    - **Validates: Requirements 15.8**

  - [ ]* 1.2 Escrever property test P18 — Diagnostic completo (campos obrigatórios)
    - **Property 18: Diagnostic completeness**
    - Verificar que `format_diagnostic(d)` contém código, severidade, localização, mensagem, suggestion e `^` para qualquer Diagnostic gerado
    - **Validates: Requirements 15.1**

  - [ ]* 1.3 Escrever testes unitários do ErrorReporter
    - Testar: Diagnostic `error` → sublinhado com `^`; múltiplos Diagnostics no mesmo intervalo → agrupados; Diagnostic com suggestion pontual → diff exibido; campo `related` na saída
    - **Validates: Requirements 15.3, 15.4, 15.6, 15.7**

- [ ] 2. Estender o IR_Generator com opcodes e SSA
  - Adicionar os novos opcodes `OP_UNION_TAG` e `OP_UNION_ACCESS` em `compiler/ir.seed`
  - Estender `IRInstruction` com os campos `ffi_wasm: bool`, `ffi_js: bool` e `debug_name: string`
  - Estender `Type` com os campos `is_owned: bool` e `trait_bounds: [string]`
  - Implementar o `SSABuilder` com as funções `build_ssa`, `insert_phi_nodes`, `rename_values`, `compute_dominators` e `compute_dom_frontier`
  - Garantir que cada `IRValue` definido em um `IRBlock` é definido exatamente uma vez (propriedade SSA)
  - Inserir nós `OP_PHI` nos pontos de convergência do CFG quando há ciclos
  - Implementar `serialize_ir_module`, `deserialize_ir_module` e `ir_modules_equivalent` para serialização JSON round-trip
  - Emitir Diagnostic `SEED010` com linha e coluna para cada `ASTNode` sem lowering definido
  - Emitir representação textual legível do `IRModule` antes do Codegen quando `--debug` está ativo
  - _Requisitos: 1.1, 1.2, 1.3, 1.7, 1.8, 1.9, 1.10_

  - [ ]* 2.1 Escrever property test P1 — Round-trip do IRModule (serialização)
    - **Property 1: IRModule serialization round-trip**
    - Gerar `IRModule` válidos arbitrários com `gen_valid_ir_module()`, serializar e desserializar, verificar equivalência estrutural com `ir_modules_equivalent`
    - Criar `compiler/tests/ir_ssa_test.seed`
    - **Validates: Requirements 1.9**

  - [ ]* 2.2 Escrever property test P2 — SSA: definição única por IRValue
    - **Property 2: SSA single assignment**
    - Gerar funções AST válidas arbitrárias, processar com `lower_function` e verificar que nenhum `IRValue.data` aparece em `result` de mais de uma instrução
    - **Validates: Requirements 1.2**

  - [ ]* 2.3 Escrever property test P7 — Monomorphization: deduplicação de instâncias
    - **Property 7: Monomorphization deduplication**
    - Gerar cenários com dois call sites para a mesma função genérica com os mesmos type args e verificar que o IRModule contém exatamente uma função monomorfa com o nome mangled
    - **Validates: Requirements 3.6**

  - [ ]* 2.4 Escrever testes unitários do IR_Generator
    - Testar: Union type ASTNode → `OP_UNION_TAG + OP_UNION_ACCESS`; closure com 2 upvalues → `OP_CAPTURE × 2 + OP_MAKE_CLOSURE`; Trait com vtable → `IRConstant` com entries; `--debug` flag → representação textual emitida; ASTNode sem lowering → `SEED010` com linha/coluna
    - **Validates: Requirements 1.3, 1.5, 1.6, 1.8, 1.10**

- [ ] 3. Checkpoint — Verificar IR e ErrorReporter
  - Garantir que todos os testes de `ir_ssa_test.seed` e `error_reporter_test.seed` passam. Perguntar ao usuário se há dúvidas antes de continuar.

- [ ] 4. Estender o Typechecker — Union Types e Exhaustividade
  - Adicionar `check_match_exhaustiveness`, `missing_variants` e `detect_circular_union` em `compiler/typechecker.seed`
  - Emitir `SEED005` quando um `match` sobre `Union` omite variantes; aceitar quando todas estão cobertas
  - Emitir `SEED007` quando um tipo incompatível é atribuído a uma variável `Union`
  - Emitir `SEED008` para Union types circulares diretos (e.g., `type X = X | Int`)
  - Implementar unificação de `Union[A, B]` com `T` se e somente se `T ∈ {A, B, Any}`
  - _Requisitos: 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7_

  - [ ]* 4.1 Escrever property test P3 — Union type: aceitação/rejeição
    - **Property 3: Union type assignment correctness**
    - Gerar cenários de atribuição com `gen_union_assignment_scenario()` e verificar que o Typechecker aceita `T ∈ {A, B, Any}` e rejeita outros
    - Criar `compiler/tests/typechecker_union_test.seed`
    - **Validates: Requirements 2.4, 2.5, 2.6**

  - [ ]* 4.2 Escrever property test P4 — Exhaustividade de match sobre Union
    - **Property 4: Match exhaustiveness**
    - Gerar cenários de match com cobertura parcial e total e verificar que `SEED005` é emitido se e somente se existem variantes não cobertas
    - **Validates: Requirements 2.2, 2.3**

  - [ ]* 4.3 Escrever property test P5 — Metamórfica de Union: inferência equivalente
    - **Property 5: Metamorphic Union inference**
    - Verificar que inferir o tipo de `v` diretamente produz o mesmo tipo que inferir `v` como `U` e acessar o braço correspondente no match
    - **Validates: Requirements 2.8**

  - [ ]* 4.4 Escrever testes unitários de Union Types
    - Testar: Union circular `type X = X | Int` → `SEED008`; atribuição compatível → aceita; atribuição incompatível → `SEED007`
    - **Validates: Requirements 2.7, 2.5, 2.6**

- [ ] 5. Estender o Typechecker — Generics e Monomorphization
  - Implementar `GenericContext` estendido com campo `instantiations` em `compiler/typechecker.seed`
  - Implementar `unify_hm`, `apply_substitution`, `generalize` e `instantiate` para Hindley-Milner completo
  - Implementar `MonomorphizationCache` com `monomorphize`, `mangle_name` e `substitute_type_vars` em `compiler/ir.seed`
  - Aceitar declarações `fn nome[T, K](a: T, b: K) -> T` e registrar TypeVars no `GenericContext`
  - Aceitar declarações `data Container[T] { Value(T) }` com TypeVar associado
  - Inferir type arguments por unificação em chamadas de funções genéricas e produzir instância monomorfa
  - Emitir `SEED011` quando type arguments violam Type Constraints
  - Reutilizar a mesma função monomorfa para dois call sites com os mesmos type args (sem duplicação no IRModule)
  - _Requisitos: 3.1, 3.2, 3.3, 3.4, 3.5, 3.6_

  - [ ]* 5.1 Escrever property test P6 — Identidade de tipo em funções genéricas
    - **Property 6: Generic type identity**
    - Para qualquer tipo concreto `A`, a instância `f[A]` deve ser aceita e `f[A](v)` deve ter tipo `A`
    - Criar `compiler/tests/typechecker_generic_test.seed`
    - **Validates: Requirements 3.3, 3.7**

  - [ ]* 5.2 Escrever testes unitários de Generics
    - Testar: `fn identidade[T](x: T) -> T` aceita e infere corretamente; dois call sites com mesmos type args → uma função monomorfa no IRModule; type arg que viola constraint → `SEED011`
    - **Validates: Requirements 3.1, 3.3, 3.4, 3.6**

- [ ] 6. Estender o Typechecker — Traits e Type Constraints
  - Implementar `TraitRegistry`, `TraitDef`, `ImplDef` e as funções `resolve_trait`, `check_impl_completeness`, `build_vtable` e `check_where_clauses` em `compiler/typechecker.seed`
  - Aceitar declaração `trait Display { fn format(self) -> String }` e registrar no `TraitRegistry`
  - Aceitar `impl Display for MeuTipo { ... }` e verificar completude e compatibilidade de assinaturas
  - Emitir `SEED012` quando `impl` omite métodos; emitir `SEED013` quando assinatura é incompatível
  - Aceitar tipo `T` em posição `dyn Display` se e somente se `impl Display for T` está no escopo
  - Rejeitar dois `impl` do mesmo Trait para o mesmo tipo no mesmo escopo → emitir `SEED014`
  - Implementar despacho de Trait no IR como `OP_GET_FIELD` sobre vtable + `OP_CALL` indireto
  - Aceitar `fn serializar[T](v: T) -> String where T: Display` e registrar constraints no `GenericContext`
  - Emitir `SEED011` quando tipo concreto não implementa Trait exigido pela where clause
  - Aceitar múltiplas constraints (`where T: Display + Serializable`) e verificar cada uma independentemente
  - Aceitar constraints transitivas (`where K: Into[V]`) e verificar em tempo de instanciação
  - Emitir `SEED015` quando where clause referencia Trait inexistente
  - _Requisitos: 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 5.1, 5.2, 5.3, 5.4, 5.5, 5.6_

  - [ ]* 6.1 Escrever property test P8 — Completude da vtable de Trait
    - **Property 8: Vtable completeness**
    - Gerar Traits com N métodos e impls completas, verificar que a vtable gerada contém exatamente N ponteiros
    - Criar `compiler/tests/typechecker_trait_test.seed`
    - **Validates: Requirements 4.8**

  - [ ]* 6.2 Escrever property test P10 — WHERE com múltiplos traits
    - **Property 10: WHERE multi-trait constraint**
    - Para qualquer combinação de `TraitA + TraitB` e tipo `C`, verificar que erro é emitido se e somente se `C` não implementa pelo menos um dos traits
    - **Validates: Requirements 5.7**

  - [ ]* 6.3 Escrever testes unitários de Traits e Type Constraints
    - Testar: `impl` omite método → `SEED012`; assinatura incompatível → `SEED013`; dois impls → `SEED014`; where clause com trait inexistente → `SEED015`; `dyn Display` com/sem impl → aceito/rejeitado
    - **Validates: Requirements 4.3, 4.4, 4.7, 5.6, 4.5**

- [ ] 7. Checkpoint — Verificar Typechecker completo
  - Garantir que todos os testes de `typechecker_union_test.seed`, `typechecker_generic_test.seed` e `typechecker_trait_test.seed` passam. Perguntar ao usuário se há dúvidas antes de continuar.

- [ ] 8. Implementar Closures com captura de variáveis
  - Implementar `ClosureAnalysis`, `analyze_closure`, `lift_closure` e `emit_capture_record` no `compiler/ir.seed`
  - O Typechecker deve aceitar `fn(x: Int) -> Int { x + n }` e inferir tipo `(Int) -> Int` quando `n` é upvalue
  - Verificar que cada upvalue existe com tipo definido no escopo externo no momento da criação da closure
  - Emitir `SEED016` quando closure modifica upvalue mutável de forma proibida pelo modelo de memória
  - Emitir `OP_CAPTURE` para cada upvalue identificado seguido de `OP_MAKE_CLOSURE`
  - O Codegen deve gerar função auxiliar com parâmetro adicional de ponteiro para o registro de captura
  - Verificar compatibilidade de assinatura da closure com o tipo `fn(A) -> B` do parâmetro receptor
  - _Requisitos: 6.1, 6.2, 6.3, 6.4, 6.5, 6.6_

  - [ ]* 8.1 Escrever property test P9 — Completude do capture record de Closure
    - **Property 9: Closure capture record completeness**
    - Gerar closures com N upvalues identificados e verificar que o capture record emitido pelo IR contém exatamente N referências
    - Criar `compiler/tests/typechecker_closure_test.seed`
    - **Validates: Requirements 6.7**

  - [ ]* 8.2 Escrever testes unitários de Closures
    - Testar: closure captura 2 variáveis → `OP_CAPTURE × 2 + OP_MAKE_CLOSURE`; upvalue inexistente no escopo → erro; closure compatível com `fn(A) -> B` → aceita; closure incompatível → rejeitada
    - **Validates: Requirements 6.1, 6.2, 6.6**

- [ ] 9. Implementar o Ownership Checker
  - Criar `compiler/ownership.seed` com `OwnershipEnv`, `OwnershipState`, `BorrowInfo` e as funções `check_ownership`, `transfer_ownership`, `check_borrow_conflicts` e `emit_free_on_scope_exit`
  - O Typechecker deve rastrear escopo de propriedade e verificar que `FREE` é emitido exatamente uma vez por alocação na saída do escopo do dono
  - Transferir propriedade ao chamador quando função retorna valor alocado (sem `FREE` no corpo)
  - Invalidar identificador original após movimento e emitir `SEED017` se usado após o movimento
  - Aceitar `ref` para empréstimo imutável e `ref mut` para empréstimo mutável exclusivo
  - Emitir `SEED018` quando dois empréstimos `ref mut` do mesmo valor coexistem no mesmo escopo
  - Emitir `OP_FREE` para cada `OP_ALLOC` cujo escopo termina no bloco corrente (invariante ALLOC/FREE)
  - _Requisitos: 7.1, 7.2, 7.3, 7.4, 7.5, 7.6, 7.7_

  - [ ]* 9.1 Escrever property test P11 — Invariante de balanço ALLOC/FREE no IR
    - **Property 11: ALLOC/FREE balance in IR**
    - Gerar programas AST arbitrários, processar com `generate_ir` e verificar que para todo `OP_ALLOC` com id `N` existe exatamente um `OP_FREE` referenciando `N` no CFG
    - Criar `compiler/tests/ownership_test.seed`
    - **Validates: Requirements 7.2, 7.7**

  - [ ]* 9.2 Escrever testes unitários do Ownership Checker
    - Testar: `let x = v; let y = v;` → `SEED017`; dois `ref mut` simultâneos → `SEED018`; função retorna valor alocado → sem FREE no IR do corpo; `ref` aceito sem erro
    - **Validates: Requirements 7.4, 7.6, 7.3, 7.5**

- [ ] 10. Implementar o MemoryTracker
  - Criar `compiler/memory_tracker.seed` com `AllocationEntry`, `MemoryTrackerState` e as funções `__mt_alloc`, `__mt_free` e `__mt_report`
  - O Codegen deve injetar chamadas a `__mt_alloc` e `__mt_free` em torno de cada `OP_ALLOC` e `OP_FREE` quando `--debug` está ativo
  - A tabela de alocações ativas deve associar cada ponteiro a: endereço, tamanho, arquivo, linha e coluna
  - Ao término normal do programa, imprimir no stderr entradas remanescentes no formato `[LEAK] <bytes> bytes em <arquivo>:<linha>:<coluna>`
  - Quando `--debug` não está ativo, o Codegen não deve injetar código de instrumentação
  - _Requisitos: 8.1, 8.2, 8.3, 8.4, 8.6_

  - [ ]* 10.1 Escrever property test P12 — Invariante de balanço do MemoryTracker
    - **Property 12: MemoryTracker balance**
    - Gerar sequências balanceadas de alloc/free, executar sobre `MemoryTrackerState` e verificar que `active_allocations.len == 0` ao término
    - Criar `compiler/tests/memory_tracker_test.seed`
    - **Validates: Requirements 8.7**

  - [ ]* 10.2 Escrever testes unitários do MemoryTracker
    - Testar: compilação com `--debug` → código de instrumentação presente no output; compilação sem `--debug` → sem instrumentação; leak reportado corretamente no formato esperado
    - **Validates: Requirements 8.1, 8.6, 8.4**

- [ ] 11. Checkpoint — Verificar Ownership e MemoryTracker
  - Garantir que todos os testes de `ownership_test.seed` e `memory_tracker_test.seed` passam. Perguntar ao usuário se há dúvidas antes de continuar.

- [ ] 12. Implementar o PackageManager — SemVer e Lockfile
  - Criar `compiler/package_manager.seed` com os tipos `SeedFile`, `VersionConstraint`, `SemVer`, `LockEntry`, `Lockfile` e as funções `parse_semver`, `semver_matches`, `resolve_dependencies`, `write_lockfile`, `read_lockfile` e `verify_sha256`
  - Aceitar formas de constraint: versão exata (`1.2.3`), tilde (`~1.2.3`), caret (`^1.2.3`) e intervalo explícito (`>=1.0.0 <2.0.0`)
  - Emitir `SEED_PKG004` para strings de versão não-SemVer
  - Resolver dependências e escrever `seed.lock` com nome, versão exata, hash SHA-256 e URL de origem
  - Quando `seed.lock` existe, instalar exatamente as versões listadas sem re-resolver
  - Verificar hash SHA-256 de cada pacote baixado; rejeitar e emitir `SEED_PKG002` quando hashes divergem
  - Emitir `SEED_PKG001` e pedir confirmação quando seedfile.seed fica incompatível com o lockfile
  - Emitir `SEED_PKG003` quando não há versão que satisfaz todas as constraints
  - Instalar pré-releases apenas quando a constraint inclui explicitamente pré-releases
  - _Requisitos: 9.1, 9.2, 9.3, 9.4, 9.5, 10.1, 10.2, 10.3, 10.4_

  - [ ]* 12.1 Escrever property test P14 — Corretude do matcher SemVer
    - **Property 14: SemVer matcher correctness**
    - Gerar pares (constraint, version) arbitrários e verificar que `semver_matches` coincide com uma implementação de referência da especificação SemVer 2.0.0
    - Criar `compiler/tests/package_manager_test.seed`
    - **Validates: Requirements 10.5**

  - [ ]* 12.2 Escrever property test P13 — Reprodutibilidade do lockfile
    - **Property 13: Lockfile reproducibility**
    - Para qualquer lockfile fixo, `simulate_install(lockfile)` chamado duas vezes deve produzir resultados idênticos
    - **Validates: Requirements 9.6**

  - [ ]* 12.3 Escrever testes unitários do PackageManager
    - Testar: `seed install` com lockfile existente instala versões exatas; seedfile modificado incompativelmente → `SEED_PKG001`; pré-release sem constraint explícita → não instalada; versão não-SemVer → `SEED_PKG004`; hashes divergentes → `SEED_PKG002`
    - **Validates: Requirements 9.3, 9.4, 9.5, 10.3, 10.4**

- [ ] 13. Implementar o Registry Client
  - Criar `compiler/registry_client.seed` com `RegistryClient`, `RegistryError`, `PackageInfo` e as funções `publish`, `search` e `download`
  - Implementar máximo de 3 tentativas em falha de rede; emitir `SEED_PKG005` após esgotamento
  - Autenticar publicações via token em `~/.seed/credentials`; emitir `SEED_PKG006` quando token ausente/expirado
  - Traduzir HTTP 409 do registry para `SEED_PKG007`
  - Emitir `SEED_PKG008` para payload não-JSON ou schema incorreto, sem abortar outros comandos
  - _Requisitos: 11.1, 11.2, 11.3, 11.4, 11.5, 11.6_

  - [ ]* 13.1 Escrever testes unitários do Registry Client
    - Testar: `seed publish` com token ausente → `SEED_PKG006`; publicar versão duplicada → `SEED_PKG007`; payload inválido → `SEED_PKG008`; 3 falhas de rede → `SEED_PKG005`; `seed search` retorna lista formatada
    - **Validates: Requirements 11.4, 11.5, 11.6, 11.3, 11.2**

- [ ] 14. Checkpoint — Verificar PackageManager e Registry Client
  - Garantir que todos os testes de `package_manager_test.seed` passam e que os cenários do Registry Client funcionam corretamente. Perguntar ao usuário se há dúvidas antes de continuar.

- [ ] 15. Implementar o módulo FFI e Codegen WASM
  - Criar `compiler/ffi.seed` com `FFIDecl`, `WASM_TYPE_MAP`, `JS_TYPE_MAP` e as funções `check_ffi_types`, `emit_wasm_import`, `emit_js_wrapper` e `emit_js_coercion`
  - Criar `compiler/wasm_codegen.seed` que produz arquivo `.wasm` binário válido (WebAssembly 2.0) quando invocado com `--target wasm`
  - Aceitar `extern wasm "modulo" fn nome(a: Int) -> Int` e registrar no TypeEnv
  - Emitir entrada na seção `import` do módulo WASM para cada função `extern wasm`
  - Emitir entrada na seção `export` do módulo WASM para cada função SEED marcada com `#[export]`
  - Emitir `SEED020` para tipos não-mapeáveis para WASM (e.g., String, closures, structs complexos)
  - Representar chamadas `extern wasm` como `OP_CALL` com flag `ffi_wasm = true`
  - _Requisitos: 12.1, 12.2, 12.3, 12.4, 12.5, 12.6_

  - [ ]* 15.1 Escrever testes unitários de FFI WASM
    - Testar: tipo `String` em posição `extern wasm` → `SEED020`; compilação com `--target wasm` → arquivo `.wasm` produzido; `OP_CALL` com `ffi_wasm = true` gerado para chamadas `extern wasm`
    - Criar `compiler/tests/ffi_wasm_test.seed`
    - **Validates: Requirements 12.5, 12.1, 12.6**

- [ ] 16. Implementar o Codegen JavaScript
  - Criar `compiler/js_codegen.seed` que gera módulo JavaScript (ESM ou CJS configurável) quando invocado com `--target js`
  - Aceitar `extern js "globalThis.console.log" fn log(msg: String) -> Unit` e registrar no TypeEnv
  - Gerar wrappers para funções SEED exportadas e código de coerção de tipos (String ↔ JS string, Int ↔ JS number, Bool ↔ JS boolean, Unit ↔ undefined)
  - Emitir `SEED021` para tipos sem mapeamento JS definido
  - Capturar exceções JavaScript e convertê-las para `Result[T, Error]` preservando mensagem e stack trace JS
  - _Requisitos: 13.1, 13.2, 13.3, 13.4, 13.5_

  - [ ]* 16.1 Escrever testes unitários de FFI JS
    - Testar: struct complexo sem mapeamento JS → `SEED021`; função SEED exportada → wrapper JS gerado; exceção JS capturada → `Result.Err` com mensagem preservada; coerção `String ↔ JS string` correta
    - Criar `compiler/tests/ffi_js_test.seed`
    - **Validates: Requirements 13.4, 13.2, 13.5, 13.3**

- [ ] 17. Checkpoint — Verificar FFI WASM e JS
  - Garantir que todos os testes de `ffi_wasm_test.seed` e `ffi_js_test.seed` passam. Perguntar ao usuário se há dúvidas antes de continuar.

- [ ] 18. Estender o Runtime C++23 com Stack Traces
  - Estender `runtime/seed_runtime.h` com `SeedFrame`, `SeedCallStack` e as funções `seed_push_frame`, `seed_pop_frame`, `seed_panic` e `seed_print_stack_trace`
  - Estender `runtime/seed_runtime.cpp` para imprimir no stderr stack trace com mínimo 20 frames em caso de pânico (acesso fora de bounds, unwrap de `None`, divisão por zero)
  - Cada frame deve conter: índice, nome de função, caminho de arquivo, linha e coluna
  - O Codegen deve emitir `DebugMap` (`DebugEntry` por instrução IR) em seção separada quando `--debug` está ativo
  - Incluir frame `<closure>` ou nome derivado quando pânico ocorre dentro de closure
  - Incluir frame `<ffi:wasm>` ou `<ffi:js>` como fronteira quando pânico ocorre dentro de wrapper FFI
  - Quando `--debug` não está ativo, exibir apenas o primeiro frame e a mensagem de erro
  - Implementar `emit_debug_section` e `load_debug_section` em `compiler/stack_trace.seed`
  - _Requisitos: 14.1, 14.2, 14.3, 14.4, 14.5, 14.6_

  - [ ]* 18.1 Escrever property test P15 — Ordenação do stack trace (ordem inversa de chamada)
    - **Property 15: Stack trace inverse ordering**
    - Gerar cadeias de chamadas `A → B → ... → Z` arbitrárias onde `Z` entra em pânico; verificar que o trace lista frames na ordem `Z, ..., B, A` (mais recente primeiro)
    - Criar `compiler/tests/stack_trace_test.seed`
    - **Validates: Requirements 14.7**

  - [ ]* 18.2 Escrever property test P17 — Número correto de frames no stack trace
    - **Property 17: Stack trace frame count**
    - Para qualquer profundidade `d`, verificar que o trace contém `min(d, 20)` frames
    - **Validates: Requirements 14.1**

  - [ ]* 18.3 Escrever testes unitários de Stack Trace
    - Testar: pânico em closure → frame `<closure>` ou nome derivado; pânico em wrapper FFI → frame `<ffi:wasm>` ou `<ffi:js>`; build sem `--debug` → apenas primeiro frame + mensagem; metadados de debug emitidos quando `--debug` ativo
    - **Validates: Requirements 14.4, 14.5, 14.6, 14.3**

- [ ] 19. Integração final — Conectar todos os módulos ao pipeline
  - Atualizar `compiler/compiler.seed` para importar e usar `error_reporter`, `ownership`, `memory_tracker`, `package_manager`, `registry_client`, `ffi`, `wasm_codegen`, `js_codegen` e `stack_trace`
  - Integrar o `ErrorReporter` ao pipeline: Typechecker, IR_Generator e Ownership Checker roteiam erros via `ErrorReporter`
  - Integrar `--error-format json` ao fluxo de compilação: `format_diagnostics_json` na stdout quando ativo
  - Integrar `--target wasm` e `--target js` ao `compile_file` e `compile_project`
  - Integrar o `MemoryTracker` ao Codegen: injetar instrumentação quando `--debug` está ativo
  - Garantir que a compilação falha (exit code 1) somente quando há Diagnostics de nível `error`
  - _Requisitos: 1.1, 15.2, 15.5_

  - [ ]* 19.1 Escrever testes de integração do pipeline completo
    - Testar com arquivos `.seed` reais: compilação com Union types → `SEED005` correto; compilação com Generics → instâncias monorfas no IRModule; compilação com `--target wasm` → arquivo `.wasm` produzido; compilação com `--debug` → stack trace e MemoryTracker ativos
    - **Validates: Requirements 1.1, 3.5, 12.1, 8.1**

- [ ] 20. Checkpoint final — Todos os testes passando
  - Garantir que todos os arquivos de teste em `compiler/tests/` passam sem erros. Verificar que os 18 property tests e os testes unitários de todos os módulos estão verdes. Perguntar ao usuário se há dúvidas antes de encerrar.

---

## Notas

- Tarefas marcadas com `*` são opcionais e podem ser puladas para um MVP mais rápido
- Cada tarefa referencia requisitos específicos para rastreabilidade
- O ErrorReporter (tarefa 1) é implementado primeiro por ser dependência de todos os outros módulos
- Os property tests usam a biblioteca PBT nativa de `stdlib/test.seed` com geradores (`gen_*`) e `property(name, gen, prop, iterations: N)`
- Cada property test deve rodar mínimo 100 iterações (200 para os mais críticos, 500 para SemVer)
- Os módulos `wasm_codegen.seed` e `js_codegen.seed` dependem de `ffi.seed` — implementar `ffi.seed` primeiro (tarefa 15)
- O Ownership Checker (tarefa 9) deve ser integrado ao Typechecker como fase adicional, não substituição
- O campo `is_owned` no tipo `Type` é utilizado pelo Ownership Checker para rastrear ownership no sistema de tipos
