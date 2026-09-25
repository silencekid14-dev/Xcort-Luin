#ifndef LUIN_VERSION_H
#define LUIN_VERSION_H

#include <string>

namespace luin {

// Current shipping version of this binary. Bump this (and the
// binary name / CHANGES file) for every public release.
constexpr const char* LUIN_VERSION = "2.4";

// Platform tag embedded in the state file so a shared state across
// different builds (Windows .exe / macOS / Linux) is still distinguishable.
#if defined(_WIN32)
constexpr const char* LUIN_PLATFORM = "windows";
#elif defined(__APPLE__)
constexpr const char* LUIN_PLATFORM = "macos";
#else
constexpr const char* LUIN_PLATFORM = "linux";
#endif

// Called once at process start (from main).
// - Network check against https://xluin.netlify.app/version.txt is performed
//   at most once per hour (timestamp stored in the state JSON).
// - If the remote version is strictly newer than LUIN_VERSION, a short
//   update notice (and any news text from the remote file) is printed.
// - The notice is repeated on subsequent runs while this binary is still
//   the old version, even if the 1-hour network cooldown has not elapsed.
// - Failures (no network, bad file, parse error) are silent; the program
//   continues normally.
void checkForUpdates();

} // namespace luin

#endif // LUIN_VERSION_H
