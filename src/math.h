#ifndef LUIN_MATH_MODULE_H
#define LUIN_MATH_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// Builds the native "math" module used by the `import math` statement.
//
// v2.1 members: sqrt, pow, abs, floor, ceil, sin, cos, tan, log, exp,
//               min, max, pi, e
//
// v2.2 adds (9 new members, still comfortably under a "few dozen" budget):
//   round(x)          -> nearest integer (ties away from zero)
//   trunc(x)          -> integer part, toward zero
//   hypot(a, b)        -> sqrt(a*a + b*b), the length of the hypotenuse
//   log2(x)            -> base-2 logarithm
//   log10(x)           -> base-10 logarithm
//   gcd(a, b)          -> greatest common divisor (integers)
//   lcm(a, b)          -> least common multiple (integers)
//   clamp(x, lo, hi)   -> x restricted to the [lo, hi] range
//   sign(x)            -> -1, 0, or 1
//
// Usage from a .sx script:
//   import math
//   show(math.round(3.6))
//   show(math.clamp(150, 0, 100))
std::shared_ptr<Module> createMathModule();

} // namespace luin

#endif // LUIN_MATH_MODULE_H
