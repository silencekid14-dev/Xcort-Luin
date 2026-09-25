#ifndef LUIN_STRING_MODULE_H
#define LUIN_STRING_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// Builds the native "string" module used by `import string`.
//
// Members:
//   string.upper(s)              -> uppercased copy of s
//   string.lower(s)               -> lowercased copy of s
//   string.trim(s)                 -> s with leading/trailing whitespace removed
//   string.split(s, sep)            -> array of substrings, split on `sep`
//   string.join(arr, sep)            -> concatenates array elements with `sep`
//   string.replace(s, from, to)       -> s with all occurrences of `from` replaced by `to`
//   string.contains(s, needle)         -> true/false
//
// Usage from a .sx script:
//   import string
//   show(string.upper("hero"))
//   show(string.split("a,b,c", ","))
std::shared_ptr<Module> createStringModule();

} // namespace luin

#endif // LUIN_STRING_MODULE_H
