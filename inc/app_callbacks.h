#ifndef APP_CALLBACKS_H
#define APP_CALLBACKS_H

#ifdef __cplusplus
extern "C" {
#endif

// Called from the Win32 UI when the user clicks
// the "Show OpenAI Models" button.
void OnShowModelsRequest(void);

// Called when the user clicks the "Send" button in the chat UI.
// message is a null-terminated ANSI string.
void OnSendChatRequest(const char* message);

// Called when the user clicks "Send"; includes selected model id (may be empty).
void OnSendChatRequestWithModel(const char* message, const char* model);

#ifdef __cplusplus
}
#endif

#endif // APP_CALLBACKS_H
