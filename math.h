#ifndef LUIN_MATH_MODULE_H
#define LUIN_MATH_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// Builds the native "math" module used by the `import math` statement.
// Exposes (14 members total, kept under the 15-feature budget):
//   functions: sqrt, pow, abs, floor, ceil, sin, cos, tan, log, exp, min, max
//   constants: pi, e
//
// Usage from a .sx script:
//   import math
//   show(math.sqrt(16))
//   show(math.sin(math.pi / 2))
std::shared_ptr<Module> createMathModule();

} // namespace luin

#endif // LUIN_MATH_MODULE_H
