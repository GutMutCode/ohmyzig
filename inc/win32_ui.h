// Lightweight Win32 UI helper used by Zig entrypoint.
// Exposes a simple function to create and show a window
// and run its message loop until quit.
#ifndef WIN32_UI_H
#define WIN32_UI_H

#ifdef __cplusplus
extern "C" {
#endif

// Creates a simple overlapped window, shows it, and runs the
// message loop. Returns the process exit code when the loop exits.
int CreateSimpleWindow(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WIN32_UI_H
