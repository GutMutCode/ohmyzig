#include "win_http.h"
#include <windows.h>
#include <wininet.h>
#include <stdlib.h>

int http_get_with_header(const char* url, const char* extra_header, char** out_buf, unsigned long* out_len) {
    if (!out_buf || !out_len) return 0;
    *out_buf = NULL;
    *out_len = 0;

    HINTERNET hInternet = InternetOpenA("ohmyzig/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return 0;

    DWORD header_len = 0;
    if (extra_header) {
        header_len = (DWORD)lstrlenA(extra_header);
    }

    HINTERNET hUrl = InternetOpenUrlA(hInternet, url, extra_header, header_len, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return 0;
    }

    const DWORD chunk = 16 * 1024;
    DWORD total = 0;
    char* buffer = NULL;

    for (;;) {
        // Grow buffer
        char* newbuf = (char*)realloc(buffer, total + chunk);
        if (!newbuf) {
            free(buffer);
            InternetCloseHandle(hUrl);
            InternetCloseHandle(hInternet);
            return 0;
        }
        buffer = newbuf;

        DWORD read = 0;
        if (!InternetReadFile(hUrl, buffer + total, chunk, &read)) {
            free(buffer);
            InternetCloseHandle(hUrl);
            InternetCloseHandle(hInternet);
            return 0;
        }
        total += read;
        if (read == 0) break; // EOF
    }

    // Optionally shrink to exact size
    if (buffer) {
        char* shrink = (char*)realloc(buffer, total);
        if (shrink) buffer = shrink;
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    *out_buf = buffer;
    *out_len = total;
    return 1;
}

void http_free(void* p) {
    if (p) free(p);
}
