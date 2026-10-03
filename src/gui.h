#ifndef LUIN_GUI_MODULE_H
#define LUIN_GUI_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// Native "gui" module — thin, fully-working raylib bindings for Luin.
//
// Usage:
//   import gui
//   gui.init(800, 600, "My Window")
//   while gui.is_open() {
//     gui.begin()
//     gui.clear(30, 30, 40)
//     gui.text(20, 20, 24, "Hello", 255, 255, 255)
//     gui.end()
//   }
//   gui.close()
//
// Provides ~40 primitives: window, drawing, input, timing, camera helpers.
std::shared_ptr<Module> createGuiModule();

} // namespace luin

#endif // LUIN_GUI_MODULE_H
