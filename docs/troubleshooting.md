Troubleshooting
===============

No Window Appears
-----------------
- Likely causes:
  - Window class not registered with the correct API (e.g., mixing `WNDCLASSEX` with `RegisterClass`).
  - `CreateWindowEx*` failed (invalid class name, missing fields, etc.).
- How this project surfaces the issue:
  - On failure, `MessageBoxA` is shown with `GetLastError()` code.
  - Because the app is GUI subsystem, console errors would otherwise be invisible.

Black Window or No Text
-----------------------
- Ensure `WM_PAINT` handles `BeginPaint`/`EndPaint` correctly and draws with a valid `HDC`.
- This project uses `TextOutA` with `strlen`. Verify `<string.h>` is included.

Console Output Not Visible
--------------------------
- GUI apps have no console attached. Options:
  - Temporarily set `exe.subsystem = .Console;` in `build.zig`.
  - Use `MessageBoxA` for quick checks.
  - Use a debugger and inspect Output window; optionally call `OutputDebugStringA`.

Character Encoding Mismatches
-----------------------------
- We explicitly use ANSI (`...A`) API variants to avoid `UNICODE` macro surprises.
- If switching to wide APIs (`...W`), migrate all text and lengths accordingly and use wide string literals (e.g., `L"..."`).

Message Loop Hangs or Exits Immediately
---------------------------------------
- `GetMessage` returns `0` on `WM_QUIT`. Make sure `PostQuitMessage` is only called when you intend to exit (e.g., on `WM_DESTROY`).
- Using `PeekMessage` without proper idle handling can cause busy loops—stick to `GetMessage` for simple apps.

Link Errors
-----------
- Undefined references to Win32 APIs typically mean a missing system library:
  - Add `exe.linkSystemLibrary("user32")` and/or `exe.linkSystemLibrary("gdi32")` in `build.zig`.
- C runtime calls (like `printf`) require `exe.linkLibC()`.

