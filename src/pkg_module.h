#ifndef LUIN_PKG_MODULE_H
#define LUIN_PKG_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// Native "pkg" module — lightweight package / dependency helpers for Luin.
// ~30 features for listing, loading, versioning, and simple package metadata.
std::shared_ptr<Module> createPkgModule();

} // namespace luin

#endif // LUIN_PKG_MODULE_H
