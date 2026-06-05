# SEED Evolution Model

SEED evolves through local, reviewable changes. It does not use an external
neural network or send source code to third parties.

## Cycle

1. Compile source with `seed0` or `seed1`.
2. Collect diagnostics and cost estimates.
3. Generate rewrite candidates.
4. Run property tests and benchmarks.
5. Apply safe autonomous rewrites or enqueue review items.
6. Save the previous version in `history/`.
7. Append the decision to `dreams/dream.log`.

## Fitness

Fitness is a weighted score:

```text
fitness =
    runtime_delta * 0.40 +
    memory_delta * 0.20 +
    bytecode_delta * 0.15 +
    readability_delta * 0.10 +
    property_score * 0.15
```

The mutator never accepts a faster version that breaks a property.

## Safety Gates

A mutation is blocked or sent to review if it:

- adds `net`, `fs.write`, `unsafe` or raw `syscall`
- removes a contract
- deletes tests
- changes public function shape without an adapter
- touches files marked `#[dream(false)]`

## Morning Log

Dreams write compact records:

```text
[03:14] MUTACAO: stdlib/string.seed - parse_int ficou 12% mais rapido
[04:02] REVISAO: programs/api.seed - reescrita adiciona net
[05:31] RECUSADA: seed1.seed - ganho insuficiente
```

## Human Control

The programmer controls the ceiling:

- disable dreams with `#[dream(false)]`
- preserve code with `#[keep]`
- require review by setting autonomous threshold above `1.0`
- delete SEED completely; the license requires full uninstall support
