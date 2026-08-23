# Luin v2.2 → v2.3

## Major additions

### 1. Compiled form (.sxc)
- `Luin_v2.3 program.sx -b`   produces `program.sxc`
- `Luin_v2.3 program.sxc -r`  (or just `Luin_v2.3 program.sxc`) runs it
- Format: binary token stream only. Original source text is never stored.
  Magic "LUINSXC1", version, count, then (type, line, lexeme) records.
- This is the "instructions that Luin understands" requested by the user.

### 2. Native `app` module (`import app`)
UI primitives that make interactive console programs short and readable:
- app.banner(text)
- app.menu(title, options_array) → chosen index or -1
- app.table(headers, rows)
- app.confirm(prompt) → bool
- app.clear()
- app.pause([msg])
- app.create / add_screen / set_start / title (foundation for richer apps)

The control-flow loop stays in pure Luin so behaviour is transparent.

### 3. Package foundation
- Import of `.sx` files already existed; search paths and `.sxp` archives
  are the next incremental step (see roadmap).

### 4. Roadmap of the remaining ~45 features
See the detailed list in the original analysis. Priorities after this release:
- Dictionary type
- map / filter / reduce
- Default & rest parameters
- Package directory search + install
- JSON module
- Stronger error objects

## Files added
- sxc.h / sxc.cpp
- app_module.h / app_module.cpp
- app_demo.sx

## Compatibility
Existing v2.2 scripts continue to run unchanged.
