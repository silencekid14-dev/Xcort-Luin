#ifndef LUIN_TIME_MODULE_H
#define LUIN_TIME_MODULE_H

#include "Interpreter.h"
#include <memory>

namespace luin {

// Builds the native "time" module used by `import time`.
//
// Members:
//   time.now()                -> current time as a string, local time,
//                                 e.g. "2026-08-08 14:32:07"
//   time.now(offset)          -> current time shifted by a GMT offset.
//                                 `offset` may be:
//                                   - a number of hours, e.g. time.now(3)
//                                   - a string like "GMT+3", "GMT-05:30",
//                                     "+0300", or "-05:30"
//                                 e.g. show(time.now("GMT+0300"))
//   time.today()               -> current date only, "2026-08-08"
//   time.clock()                -> current time only, "14:32:07"
//   time.year() / month() / day() / hour() / minute() / second()
//                                -> the current local value as an int
//   time.stamp()                 -> Unix timestamp (seconds since epoch) as int
//
// Usage from a .sx script:
//   import time
//   show(time.now())
//   show(time.now("GMT +0300"))
//   show(time.now(-5))
std::shared_ptr<Module> createTimeModule();

} // namespace luin

#endif // LUIN_TIME_MODULE_H
