# SEED 0.3 Language Specification

SEED 0.3 is the result-match dialect. It keeps the seven primitive runtime
instructions, preserves the obstinate compiler contract, and adds explicit
sum types, exhaustive branching and concise error propagation.

## Primitive Runtime

Every construct must lower to the original seven operations:

- `ALLOC`
- `FREE`
- `READ`
- `WRITE`
- `CMP`
- `JMP`
- `SYSCALL`

The compiler may invent derived operations, but they are only names for patterns
that lower back into these primitives.

## Source Form

```seed
module example

import stdlib.io
import stdlib.result

#[expires(data_rows > 100000)]
fn buscar_usuarios() effect fs -> Result[Array[Usuario], Error] {
    let rows = sql_query("SELECT id, nome FROM usuarios LIMIT 1000")?
    return Ok(rows)
}
```

## Algebraic Data Types

`data` declares a closed set of variants. Variants may carry values.

```seed
data Option[T] {
    Some(T)
    None
}

data Result[T,E] {
    Ok(T)
    Err(E)
}
```

ADTs are values. They lower to tagged records before reaching the seven runtime
primitives.

## Match

`match` selects by variant and must be exhaustive for closed ADTs.

```seed
fn describe(value: Result[Int, Error]) -> String {
    match value {
        Ok(n) => "ok: " + to_string(n)
        Err(e) => "erro: " + e.message
    }
}
```

Missing a variant emits `SEED005`. A wildcard `_` is allowed, but SEED may ask
for explicit branches when diagnostics or tests would become clearer.

## Error Propagation

`?` unwraps `Ok(value)` or `Some(value)`. On `Err(error)` or `None`, it returns
from the current function with the same failure shape.

```seed
fn read_config(path: String) effect fs -> Result[Config, Error] {
    let body = fs.read_text(path)?
    let parsed = parse_config(body)?
    return Ok(parsed)
}
```

A call returning `Result` or `Option` that is neither matched nor unwrapped with
`?` emits `SEED006`.

## Attributes

Attributes are promises, constraints or refusals placed directly on code.

- `#[keep]`: compile the original form even if SEED proposes a rewrite.
- `#[expires(condition)]`: mark code that must be rechecked when the condition
  becomes true.
- `#[fitness(metric)]`: define the metric used by the mutator.
- `#[capability(name)]`: explicitly grant a capability such as `net` or `fs`.
- `#[dream(false)]`: exclude a function or file from idle mutation.

## Effects

Effects make hidden power visible.

```seed
fn salvar(path: string, body: string) effect fs -> Result[Unit, Error] {
    fs.write_text(path, body)
}
```

Allowed effects:

- `none`
- `io`
- `fs`
- `net`
- `crypto`
- `allocate`
- `mutate_code`
- `unsafe`

A rewrite may reduce effects silently. A rewrite may not add effects without
human review.

## Contracts

Contracts describe properties the program must keep.

```seed
contract sorted_same_elements(input: Array[Int], output: Array[Int]) {
    assert(output.is_sorted())
    assert(output.multiset() == input.multiset())
}
```

Contracts are used by:

- compiler diagnostics
- generated tests
- mutator fitness
- synthesizer candidate selection

## Genitors

A genitor describes a family of possible programs instead of one fixed
implementation.

```seed
genitor Ordenador {
    entrada: Array[Int]
    saida: Array[Int]
    propriedade: saida.is_sorted() && saida.multiset() == entrada.multiset()
    fitness: runtime + memory + bytecode_size
}
```

The synthesizer creates candidates, the test runner kills invalid candidates,
and the mutator keeps the best survivor.

## Diagnostics

SEED does not hide disagreement. Diagnostics are structured:

```seed
Diagnostic {
    severity: proposal
    code: "SEED002"
    message: "Fibonacci recursivo ingenuo O(2^n)."
    suggestion: "Trocar por matriz O(log n)."
    confidence: 0.98
}
```

Core diagnostic codes:

- `SEED000`: approved program
- `SEED001`: inefficient sort pattern
- `SEED002`: naive Fibonacci recursion
- `SEED003`: code freshness or data growth issue
- `SEED004`: implicit type negotiation
- `SEED005`: non-exhaustive `match`
- `SEED006`: unchecked `Result` or `Option`
- `SEED100`: rewrite attempted to add capability

## Type Negotiation

SEED allows limited negotiated coercions, but explains them.

```seed
let nome: string = "Joao"
let idade: int = 30
let r = nome + idade
```

The compiler may lower this to:

```seed
let r = nome + to_string(idade)
```

It must emit `SEED004`.

## Rewrites

Every rewrite has:

- name
- before
- after
- reason
- before cost
- after cost
- accepted/refused state

Autonomous rewrites require:

- fitness delta greater than `0.05`
- safety score at least `0.90`
- property test score at least `0.99`
- no new capability

## License Rule

`#[keep]` and an explicit "NAO" from the programmer override optimization. SEED
may warn, but it must compile the original code.
