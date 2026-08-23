#ifndef LUIN_ARRAYS_MODULE_H
#define LUIN_ARRAYS_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// Builds the native "arrays" module used by `import arrays`.
// (Named "arrays", not "array", to avoid colliding with the `arr`-style
// variable names scripts commonly use.)
//
// Members:
//   arrays.push(arr, value)   -> new array with `value` appended (arr unchanged)
//   arrays.pop(arr)             -> new array with the last element removed
//   arrays.reverse(arr)           -> new array with elements in reverse order
//   arrays.sort(arr)                -> new array sorted ascending (numbers or strings)
//   arrays.contains(arr, value)       -> true/false
//
// Usage from a .sx script:
//   import arrays
//   nums = arrays.push(nums, 42)
//   show(arrays.sort([3, 1, 2]))
std::shared_ptr<Module> createArraysModule();

} // namespace luin

#endif // LUIN_ARRAYS_MODULE_H
