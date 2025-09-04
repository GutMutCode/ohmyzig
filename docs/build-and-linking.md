Build and Linking
=================

This project mixes Zig and C code, and links Win32 system libraries to build a GUI application.

Zig Build Script (`build.zig`)
------------------------------
- Entry: `pub fn build(b: *std.Build) void`.
- Target/optimize: configured via standard CLI flags (`-Dtarget=...`, `-Drelease-fast`, etc.).
- Root module: `src/main.zig`.
- Executable name: `ohmyzig`.
- Include path: `exe.addIncludePath(b.path("inc"))` exposes headers to Zig's C importer and to C compilation.
- C sources:
  - `src/libc/c_functions.c`
  - `src/platform/win32/win32_ui.c`
  - `src/platform/win32/win_http.c`
  - `src/platform/win32/win_secret.c`
- Linking:
  - `exe.linkLibC();` — for `printf` used in `c_functions.c`.
  - `exe.linkSystemLibrary("user32");` — Core Win32 UI APIs.
  - `exe.linkSystemLibrary("gdi32");` — GDI text output (`TextOutA`).
  - `exe.linkSystemLibrary("wininet");` — WinINet HTTP.
  - `exe.linkSystemLibrary("crypt32");` — DPAPI encryption.
- Subsystem:
  - `exe.subsystem = .Windows;` — Builds a GUI app (no console attached).
  - For debugging printouts, temporarily switch to `.Console`.
- Install and run steps:
  - `b.installArtifact(exe);` — `zig build` places the exe under `zig-out/bin`.
  - `b.addRunArtifact(exe)` + `b.step("run", ...)` — enables `zig build run`.

Zig ↔ C Interop
---------------
- Zig adapters import C headers via `@cImport` in `src/platform/*.zig`.
- Adapters expose Zig-friendly APIs to features, hiding Win32 details.
- C files are compiled by Zig's build system and linked into the final executable.

Named Zig Modules
-----------------
- The build registers a named module `openai` for API helpers:
  - Registration in `build.zig`:
    - `const openai_mod = b.addModule("openai", .{ .root_source_file = b.path("src/openai/mod.zig"), ... });`
    - `exe.root_module.addImport("openai", openai_mod);`
  - Usage from Zig: `const openai = @import("openai");`

Artifacts
---------
- Output exe and PDB (debug symbols) land in `zig-out/bin/`.
- Build cache in `.zig-cache/`.
- These are ignored in VCS by `.gitignore`.

