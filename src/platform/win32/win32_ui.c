// Win32 UI helper implementation.
// Creates a basic overlapped window and draws a line of text.
// Notes:
//  - Uses explicit ANSI versions of Win32 APIs (RegisterClassExA, TextOutA, ...)
//    to avoid UNICODE macro surprises when building in different environments.
//  - Shows simple MessageBoxA errors if class registration or window creation fails.
#include "win32_ui.h"
#include "app_callbacks.h"
#include <windows.h>
#include <string.h>

// Window text constants
static const char WINDOW_CLASS[] = "SampleWindowClass";
static const char WINDOW_TITLE[] = "Zig + C Win32 Window";

// Control IDs
#define IDC_BTN_UPDATE      2001
#define IDC_BTN_SEND        2002
#define IDC_MODEL_COMBO     2003
#define IDC_INPUT_EDIT      3000
#define IDC_TEXTVIEW_EDIT   3001
#define IDC_MAIN_EDIT       3002

static HWND g_main_edit = NULL;
static HWND g_input_edit = NULL;
static HWND g_model_combo = NULL;

// Helper: format and show last-error details
static void ShowLastErrorA(const char* title_prefix);
static void CenterWindow(HWND hwnd);
static void ModalLoopUntilDestroyed(HWND hwnd);

// Basic window procedure: handles paint and quit.
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            HINSTANCE hi = ((LPCREATESTRUCTA)lParam)->hInstance;
            // Model selection combo
            g_model_combo = CreateWindowExA(0, "COMBOBOX", "",
                                           WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                           10, 10, 300, 300,
                                           hwnd, (HMENU)(INT_PTR)IDC_MODEL_COMBO,
                                           hi, NULL);

            CreateWindowExA(0, "BUTTON", "Update Model List",
                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                            320, 10, 180, 28,
                            hwnd, (HMENU)(INT_PTR)IDC_BTN_UPDATE,
                            hi, NULL);
            // Chat input + Send button
            g_input_edit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                           WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                           200, 44, 490, 24,
                                           hwnd, (HMENU)(INT_PTR)IDC_INPUT_EDIT, hi, NULL);
            CreateWindowExA(0, "BUTTON", "Send",
                            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                            695, 44, 80, 28,
                            hwnd, (HMENU)(INT_PTR)IDC_BTN_SEND,
                            hi, NULL);
            // Multiline read-only text area to show model list
            g_main_edit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                          WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
                                          10, 80, 760, 460,
                                          hwnd, (HMENU)(INT_PTR)IDC_MAIN_EDIT, hi, NULL);
            // Populate on startup (cached first, else online)
            OnShowModelsRequest();
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_COMMAND: {
            const int id = LOWORD(wParam);
            if (id == IDC_BTN_UPDATE) {
                OnShowModelsRequest();
                return 0;
            }
            if (id == IDC_BTN_SEND) {
                char buf[2048];
                if (g_input_edit) {
                    GetWindowTextA(g_input_edit, buf, (int)sizeof(buf));
                    if (buf[0] != '\0') {
                        char model_buf[256] = {0};
                        if (g_model_combo) {
                            LRESULT sel = SendMessageA(g_model_combo, CB_GETCURSEL, 0, 0);
                            if (sel != CB_ERR) {
                                SendMessageA(g_model_combo, CB_GETLBTEXT, (WPARAM)sel, (LPARAM)model_buf);
                            }
                        }
                        if (model_buf[0] != '\0')
                            OnSendChatRequestWithModel(buf, model_buf);
                        else
                            OnSendChatRequest(buf);
                    }
                }
                return 0;
            }
            break;
        }
        case WM_SIZE: {
            RECT rc; GetClientRect(hwnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            if (g_main_edit) {
                MoveWindow(g_main_edit, 10, 80, w - 20, (h - 90), TRUE);
            }
            if (g_input_edit) {
                int send_w = 80;
                int margin = 10;
                int input_x = 200;
                int input_w = w - input_x - margin - send_w - margin;
                if (input_w < 120) input_w = 120;
                MoveWindow(g_input_edit, input_x, 44, input_w, 24, TRUE);
                HWND hSend = GetDlgItem(hwnd, IDC_BTN_SEND);
                if (hSend) MoveWindow(hSend, input_x + input_w + margin, 44, send_w, 28, TRUE);
            }
            if (g_model_combo) {
                MoveWindow(g_model_combo, 10, 10, 300, 300, TRUE);
                HWND hUpdate = GetDlgItem(hwnd, IDC_BTN_UPDATE);
                if (hUpdate) MoveWindow(hUpdate, 320, 10, 180, 28, TRUE);
            }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

// Show the last Win32 error in a message box with system message text.
static void ShowLastErrorA(const char* title_prefix) {
    DWORD err = GetLastError();
    LPSTR msg = NULL;
    DWORD len = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);
    char title[256];
    wsprintfA(title, "%s (0x%08lX)", title_prefix, (unsigned long)err);
    if (len && msg) {
        MessageBoxA(NULL, msg, title, MB_OK | MB_ICONERROR);
        LocalFree(msg);
    } else {
        MessageBoxA(NULL, title, "Win32 Error", MB_OK | MB_ICONERROR);
    }
}

int CreateSimpleWindow(void) {
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandleA(NULL);

    WNDCLASSEXA wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIconA(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = WINDOW_CLASS;
    wc.hIconSm       = LoadIconA(NULL, IDI_APPLICATION);

    if (!RegisterClassExA(&wc)) {
        ShowLastErrorA("RegisterClassExA failed");
        return 0;
    }

    HWND hwnd = CreateWindowExA(
        0,
        WINDOW_CLASS,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        ShowLastErrorA("CreateWindowExA failed");
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}

void ShowInfoMessage(const char* title, const char* body) {
    MessageBoxA(NULL, body, title, MB_OK | MB_ICONINFORMATION);
}

void SetMainText(const char* body) {
    if (g_main_edit) {
        SetWindowTextA(g_main_edit, body ? body : "");
    } else {
        // Fallback if not ready
        MessageBoxA(NULL, body ? body : "", "Models", MB_OK | MB_ICONINFORMATION);
    }
}

void SetModelOptions(const char* newline_list) {
    if (!g_model_combo) return;
    SendMessageA(g_model_combo, CB_RESETCONTENT, 0, 0);
    if (!newline_list || !newline_list[0]) return;
    const char* p = newline_list;
    const char* line = p;
    char item[256];
    while (*p) {
        if (*p == '\n') {
            size_t len = (size_t)(p - line);
            if (len > 0) {
                // Trim trailing CR
                if (line[len - 1] == '\r') len -= 1;
                if (len > 0) {
                    size_t cpy = len < sizeof(item) - 1 ? len : sizeof(item) - 1;
                    memcpy(item, line, cpy);
                    item[cpy] = '\0';
                    SendMessageA(g_model_combo, CB_ADDSTRING, 0, (LPARAM)item);
                }
            }
            p += 1;
            line = p;
        } else {
            p += 1;
        }
    }
    // Last line (no trailing newline)
    if (p != line) {
        size_t len = (size_t)(p - line);
        if (len > 0) {
            if (line[len - 1] == '\r') len -= 1;
            if (len > 0) {
                size_t cpy = len < sizeof(item) - 1 ? len : sizeof(item) - 1;
                memcpy(item, line, cpy);
                item[cpy] = '\0';
                SendMessageA(g_model_combo, CB_ADDSTRING, 0, (LPARAM)item);
            }
        }
    }
    // Select first item if any
    if (SendMessageA(g_model_combo, CB_GETCOUNT, 0, 0) > 0) {
        SendMessageA(g_model_combo, CB_SETCURSEL, 0, 0);
    }
}

// --- Scrollable text dialog ---
typedef struct TextViewState {
    const char* title;
    const char* body;
    HWND hEdit;
} TextViewState;

static LRESULT CALLBACK TextWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    TextViewState* st = (TextViewState*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTA* cs = (CREATESTRUCTA*)lParam;
            st = (TextViewState*)cs->lpCreateParams;
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)st);

            // Multiline, read-only edit with vertical scrollbar
            st->hEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
                                        10, 10, 560, 320, hwnd, (HMENU)IDC_TEXTVIEW_EDIT, cs->hInstance, NULL);
            // Set text
            if (st && st->body) SetWindowTextA(st->hEdit, st->body);

            // OK button
            CreateWindowExA(0, "BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                            500, 340, 70, 24, hwnd, (HMENU)IDOK, cs->hInstance, NULL);
            return 0;
        }
        case WM_SIZE: {
            // Resize edit area to fill client rect with margins
            RECT rc; GetClientRect(hwnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            if (st && st->hEdit) {
                MoveWindow(st->hEdit, 10, 10, w - 20, h - 20 - 30, TRUE);
            }
            // Move OK button to bottom-right
            HWND hOk = GetDlgItem(hwnd, IDOK);
            if (hOk) MoveWindow(hOk, w - 10 - 70, h - 10 - 24, 70, 24, TRUE);
            return 0;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDOK) {
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void ShowScrollableText(const char* title, const char* body) {
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandleA(NULL);
    const char CLASS_NAME[] = "OhMyZigTextView";
    static ATOM g_text_class_atom = 0;
    if (!g_text_class_atom) {
        WNDCLASSEXA wc = {0};
        wc.cbSize        = sizeof(WNDCLASSEXA);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = TextWndProc;
        wc.hInstance     = hInstance;
        wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = CLASS_NAME;
        g_text_class_atom = RegisterClassExA(&wc);
        if (!g_text_class_atom && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            ShowLastErrorA("Register text view class failed");
            return;
        }
    }

    TextViewState st = {0};
    st.title = title;
    st.body = body;
    st.hEdit = NULL;

    HWND hwnd = CreateWindowExA(0, CLASS_NAME, title ? title : "",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
                                CW_USEDEFAULT, CW_USEDEFAULT, 600, 420,
                                NULL, NULL, hInstance, &st);
    if (!hwnd) return;

    CenterWindow(hwnd);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    ModalLoopUntilDestroyed(hwnd);
}

// --- Simple modal input window for API key ---
typedef struct InputState {
    char* out;
    int out_len;
    int result; // 1 = OK, 0 = Cancel
    HWND hEdit;
    HWND hCheck;
    int* out_save;
} InputState;

static LRESULT CALLBACK InputWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    InputState* st = (InputState*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTA* cs = (CREATESTRUCTA*)lParam;
            st = (InputState*)cs->lpCreateParams;
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)st);

            CreateWindowExA(0, "STATIC", "Enter OpenAI API Key:", WS_CHILD | WS_VISIBLE,
                            10, 10, 360, 20, hwnd, NULL, cs->hInstance, NULL);
            st->hEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_PASSWORD,
                                        10, 35, 360, 24, hwnd, (HMENU)1001, cs->hInstance, NULL);
            st->hCheck = CreateWindowExA(0, "BUTTON", "Save key (encrypted)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                         10, 70, 160, 20, hwnd, (HMENU)1002, cs->hInstance, NULL);
            CreateWindowExA(0, "BUTTON", "OK", WS_CHILD | WS_VISIBLE,
                            200, 70, 80, 24, hwnd, (HMENU)IDOK, cs->hInstance, NULL);
            CreateWindowExA(0, "BUTTON", "Cancel", WS_CHILD | WS_VISIBLE,
                            290, 70, 80, 24, hwnd, (HMENU)IDCANCEL, cs->hInstance, NULL);
            return 0;
        }
        case WM_COMMAND: {
            const int id = LOWORD(wParam);
            if (id == IDOK) {
                if (st && st->out && st->out_len > 0) {
                    GetWindowTextA(st->hEdit, st->out, st->out_len);
                    if (st->out_save) {
                        LRESULT checked = SendMessageA(st->hCheck, BM_GETCHECK, 0, 0);
                        *st->out_save = (checked == BST_CHECKED) ? 1 : 0;
                    }
                    st->result = 1;
                }
                DestroyWindow(hwnd);
                return 0;
            }
            if (id == IDCANCEL) {
                st->result = 0;
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
        case WM_CLOSE:
            st->result = 0;
            DestroyWindow(hwnd);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int PromptForApiKey(char* out_buf, int out_buf_len, int* out_save) {
    if (!out_buf || out_buf_len <= 1) return 0;

    HINSTANCE hInstance = (HINSTANCE)GetModuleHandleA(NULL);
    const char CLASS_NAME[] = "OhMyZigInputClass";
    static ATOM g_input_class_atom = 0;
    if (!g_input_class_atom) {
        WNDCLASSEXA wc = {0};
        wc.cbSize        = sizeof(WNDCLASSEXA);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = InputWndProc;
        wc.hInstance     = hInstance;
        wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = CLASS_NAME;
        g_input_class_atom = RegisterClassExA(&wc);
        if (!g_input_class_atom && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            ShowLastErrorA("Register input class failed");
            return 0;
        }
    }

    InputState st = {0};
    st.out = out_buf;
    st.out_len = out_buf_len;
    st.result = 0;
    st.hEdit = NULL;
    st.hCheck = NULL;
    st.out_save = out_save;

    HWND hwnd = CreateWindowExA(0, CLASS_NAME, "OpenAI Authentication",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                CW_USEDEFAULT, CW_USEDEFAULT, 400, 140,
                                NULL, NULL, hInstance, &st);
    if (!hwnd) return 0;

    CenterWindow(hwnd);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    ModalLoopUntilDestroyed(hwnd);

    return st.result;
}

// --- Helpers ---
static void CenterWindow(HWND hwnd) {
    RECT rc; GetWindowRect(hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

static void ModalLoopUntilDestroyed(HWND hwnd) {
    MSG msg;
    while (IsWindow(hwnd) && GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}
