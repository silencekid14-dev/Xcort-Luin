#ifndef LUIN_TIME_MODULE_H
#define LUIN_TIME_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

std::shared_ptr<Module> createTimeModule();

} // namespace luin

#endif // LUIN_TIME_MODULE_H
