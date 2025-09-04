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

// Shows an informational message box with the given title and body.
void ShowInfoMessage(const char* title, const char* body);

// Shows a scrollable, read-only dialog with the given title and body text.
// Intended for long, multi-line content like model lists.
void ShowScrollableText(const char* title, const char* body);

// Sets the main window's read-only text area content (multiline).
void SetMainText(const char* body);

// Populates a combo box with model options from a newline-separated list.
// If the list is empty, clears the options.
void SetModelOptions(const char* newline_list);

// Prompts the user for the OpenAI API key using a simple modal input window.
// Writes a null-terminated string into out_buf (up to out_buf_len-1 chars).
// out_save receives 1 if user checked "Save key (encrypted)", else 0.
// Returns 1 on success (OK), 0 on cancel or error.
int PromptForApiKey(char* out_buf, int out_buf_len, int* out_save);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WIN32_UI_H
