// Win32 UI helper implementation.
// Creates a basic overlapped window and draws a line of text.
// Notes:
//  - Uses explicit ANSI versions of Win32 APIs (RegisterClassExA, TextOutA, ...)
//    to avoid UNICODE macro surprises when building in different environments.
//  - Shows simple MessageBoxA errors if class registration or window creation fails.
#include "win32_ui.h"
#include <windows.h>
#include <string.h>

// 윈도우 프로시져: 윈도우에서 발생하는 이벤트(메시지)를 처리하는 함수
// Basic window procedure: handles paint and quit.
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
            const char* text = "Hello, Zig + C World!";
            TextOutA(hdc, 10, 10, text, (int)strlen(text));
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Show the last Win32 error in a message box (best-effort).
static void ShowLastErrorA(const char* title_prefix) {
    DWORD err = GetLastError();
    char buf[256];
    wsprintfA(buf, "%s (GetLastError=%lu)", title_prefix, (unsigned long)err);
    MessageBoxA(NULL, buf, "Win32 Error", MB_OK | MB_ICONERROR);
}

int CreateSimpleWindow(void) {
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandleA(NULL);
    const char CLASS_NAME[] = "SampleWindowClass";

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
    wc.lpszClassName = CLASS_NAME;
    wc.hIconSm       = LoadIconA(NULL, IDI_APPLICATION);

    if (!RegisterClassExA(&wc)) {
        ShowLastErrorA("RegisterClassExA failed");
        return 0;
    }

    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "Zig + C Win32 Window",
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

    // 메시지 루프
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
