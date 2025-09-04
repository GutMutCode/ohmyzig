Architecture
============

Goals
-----
- Separation of concerns for easy extension.
- Clear boundaries between Zig app logic, OpenAI integration, and platform-specific UI.

Layout
------
- src/main.zig — app entrypoint. Keep this thin; import modules as needed.
- src/openai/ — OpenAI-related Zig code
  - mod.zig — public entry that re-exports submodules
  - chatgpt.zig — Chat Completions helpers
  - models.zig — Models listing and parsing helpers
- src/platform/win32/ — Win32-specific C sources
  - win32_ui.c — window class, creation, message loop
- src/libc/ — small C utilities
  - c_functions.c — sample C function used by Zig
- inc/ — public C headers used by @cImport
  - win32_ui.h, c_functions.h
- docs/ — project documentation

Build Wiring
------------
- `build.zig` registers a named Zig module `openai`:
  - Importable via `const openai = @import("openai");`
  - Add new modules similarly: create a `src/<name>/mod.zig` and register with `b.addModule` + `exe.root_module.addImport("<name>", ...)`.
- C sources are grouped by concern and compiled with `addCSourceFiles`.
  - Headers live in `inc/` and are added via `exe.addIncludePath(b.path("inc"))`.

Adding a New Zig Module
-----------------------
1) Create folder: `src/feature_x/` with `mod.zig` and subfiles.
2) Register in `build.zig`:
   ```zig
   const feature_x = b.addModule("feature_x", .{ .root_source_file = b.path("src/feature_x/mod.zig"), .target = target, .optimize = optimize });
   exe.root_module.addImport("feature_x", feature_x);
   ```
3) Use from Zig:
   ```zig
   const feature_x = @import("feature_x");
   ```

Adding a New C Component
------------------------
1) Put C source under `src/<group>/` (e.g., `src/platform/win32/foo.c`).
2) Put headers under `inc/` (public) or private beside the C file.
3) Update `build.zig` `exe.addCSourceFiles` list and include flags if needed.

Subsystem Note
--------------
- App uses GUI subsystem (`exe.subsystem = .Windows;`). Console output won’t appear by default. For debugging, temporarily switch to `.Console`.

Secrets
-------
- Store API keys in `.env` and ensure it’s git-ignored. Do not commit secrets.

