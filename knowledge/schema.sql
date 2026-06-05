PRAGMA journal_mode=WAL;

CREATE TABLE IF NOT EXISTS algoritmos (
  id INTEGER PRIMARY KEY,
  name TEXT UNIQUE NOT NULL,
  description TEXT NOT NULL,
  complexity TEXT NOT NULL,
  code TEXT NOT NULL,
  fitness REAL DEFAULT 0.0
);

CREATE TABLE IF NOT EXISTS otimizacoes (
  id INTEGER PRIMARY KEY,
  pattern TEXT NOT NULL,
  replacement TEXT NOT NULL,
  explanation TEXT NOT NULL,
  min_gain REAL DEFAULT 0.0
);

CREATE TABLE IF NOT EXISTS papers (
  id INTEGER PRIMARY KEY,
  title TEXT NOT NULL,
  year INTEGER,
  summary TEXT NOT NULL,
  local_note TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS historia (
  id INTEGER PRIMARY KEY,
  ts TEXT NOT NULL,
  file TEXT NOT NULL,
  reason TEXT NOT NULL,
  before_hash TEXT,
  after_hash TEXT
);

INSERT OR IGNORE INTO algoritmos(name, description, complexity, code, fitness)
VALUES
('sort.timsort', 'ordenar lista de numeros com dados parcialmente ordenados', 'O(n log n)', 'fn sort(xs){ timsort(xs) }', 0.88),
('sort.quicksort', 'ordenar lista de numeros geral', 'O(n log n) average', 'fn sort(xs){ if len(xs)<=1{xs}else{ let p=xs[0]; sort(filter(xs[1..], x=>x<=p))+[p]+sort(filter(xs[1..], x=>x>p)) } }', 0.74),
('fib.matrix', 'fibonacci exato por exponenciacao de matriz', 'O(log n)', 'fn fib(n){ matrix_fib(n) }', 0.91);

INSERT OR IGNORE INTO otimizacoes(pattern, replacement, explanation, min_gain)
VALUES
('fib(n-1) + fib(n-2)', 'matrix_fib(n)', 'Fibonacci recursivo ingenuo e exponencial; matriz reduz para O(log n).', 0.50),
('SELECT * FROM', 'SELECT explicit_columns FROM', 'SELECT * envelhece mal e desperdiça IO.', 0.05),
('bubble sort nested loops', 'timsort', 'Bubble sort e rejeitado exceto para ensino ou #[keep].', 0.20);
