#ifndef LUIN_APP_MODULE_H
#define LUIN_APP_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// app module: create/add_screen/set_start/banner/clear/pause/confirm/menu/table/title
// + 20: input, ask_int, ask_float, progress, color, print, println, hr, box,
// choice, spinner, alert, error, success, warn, indent, center, columns, yes_no, countdown
std::shared_ptr<Module> createAppModule();

} // namespace luin

#endif
