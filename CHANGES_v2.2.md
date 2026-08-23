# Luin v2.1.2 → v2.2

Rebuilds cleanly with `g++ -std=c++17` (see `build.sh`). All four
requested features below were implemented and smoke-tested; `code.sx`
and `code1.sx` from v2.1.2 still run to completion (exit code 0)
unchanged.

## 1. Multiple .sx files in one invocation

`main.cpp` now accepts any number of `.sx` files on the command line:

```
Luin_v2.2.exe test.sx if.sx
.\Luin_v2.2.exe test.sx if.sx
./Luin_v2.2 test.sx if.sx
```

Each file runs as its own independent program (fresh interpreter/scope
per file — they don't share variables). Between files, a marker line is
printed:

```
== test.sx run: ==
...test.sx's output...

== if.sx run: ==
...if.sx's output...
```

This works identically whether invoked from a plain OS terminal, VS
Code's terminal, or X-Luin-VSCode's built-in terminal — the marker line
is plain, readable text on its own; X-Luin-VSCode's `terminal_panel.py`
additionally re-styles it as a header for readability, but doesn't
require it.

Running exactly one file behaves exactly as v2.1.2 did (no markers, no
behavior change), so nothing that depended on single-file output breaks.

## 2. Expanded math module (9 new functions)

`math.cpp` / `math.h` add, on top of the existing 12 members:

- `math.round(x)`, `math.trunc(x)`
- `math.hypot(a, b)`
- `math.log2(x)`, `math.log10(x)`
- `math.gcd(a, b)`, `math.lcm(a, b)`
- `math.clamp(x, lo, hi)`
- `math.sign(x)`

## 3. `import "some_program.sx"`

`import` now also accepts a `.sx` filename, in addition to native module
names:

```
import "helpers.sx"
import helpers.sx        # equivalent bare-word form
```

This loads and runs the target file's top-level statements once (guarded
against duplicate/circular imports by resolved absolute path), so any
`fn`/`cls` it defines become callable/instantiable from the importing
script afterward. Implemented in `Interpreter::executeSxFileImport()`
(`Interpreter.cpp/h`) using the same `Lexer`/`Parser` pipeline as
`main.cpp`, and `Parser::parseImportStmt()` (`Parser.cpp`) now accepts
either a string literal or a bare `identifier.identifier` filename.

## 4. Five new native modules

New files, same `Module`/`NativeFunction` pattern as `math.cpp/h`:

- **`time.cpp/h`** (`import time`) — `time.now()`, `time.now(offset)`
  with GMT offsets (`time.now(3)`, `time.now("GMT+0300")`,
  `time.now("GMT-05:30")`), `time.today()`, `time.clock()`,
  `time.year()/month()/day()/hour()/minute()/second()`, `time.stamp()`.
- **`string_module.cpp/h`** (`import string`) — `upper`, `lower`,
  `trim`, `split`, `join`, `replace`, `contains`.
- **`random_module.cpp/h`** (`import random`) — `random.int(min,max)`,
  `random.float()`, `random.choice(arr)`, `random.seed(n)`.
- **`arrays_module.cpp/h`** (`import arrays`) — `push`, `pop`,
  `reverse`, `sort`, `contains` (all return a *new* array; Luin arrays
  passed as `Value` are otherwise immutable-by-convention here).
- **`os_module.cpp/h`** (`import os`) — `exists`, `cwd`, `listdir`,
  `mkdir`, `isFile`, `isFolder`.

## Files touched

- `main.cpp` — multi-file loop, per-file marker banner
- `math.cpp`, `math.h` — 9 new functions
- `Interpreter.cpp`, `Interpreter.h` — module wiring, `.sx` import
- `Parser.cpp` — `import "file.sx"` / `import file.sx` parsing
- **new:** `time.cpp/h`, `string_module.cpp/h`, `random_module.cpp/h`,
  `arrays_module.cpp/h`, `os_module.cpp/h`
- unchanged: `AST.cpp/h`, `Lexer.cpp/h`, `Token.cpp/h`, `Parser.h`
