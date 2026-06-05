#include "seed_lib.hpp"

struct CompileReport;
struct Pass;
struct Optimization;

struct CompileReport {
std::string generation;
Array[seed0. diagnostics;
] Diagnostic;
Array[Optimization] optimizations;
int bytecode_size;
int accepted_rewrites;
int refused_rewrites;

};

struct Pass {
std::string name;
std::string phase;
bool enabled;
std::string explain;

};

struct Optimization {
std::string name;
double confidence;
std::string explain;
std::string before_cost;
std::string after_cost;
bool requires_consent;

};

auto pass_table() -> Array[Pass];
auto constant_folding({ program( returnrule programnumber {) (( +) bnumber =+ (; b( (* ab number) -> void;
auto algebraic_simplify({ program( returnrule programx {) +x =0 ;) +x =x ;) -x =x ;) *x =1 ;) *x =x ;) *0 =0 ;) *0 =x ;) &&x =x ;) ||x =) ;; ;( fn{ program( returnblock programblock === .; falsefn }program inline_cache) -> void;
auto complexity_guard({ program= letif programdetects_bubble_sort seed0. (const const && source. !p has_keep) .. {push rewrites. ("bubble-sort-to-timsort" Rewrite, "stdlib.collections.sort_timsort(lista)" "nested swap loops", seed0 "Bubble sort O(n^2) perde para timsort O(n log n) em entradas gerais."( ."O(1)" "O(n^2)", , 0.95Cost seed0, (0.92 "O(n)") -> void;

auto pass_table() -> Array[Pass] {
return [ Pass{std::string("constant-folding") , std::string("ast") , true , std::string("Avalia expressoes constantes antes de emitir bytecode.") } , Pass{std::string("algebraic-simplify") , std::string("ast") , true , std::string("Remove identidades como x+0, x*1 e x*0.") } , Pass{std::string("complexity-guard") , std::string("semantic") , true , std::string("Detecta algoritmos com custo assintotico ruim.") } , Pass{std::string("effect-audit") , std::string("semantic") , true , std::string("Impede que otimizacao aumente capacidades sem permissao.") } , Pass{std::string("freshness-check") , std::string("semantic") , true , std::string("Marca funcoes vencidas por crescimento de dados ou data.") } , Pass{std::string("exhaustiveness-check") , std::string("semantic") , true , std::string("Exige match completo para ADTs fechados como Result e Option.") } , Pass{std::string("result-flow-check") , std::string("semantic") , true , std::string("Detecta Result/Option ignorado e propoe match ou propagacao com ?.") } , Pass{std::string("inline-cache") , std::string("bytecode") , true , std::string("Memoriza destinos de chamadas frequentes.") } , Pass{std::string("peephole") , std::string("bytecode") , true , std::string("Troca sequencias curtas por equivalentes menores.") } , Pass{std::string("loop-unroll-small") , std::string("bytecode") , true , std::string("Desenrola loops pequenos com limite conhecido.") } ; ] ;
}

auto constant_folding({ program( returnrule programnumber {) (( +) bnumber =+ (; b( (* ab number) -> void {
= > number ( a * b ) ;
( number ( a ) - number ( b ) ) = > number ( a - b ) ;
( number ( a ) / number ( b ) ) when b != 0 = > number ( a / b ) ;
}

auto algebraic_simplify({ program( returnrule programx {) +x =0 ;) +x =x ;) -x =x ;) *x =1 ;) *x =x ;) *0 =0 ;) *0 =x ;) &&x =x ;) ||x =) ;; ;( fn{ program( returnblock programblock === .; falsefn }program inline_cache) -> void {
return annotate_calls ( program , call = > { ((call . count_last_runs >= 3 ) ? (call . cache_target = true call . reason = std::string("Chamada estavel detectada em 3+ execucoes.") ; ) : ()) return call } ) ;
}

auto complexity_guard({ program= letif programdetects_bubble_sort seed0. (const const && source. !p has_keep) .. {push rewrites. ("bubble-sort-to-timsort" Rewrite, "stdlib.collections.sort_timsort(lista)" "nested swap loops", seed0 "Bubble sort O(n^2) perde para timsort O(n log n) em entradas gerais."( ."O(1)" "O(n^2)", , 0.95Cost seed0, (0.92 "O(n)") -> void {
false ) ) ; } ; ((seed0 . detects_naive_fib ( p . source ) ) ? (p . rewrites . push_back ( seed0 . Rewrite ( std::string("fib-naive-to-matrix") , std::string("fib(n-1)+fib(n-2)") , std::string("fib_matrix(n)") , std::string("Recursao ingenua explode exponencialmente.") , seed0 . Cost ( std::string("O(2^n)") , std::string("O(n)") , 0.98 ) , seed0 . Cost ( std::string("O(log n)") , std::string("O(1)") , 0.94 ) , false ) ) ; ) : ()) ; return p ; } ; freshness_check ( program ) { let p = program ; ((seed0 . detects_unbounded_select ( p . source ) ) ? (p . rewrites . push_back ( seed0 . Rewrite ( std::string("select-star-to-paged-cache") , std::string("SELECT *") , std::string("cache.or_else(sql_query_paginated(..., page_size=1000))") , std::string("Consulta sem limite envelhece conforme a tabela cresce.") , seed0 . Cost ( std::string("O(table)") , std::string("O(table)") , 0.88 ) , seed0 . Cost ( std::string("O(page)") , std::string("O(page)") , 0.84 ) , false ) ) ; ) : ()) ; return p ; } ; exhaustiveness_check ( program ) { let p = program ; ((seed0 . detects_non_exhaustive_match ( p . source ) ) ? (p . rewrites . push_back ( seed0 . Rewrite ( std::string("complete-match-result") , std::string("match with missing variant") , std::string("match with Ok/Err or Some/None branches") , std::string("ADTs fechados permitem provar que todos os casos foram tratados.") , seed0 . Cost ( std::string("branch may panic") , std::string("unknown") , 0.91 ) , seed0 . Cost ( std::string("total branch") , std::string("same") , 0.91 ) , false ) ) ; ) : ()) ; return p ; } ; result_flow_check ( program ) { let p = program ; ((seed0 . detects_unchecked_result ( p . source ) ) ? (p . rewrites . push_back ( seed0 . Rewrite ( std::string("unchecked-result-to-question") , std::string("fallible call without handling") , std::string("fallible_call()?") , std::string("Falhas devem ser propagadas ou tratadas no ponto de uso.") , seed0 . Cost ( std::string("hidden failure") , std::string("unknown") , 0.88 ) , seed0 . Cost ( std::string("explicit failure") , std::string("same") , 0.88 ) , false ) ) ; ) : ()) ; return p ; } ; effect_audit ( before , after ) -> bool { let before_effects = before . effects ; let after_effects = after . effects ; for e in after_effects { ((! before_effects . find ( e ) && e != seed0 . Effect . none ) ? (return false ) : ()) ; } ; return true } ; peephole ( bytecode ) { return bytecode . replace ( [ [ cmp r0 r0 , jmp if_zero X ] = > [ jmp always X ] , [ alloc 0 ] = > {} , [ write rA rB 0 ] = > {} , [ read rA X , write X rA ] = > {} ; ] ) ; } ; negotiate ( source ) -> Array [ Optimization ] { let opts = Array [ Optimization ] ( ) ; ((source . find ( std::string("hash_map") ) && source . estimated_items <= 8 ) ? (opts . push_back ( Optimization{std::string("array-linear-over-hashmap") , 0.82 , std::string("Hash map tem overhead alto para poucos elementos; array linear tende a ser mais rapido.") , std::string("O(1) medio com alocacoes e hashing") , std::string("O(n) com n<=8 e memoria contigua") , true } ) ; ) : ()) ; ((source . find ( std::string("while") ) && source . loop_bounds_known ) ? (opts . push_back ( Optimization{std::string("while-to-for") , 0.77 , std::string("Loop com limite conhecido fica mais claro e abre unrolling.") , std::string("loop aberto") , std::string("loop limitado") , true } ) ; ) : ()) ; ((source . find ( std::string("#[expires") ) || source . find ( std::string("SELECT *") ) ) ? (opts . push_back ( Optimization{std::string("freshness-contract") , 0.90 , std::string("Funcao tem risco de envelhecimento; associar validade a tamanho de dados e testes.") , std::string("sem validade") , std::string("validade auditavel") , false } ) ; ) : ()) ; ((source . find ( std::string("genitor ") ) ) ? (opts . push_back ( Optimization{std::string("evolutionary-search") , 0.71 , std::string("Genitor detectado; gerar populacao, cruzar candidatos e selecionar por propriedade.") , std::string("implementacao unica") , std::string("populacao testada") , true } ) ; ) : ()) ; ((source . find ( std::string("match ") ) ) ? (opts . push_back ( Optimization{std::string("match-exhaustiveness") , 0.91 , std::string("match detectado; verificar cobertura de variantes antes de aceitar a geracao.") , std::string("ramificacao parcial") , std::string("ramificacao total") , false } ) ; ) : ()) ; ((source . find ( std::string("?") ) ) ? (opts . push_back ( Optimization{std::string("result-propagation") , 0.87 , std::string("Operador ? detectado; baixar para match que propaga Err/None.") , std::string("tratamento manual repetido") , std::string("propagacao explicita") , false } ) ; ) : ()) ; return opts ; } ; approve_rewrites ( program , opts : Array [ Optimization ] ) { let accepted = 0 ; let refused = 0 ; for r in program . rewrites { let gain = estimate_gain ( r . cost_before , r . cost_after ) ; ((gain > MIN_AUTONOMOUS_GAIN && ! r . after . find ( std::string("net.") ) && ! r . after . find ( std::string("unsafe") ) ) ? (r . accepted = true accepted = accepted + 1 ; ) : (r . accepted = io . ask_yes_no ( std::string("SEED propoe ") + r . name + std::string(": ") + r . reason + std::string(" Aceita?") ) ; ((r . accepted ) ? (accepted = accepted + 1 ) : (refused = refused + 1 )) ; )) ; } ; return { accepted : accepted , refused : refused } ; } ; compile ( source : std::string ) { let p = seed0 . compile ( source ) ; let guarded = complexity_guard ( p ) ; let fresh = freshness_check ( guarded ) ; let exhaustive = exhaustiveness_check ( fresh ) ; let result_checked = result_flow_check ( exhaustive ) ; let folded = constant_folding ( result_checked ) ; let simplified = algebraic_simplify ( folded ) ; let cached = inline_cache ( simplified ) ; let clean = dead_code_elimination ( cached ) ; ((effect_audit ( p , clean ) == false ) ? (clean . diagnostics . push_back ( seed0 . diagnostic ( seed0 . DiagnosticSeverity . warning , std::string("SEED100") , std::string("Otimizacao tentou aumentar capacidades do programa.") , std::string("Reescrita bloqueada ate aprovacao explicita.") , 1.0 ; ) ) ; ) : ()) ; clean . bytecode = peephole ( clean . bytecode ) ; clean . notes = negotiate ( source ) ; clean . rewrite_stats = approve_rewrites ( clean , clean . notes ) ; return clean ; } ; main ( args ) { let src = io . read_text ( args [ 0 ] ) ; let out = compile ( src ) ; io . write_bytecode ( std::string("seed1.bin") , out . bytecode ) ; io . seed::print ( std::string("SEED1 ") + COMPILER_GENERATION + std::string("\n") ) ; io . seed::print ( std::string("passes ativos: ") + pass_table ( ) . filter ( p = > p . enabled ).size() + std::string("\n") ) ; io . print_report ( out . notes ) ; } ; ;
}

