# Luin v2.3 → v2.4

## Version checker

New files: `version.h` / `version.cpp` / `version.txt`

On every launch the interpreter calls `luin::checkForUpdates()`:

- Fetches `https://xluin.netlify.app/version.txt` **at most once per hour**.
- Compares the remote version number with the compile-time constant
  `LUIN_VERSION` (`"2.4"`).
- If the remote version is **strictly newer**, prints a short update notice
  (plus any news text that follows the version line in the remote file) to
  **stderr**.
- While this binary is still the old version the notice is repeated on later
  runs (even inside the 1-hour network cooldown), so the user keeps seeing
  the reminder until they upgrade.
- State (last check timestamp, last seen remote version, last notified
  version, platform tag, cached news) is stored in a small JSON file:
  - Linux / macOS: `~/.luin/version_state.json`
  - Windows: `%USERPROFILE%\.luin\version_state.json`
  - Fallback: `./luin_version_state.json`
- Network or parse failures are silent; the program continues normally.
- Platform tag (`windows` / `macos` / `linux`) is recorded in the state
  file so mixed installs stay distinguishable.

### Remote `version.txt` format

```
2.5
Optional multi-line news / changelog text.
Please download from https://xluin.netlify.app
```

Only the first non-empty token of the first non-empty line is treated as the
version number; everything after that becomes the news body.

### Build

`build.sh` now produces `./Luin_v2.4` and includes `version.cpp`.

## Compatibility

Existing v2.3 scripts continue to run unchanged.
