Build and Linking
=================

This project mixes Zig and C code, and links Win32 system libraries to build a GUI application.

Zig Build Script (`build.zig`)
------------------------------
- Entry: `pub fn build(b: *std.Build) void`.
- Target/optimize: configured via standard CLI flags (`-Dtarget=...`, `-Drelease-fast`, etc.).
- Root module: `src/main.zig`.
- Executable name: `ohmyzig`.
- Include path: `exe.addIncludePath(b.path("inc"))` exposes headers to Zig’s C importer and to C compilation.
- C sources: `exe.addCSourceFiles(.{ .files = &.{ "src/c_functions.c", "src/win32_ui.c" }, .flags = &.{ "-Iinc" }, });`
  - Passes `-Iinc` so the C compiler can find headers.
- Linking:
  - `exe.linkLibC();` — for `printf` used in `c_functions.c`.
  - `exe.linkSystemLibrary("user32");` — Core Win32 UI APIs.
  - `exe.linkSystemLibrary("gdi32");` — GDI text output (`TextOutA`).
- Subsystem:
  - `exe.subsystem = .Windows;` — Builds a GUI app (no console attached).
  - For debugging printouts, temporarily switch to `.Console`.
- Install and run steps:
  - `b.installArtifact(exe);` — `zig build` places the exe under `zig-out/bin`.
  - `b.addRunArtifact(exe)` + `b.step("run", ...)` — enables `zig build run`.

Zig ↔ C Interop
---------------
- Zig imports C headers via `@cImport` in `src/main.zig`, exposing functions under the `c` namespace.
- Headers `inc/*.h` use `extern "C"` guards to avoid C++ name mangling if compiled in a C++ context.
- C files are compiled by Zig’s build system and linked into the final executable.

Artifacts
---------
- Output exe and PDB (debug symbols) land in `zig-out/bin/`.
- Build cache in `.zig-cache/`.
- These are ignored in VCS by `.gitignore`.

