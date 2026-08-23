#ifndef LUIN_APP_MODULE_H
#define LUIN_APP_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// Native "app" module — the simplest possible way to build structured
// interactive console applications in Luin.
//
// Design goal: fewer concepts than a typical Python CLI framework.
// Everything is a plain Value (string / number / array / dict-like via arrays
// of pairs for now). No hidden global state beyond what the user explicitly
// puts into the returned App object.
//
// Core idea:
//   import app
//   a = app.create("My Tool")
//   app.add_screen(a, "home", home_fn)
//   app.run(a)
//
// The module supplies:
//   app.create(title)          -> App object (a ClassInstance)
//   app.add_screen(app, name, fn)
//   app.set_start(app, name)
//   app.run(app)               -> enters the main loop
//   app.menu(title, options)   -> shows a numbered menu, returns chosen index (0-based) or -1
//   app.ask(prompt)            -> string input (wrapper around the language ask)
//   app.confirm(prompt)        -> bool
//   app.clear()                -> clear console (best-effort)
//   app.pause([msg])           -> wait for Enter
//   app.banner(text)           -> pretty header
//   app.table(headers, rows)   -> simple columnar display
//
// This is intentionally small and composable. Larger frameworks can be built
// on top as pure .sx packages later.

std::shared_ptr<Module> createAppModule();

} // namespace luin

#endif // LUIN_APP_MODULE_H
