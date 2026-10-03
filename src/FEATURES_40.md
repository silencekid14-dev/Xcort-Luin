# Luin 2.6+ — 40 C++-inspired features with simple Luin syntax

Luin keeps a deliberately short, readable syntax.  
Below are **40 features found in modern C++**, mapped to **simple Luin forms**.  
`switch` is fully implemented in the interpreter (see source changes).  
The rest are the official proposed / planned surface for upcoming versions;  
example programs use pure Luin syntax so they stay readable today.

---

## 1. switch / case / default  (IMPLEMENTED)

Eliminates long if/elf/els chains. No fall-through (each case is exclusive).

```luin
day = 3
switch day {
  case 1:
    show("Monday")
  case 2, 3:
    show("Tue or Wed")
  case 4, 5:
    show("Thu or Fri")
  default:
    show("Weekend")
}
```

## 2. enum (named integer constants)

```luin
enum Color {
  Red = 1
  Green = 2
  Blue = 3
}
c = Color.Red
```

## 3. struct / record (lightweight data)

```luin
struct Point {
  x
  y
}
p = Point{10, 20}
show(p.x)
```

## 4. const variables

```luin
const PI = 3.14159
```

## 5. auto type deduction

```luin
auto n = 42
auto s = "hi"
```

## 6. lambda / anonymous functions

```luin
add = fn(a, b) { rtn a + b }
show(add(2, 3))
```

## 7. range-based for (already present)

```luin
for x in [1, 2, 3] {
  show(x)
}
```

## 8. structured bindings / unpack

```luin
(x, y) = Point{3, 4}
```

## 9. default arguments

```luin
fn greet(name = "world") {
  show(f"Hello {name}")
}
```

## 10. namespaces

```luin
ns mathx {
  fn square(n) { rtn n * n }
}
show(mathx.square(5))
```

## 11. inheritance

```luin
cls Animal {
  fn speak() { show("...") }
}
cls Dog : Animal {
  fn speak() { show("woof") }
}
```

## 12. override / final markers

```luin
fn speak() override { show("meow") }
fn sealed() final { }
```

## 13. static members

```luin
cls Counter {
  static count = 0
  fn inc() {
    Counter.count = Counter.count + 1
  }
}
```

## 14. operator overloading (selected ops)

```luin
cls Vec {
  fn __add__(other) {
    rtn Vec{self.x + other.x, self.y + other.y}
  }
}
```

## 15. nullptr / null

```luin
p = null
if p == null { show("empty") }
```

## 16. ternary / conditional expression

```luin
msg = age >= 18 ? "adult" : "minor"
```

## 17. compound assignment

```luin
x += 1
y *= 2
```

## 18. increment / decrement

```luin
i++
--j
```

## 19. bitwise operators

```luin
flags = a | b
masked = x & 0xff
shifted = n << 2
```

## 20. logical short-circuit (and / or)

```luin
if a and b { }
if x or y { }
```

## 21. try / catch with typed handlers (extends existing try)

```luin
try {
  risky()
} catch e {
  show(e)
}
```

## 22. finally / defer cleanup

```luin
defer close(f)
```

## 23. using / alias

```luin
using Vec2 = Point
```

## 24. modules (already present via import)

```luin
import math
import "mylib.sx"
```

## 25. inline / small-fn hint

```luin
inline fn twice(x) { rtn x * 2 }
```

## 26. constexpr / compile-time eval (planned)

```luin
constexpr FACT5 = 120
```

## 27. concept-like constraints (simple)

```luin
fn add(a: num, b: num) { rtn a + b }
```

## 28. initializer lists

```luin
nums = {1, 2, 3, 4}
```

## 29. array / slice helpers

```luin
first = arr[0]
slice = arr[1:3]
```

## 30. map / dictionary literal

```luin
m = {"name": "Luin", "ver": 2.5}
show(m["name"])
```

## 31. optional / maybe type

```luin
opt = some(42)
opt = none
```

## 32. variant / sum type (simple)

```luin
v = either(1, "hi")
```

## 33. RAII-style resource (with)

```luin
with open("f.txt") as f {
  show(f.read())
}
```

## 34. move / ownership hint

```luin
v2 = move(v1)
```

## 35. friend-like access

```luin
cls Box {
  friend helper
}
```

## 36. attributes / annotations

```luin
@deprecated
fn old_api() { }
```

## 37. spaceship / three-way compare

```luin
cmp = a <=> b   # -1, 0, 1
```

## 38. fold / reduce helpers

```luin
sum = fold(arr, 0, fn(a,b){ rtn a+b })
```

## 39. generator / yield (simple coroutines)

```luin
fn count(n) {
  i = 0
  while i < n {
    yield i
    i = i + 1
  }
}
```

## 40. pattern matching (extends switch)

```luin
match val {
  case 0: show("zero")
  case n if n > 10: show("big")
  case [a, b]: show(f"pair {a} {b}")
  default: show("other")
}
```

---

## Design principles used

- Prefer short keywords already familiar in Luin (`fn`, `cls`, `rtn`, `sloop`, `elf`, `els`).
- Braces `{}` for blocks, optional `:` after conditions.
- No mandatory semicolons.
- `show` / `ask` stay the I/O surface.
- Features that need deep runtime support are marked planned; syntax is frozen so code written today stays valid.
