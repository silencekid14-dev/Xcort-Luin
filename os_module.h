#ifndef LUIN_OS_MODULE_H
#define LUIN_OS_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// Builds the native "os" module used by `import os`.
// Complements the existing get()/del()/crt() builtins with a few
// read-only, script-friendly filesystem helpers.
//
// Members:
//   os.exists(path)    -> true/false
//   os.cwd()             -> current working directory as a string
//   os.listdir(path)       -> array of entry names (strings) in `path`
//   os.mkdir(path)           -> creates a directory (like crt() for folders)
//   os.isFile(path)            -> true/false
//   os.isFolder(path)            -> true/false
//
// Usage from a .sx script:
//   import os
//   if os.exists("save.txt") { show("save found") }
std::shared_ptr<Module> createOsModule();

} // namespace luin

#endif // LUIN_OS_MODULE_H
