f <- rir.compile(function(a) a)
state <- rir.analysis.strictness(f)
stopifnot(state$strict["a"] == "ALWAYS")

f <- rir.compile(function(a) if (b) a)
state <- rir.analysis.strictness(f)
stopifnot(state$strict["a"] == "SOMETIMES")
stopifnot(state$strict["b"] == "ALWAYS")

f <- rir.compile(function(a) if (b) d)
state <- rir.analysis.strictness(f)
stopifnot(state$strict["a"] == "NEVER")
stopifnot(state$strict["b"] == "ALWAYS")
stopifnot(state$strict["d"] == "SOMETIMES")

f <- rir.compile(function(a, b) if (b) a else c)
state <- rir.analysis.strictness(f)
stopifnot(state$strict["a"] == "SOMETIMES")
stopifnot(state$strict["b"] == "ALWAYS")

f <- rir.compile(function(a, b) if (b) a else a)
state <- rir.analysis.strictness(f)
stopifnot(state$strict["a"] == "ALWAYS")
stopifnot(state$strict["b"] == "ALWAYS")

f <- rir.compile(function(a, b) { a <- 56; a ; b })
state <- rir.analysis.strictness(f)
stopifnot(state$strict["a"] == "NEVER")
stopifnot(state$strict["b"] == "YES")

f <- rir.compile(function(a, b) { if (g) a <- 56; a ; b })
state <- rir.analysis.strictness(f)
stopifnot(state$strict["a"] == "SOMETIMES")
stopifnot(state$strict["b"] == "ALWAYS")
