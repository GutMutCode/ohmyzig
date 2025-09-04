#include "win_http.h"
#include <windows.h>
#include <wininet.h>
#include <stdlib.h>
#include <string.h>

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

static int ci_equal(char a, char b) {
    if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
    if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
    return a == b;
}

static int contains_ci(const char* haystack, const char* needle) {
    if (!haystack || !needle) return 0;
    size_t nlen = strlen(needle);
    if (nlen == 0) return 1;
    for (const char* p = haystack; *p; ++p) {
        size_t i = 0;
        while (needle[i] && p[i] && ci_equal(p[i], needle[i])) {
            ++i;
        }
        if (i == nlen) return 1;
    }
    return 0;
}

static int ensure_header_has_content_type_json(const char* extra, char** out_header) {
    // If extra already contains Content-Type (any case), just duplicate it; else append one.
    int has_ct = extra ? contains_ci(extra, "Content-Type:") : 0;
    size_t extra_len = extra ? (size_t)lstrlenA(extra) : 0;
    const char* default_ct = "Content-Type: application/json\r\n";
    size_t add_len = has_ct ? 0 : strlen(default_ct);
    char* hdr = (char*)malloc(extra_len + add_len + 1);
    if (!hdr) return 0;
    if (extra_len) memcpy(hdr, extra, extra_len);
    if (!has_ct) memcpy(hdr + extra_len, default_ct, add_len);
    hdr[extra_len + add_len] = '\0';
    *out_header = hdr;
    return 1;
}

int http_post_json_with_header(const char* url, const char* extra_header, const char* json_body,
                               char** out_buf, unsigned long* out_len) {
    if (!out_buf || !out_len || !url) return 0;
    *out_buf = NULL;
    *out_len = 0;

    HINTERNET hInternet = InternetOpenA("ohmyzig/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return 0;

    URL_COMPONENTSA comps = {0};
    char host[256] = {0};
    char path[1024] = {0};
    comps.dwStructSize = sizeof(comps);
    comps.lpszHostName = host; comps.dwHostNameLength = sizeof(host);
    comps.lpszUrlPath = path; comps.dwUrlPathLength = sizeof(path);
    if (!InternetCrackUrlA(url, 0, 0, &comps)) {
        InternetCloseHandle(hInternet);
        return 0;
    }

    INTERNET_PORT port = comps.nPort ? comps.nPort : INTERNET_DEFAULT_HTTPS_PORT;
    BOOL use_https = comps.nScheme == INTERNET_SCHEME_HTTPS;
    DWORD flags = use_https ? INTERNET_FLAG_SECURE : 0;

    HINTERNET hConnect = InternetConnectA(hInternet, host, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) { InternetCloseHandle(hInternet); return 0; }

    const char* verbs[] = { "POST" };
    HINTERNET hReq = HttpOpenRequestA(hConnect, verbs[0], path, NULL, NULL, NULL,
                                      flags | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
                                      0);
    if (!hReq) { InternetCloseHandle(hConnect); InternetCloseHandle(hInternet); return 0; }

    char* header_final = NULL;
    if (!ensure_header_has_content_type_json(extra_header, &header_final)) {
        InternetCloseHandle(hReq); InternetCloseHandle(hConnect); InternetCloseHandle(hInternet); return 0;
    }

    const void* body_ptr = json_body ? (const void*)json_body : (const void*)"";
    DWORD body_len = json_body ? (DWORD)lstrlenA(json_body) : 0;

    BOOL ok = HttpSendRequestA(hReq, header_final, (DWORD)lstrlenA(header_final), (LPVOID)body_ptr, body_len);
    free(header_final);
    if (!ok) {
        InternetCloseHandle(hReq); InternetCloseHandle(hConnect); InternetCloseHandle(hInternet);
        return 0;
    }

    const DWORD chunk = 16 * 1024;
    DWORD total = 0;
    char* buffer = NULL;
    for (;;) {
        char* newbuf = (char*)realloc(buffer, total + chunk);
        if (!newbuf) { free(buffer); InternetCloseHandle(hReq); InternetCloseHandle(hConnect); InternetCloseHandle(hInternet); return 0; }
        buffer = newbuf;
        DWORD read = 0;
        if (!InternetReadFile(hReq, buffer + total, chunk, &read)) { free(buffer); InternetCloseHandle(hReq); InternetCloseHandle(hConnect); InternetCloseHandle(hInternet); return 0; }
        total += read;
        if (read == 0) break;
    }

    if (buffer) {
        char* shrink = (char*)realloc(buffer, total);
        if (shrink) buffer = shrink;
    }

    InternetCloseHandle(hReq);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    *out_buf = buffer;
    *out_len = total;
    return 1;
}
