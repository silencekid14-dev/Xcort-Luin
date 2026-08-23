#ifndef LUIN_RANDOM_MODULE_H
#define LUIN_RANDOM_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// Builds the native "random" module used by `import random`.
//
// Members:
//   random.int(min, max)    -> random integer in [min, max], inclusive
//   random.float()            -> random double in [0.0, 1.0)
//   random.choice(arr)          -> a random element from an array
//   random.seed(n)                -> reseeds the generator (for repeatable runs)
//
// Usage from a .sx script:
//   import random
//   show(random.int(1, 6))
//   show(random.choice(["rock", "paper", "scissors"]))
std::shared_ptr<Module> createRandomModule();

} // namespace luin

#endif // LUIN_RANDOM_MODULE_H
