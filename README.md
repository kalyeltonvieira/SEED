# SEED

SEED e uma linguagem experimental que trata o compilador como um par programador.
Ela nasce de sete primitivas assembly e cresce por regras locais de analise,
mutacao, teste e revisao humana.

Versao atual da semente: `0.3-result-match`.

O sistema nesta pasta e a semente inicial:

- `runtime/`: Runtime C nativo com VM de 7 primitivas e interface de sistema.
- `seed0.seed`: compilador minimo escrito no dialeto bootstrap, agora com
  diagnosticos estruturados, efeitos, atributos, ADTs, `match` e AST rasa.
- `seed1.seed`: seed0 com passes de otimizacao, negociacao, auditoria de
  efeitos, exaustividade e contratos de validade.
- `mutator.seed`: gera mutacoes, mede fitness e versiona em `history/`.
- `synthesizer.seed`: sintetiza codigo a partir de descricoes usando conhecimento local.
- `docs/language.md`: especificacao do dialeto SEED 0.3.
- `examples/`: programas que mostram a linguagem negociando com o programador.
- `stdlib/`: biblioteca padrao SEED.
- `knowledge/`: base local de algoritmos, otimizacoes, testes e schema SQLite.
- `dreams/`, `generations/`, `tests/`, `licenses/`: runtime evolutivo.

## Principio

A SEED nao confia automaticamente no programador, mas obedece a Licenca SEED:
ela explica, aceita "NAO", nao envia codigo para terceiros e nao evolui alem do
permitido.

## Dialeto 0.3

A linguagem agora entende estes conceitos:

- `#[keep]`: compila a versao original mesmo quando a SEED discorda.
- `#[expires(...)]`: marca funcoes que envelhecem com dados, tempo ou contexto.
- `effect`: declara capacidades como `io`, `fs`, `net`, `crypto` e `mutate_code`.
- `contract`: define propriedades que o codigo precisa manter.
- `data`: declara tipos algebricos com variantes nomeadas.
- `match`: exige tratamento explicito das variantes relevantes.
- `?`: propaga `Err`/`None` sem esconder a falha.
- `genitor`: descreve um programa por entradas, saidas e propriedades; a SEED
  gera candidatos e seleciona os sobreviventes.
- diagnosticos `SEED000..SEED100`: todo aviso tem causa, sugestao e confianca.

Exemplo:

```seed
#[expires(data_rows > 100000)]
fn buscar_usuarios() effect fs -> Result[Array[Usuario], Error] {
    let rows = sql_query("SELECT id, nome FROM usuarios LIMIT 1000")?
    return Ok(rows)
}

fn descrever(resultado: Result[Array[Usuario], Error]) -> String {
    match resultado {
        Ok(rows) => "usuarios: " + to_string(rows.len)
        Err(e) => "erro: " + e.message
    }
}
```

A SEED compila, registra a validade da funcao e exige que a falha da consulta
seja tratada com `match` ou propagada com `?`.

## Build

```bat
build.bat
```

O build compila o runtime C nativo para `seed.exe`. Se `sqlite3.exe` existir, a base
`knowledge.db` e criada como SQLite real; caso contrario, e criado um arquivo
seeddb local inicial para a linguagem importar depois.

## Uso rapido

```bat
seed.exe seed0.seed -o seed0.bin
seed.exe seed1.seed -o seed1.bin
```

O executavel atual é o runtime C nativo. A semantica rica vive nos
arquivos `.seed`, que sao a especificacao evolutiva que as proximas geracoes do
compilador devem assumir.

## Estado do `seed.exe`

O compilador ativo em `runtime/seed_runtime.cpp` e o caminho canonico para
programas `.seed` nesta geracao. Ele transpila para C++23 e ja cobre o
subconjunto inicial de `0.3-result-match`: assinaturas com `effect`, tipos
`Result[T,E]`/`Option[T]`, propagacao `?` em declaracoes `let`, construtores
`Ok`/`Err`, e `match` simples sobre `Ok`/`Err` e `Some`/`None`.

Ainda faltam a checagem semantica completa de exaustividade, contratos,
genitors, auditoria real de efeitos e o backend final nas sete primitivas.
