Architecture
============

Goals
-----
- Separation of concerns for easy extension.
- Clear boundaries between Zig app logic, OpenAI integration, and platform-specific code.

Layout
------
- src/main.zig — app entrypoint (kept thin)
- src/openai/ — OpenAI-related Zig code
  - mod.zig — public entry re-exporting submodules
  - chatgpt.zig — Chat Completions helpers
  - models.zig — Models listing and parsing helpers
- src/platform/ — platform adapters and native implementations
  - ui.zig — Win32 UI adapter (message box, API key prompt, create window)
  - http.zig — HTTP adapter (WinINet GET with headers)
  - secrets.zig — Secrets adapter (DPAPI encrypt/decrypt in AppData)
  - win32/ — native C code used by adapters
    - win32_ui.c — window class, creation, message loop, prompt
    - win_http.c — HTTP GET via WinINet
    - win_secret.c — DPAPI encrypt/decrypt
- src/libc/ — small C utilities
  - c_functions.c — sample C function used by Zig
- inc/ — public C headers used by @cImport
  - win32_ui.h, win_http.h, win_secret.h, c_functions.h
- docs/ — project documentation

Build Wiring
------------
- `build.zig` registers the OpenAI module and compiles platform C code.
- C headers in `inc/` are made available via `exe.addIncludePath`.
- System libraries linked: `user32`, `gdi32`, `wininet`, `crypt32`.

Adapters (Why)
--------------
- Feature code imports `src/platform/*.zig` adapters instead of C headers.
- Adapters hide Win32 details and make future swaps (e.g., different HTTP backend) easier.

Adding a New Zig Module
-----------------------
1) Create folder `src/feature_x/` with `mod.zig` and subfiles.
2) Register in `build.zig`:
   const feature_x = b.addModule("feature_x", .{ .root_source_file = b.path("src/feature_x/mod.zig"), .target = target, .optimize = optimize });
   exe.root_module.addImport("feature_x", feature_x);
3) Use from Zig:
   const feature_x = @import("feature_x");

Subsystem Note
--------------
- App uses GUI subsystem (`exe.subsystem = .Windows;`). Console output won't appear by default. For debugging, temporarily switch to `.Console`.

Secrets
-------
- API keys are stored encrypted per-user in `%APPDATA%\ohmyzig\openai.key` using DPAPI.
- Runtime load order: `OPENAI_API_KEY` env var → encrypted AppData file → prompt.
- “Save key (encrypted)” in the prompt persists to AppData; `.env` is not used anymore.

