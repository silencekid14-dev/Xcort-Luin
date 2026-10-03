#ifndef LUIN_MATH_MODULE_H
#define LUIN_MATH_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// math module: original + 20 new (asin, acos, atan, atan2, sinh, cosh, tanh,
// deg, rad, factorial, is_prime, lerp, map, mod, fract, cbrt, dist, sum, mean, smoothstep)
// constants: pi, e, tau, phi
std::shared_ptr<Module> createMathModule();

} // namespace luin

#endif
