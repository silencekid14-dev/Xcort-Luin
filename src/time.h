#ifndef LUIN_TIME_MODULE_H
#define LUIN_TIME_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// Native "time" module — clock, calendar, formatting, sleep, elapsed.
// Expanded with 30 additional features beyond the original set.
std::shared_ptr<Module> createTimeModule();

} // namespace luin

#endif // LUIN_TIME_MODULE_H
