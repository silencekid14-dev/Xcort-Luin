#ifndef LUIN_ARRAYS_MODULE_H
#define LUIN_ARRAYS_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// Native "arrays" module — push/pop/sort plus complex helpers:
// length, first, last, slice, concat, unique, flatten, sum, avg, min, max,
// index_of, fill, range, zip, chunk, insert, remove_at, repeat.
std::shared_ptr<Module> createArraysModule();

} // namespace luin

#endif
