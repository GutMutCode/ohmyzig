Win32 Overview
==============

This document explains how the current codebase creates and runs a basic Win32 GUI window using C, called from Zig.

Call Flow
---------
- `src/main.zig` calls `c.CreateSimpleWindow()` (imported via `@cImport`).
- `src/win32_ui.c` registers a window class, creates a window, shows it, and runs the message loop.
- `WndProc` handles messages such as `WM_PAINT` and `WM_DESTROY`.

Subsystem
---------
- The executable is built as a GUI app (`exe.subsystem = .Windows` in `build.zig`).
- A GUI subsystem app does not have a console attached; `printf` and Zig `std.debug.print` will not appear in a console window by default.

Window Class Registration
-------------------------
- Code uses the ANSI variant explicitly: `WNDCLASSEXA` + `RegisterClassExA`.
  - Fields set: `style`, `lpfnWndProc`, `hInstance`, `hIcon`, `hCursor`, `hbrBackground`, `lpszClassName`, `hIconSm`.
  - Background brush is set to `COLOR_WINDOW + 1` for a standard white background.
- Why ANSI explicit? It avoids surprises when `UNICODE` is (or isn’t) defined, making the build deterministic across environments.
- Mismatch gotcha: Using `WNDCLASSEX`/`RegisterClassEx` is correct; using `WNDCLASSEX` with `RegisterClass` (older API) can fail.

Creating the Window
-------------------
- Uses `CreateWindowExA` with:
  - Class name: the one registered above (`"SampleWindowClass"`).
  - Title: `"Zig + C Win32 Window"`.
  - Style: `WS_OVERLAPPEDWINDOW` (resizable, with caption and system menu).
  - Size/pos: `CW_USEDEFAULT` for position, `800x600` for size.
- On failure, calls a helper `ShowLastErrorA` which shows a `MessageBoxA` with `GetLastError()` code.

Showing and Painting
--------------------
- `ShowWindow(hwnd, SW_SHOW)` then `UpdateWindow(hwnd)` makes the window visible and triggers an initial paint.
- `WM_PAINT` handling in `WndProc`:
  - Calls `BeginPaint`/`EndPaint` to get a valid `HDC` and mark the update region.
  - Fills the background with `FillRect` using the window color brush.
  - Draws text: `TextOutA(hdc, 10, 10, text, (int)strlen(text))`.
  - Requires `#include <string.h>` for `strlen`.

Message Loop
------------
- Standard loop:
  - `while (GetMessageA(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessageA(&msg); }`
  - `GetMessageA` returns `0` when a `WM_QUIT` is retrieved, which breaks the loop.
- `WM_DESTROY` handler posts quit via `PostQuitMessage(0)`, causing the loop to exit.
- Return value: `CreateSimpleWindow()` returns `msg.wParam` as the process exit code.

Window Procedure (`WndProc`)
---------------------------
- Minimal handlers implemented:
  - `WM_DESTROY`: `PostQuitMessage(0)`
  - `WM_PAINT`: background fill + `TextOutA` of a static string
- All other messages pass to `DefWindowProc` for default handling.

Error Handling Strategy
-----------------------
- On class registration or window creation failure:
  - Show a modal `MessageBoxA` including the `GetLastError()` code to surface the issue in a GUI app.
- Typical failure causes:
  - Class not properly registered, wrong `RegisterClass*` call used, or missing required fields.
  - Using wide/ANSI API mismatches unintentionally.

ANSI vs UNICODE
---------------
- This project pins to the `A` (ANSI) API variants (`RegisterClassExA`, `CreateWindowExA`, `TextOutA`, etc.).
- To switch to wide-character APIs:
  - Use `WNDCLASSEXW`, `RegisterClassExW`, `CreateWindowExW`, `TextOutW`, wide string literals (L"..."), and `wcslen`.
  - Ensure your text data is `wchar_t*` and linked fonts support the characters you plan to render.

Linked Libraries
----------------
- `user32`: window creation, message pump, cursors, icons, message boxes, etc.
- `gdi32`: drawing text and simple 2D rendering (e.g., `TextOutA`).
- See `build-and-linking.md` for how these are linked in `build.zig`.

