// Minimal WinINet HTTP GET helper for Windows GUI app.
#ifndef WIN_HTTP_H
#define WIN_HTTP_H

#ifdef __cplusplus
extern "C" {
#endif

// Performs HTTP GET to the given URL with optional extra header string
// (e.g., "Authorization: Bearer ...\r\n").
// On success returns 1 and sets *out_buf/*out_len. Caller must free via http_free.
int http_get_with_header(const char* url, const char* extra_header, char** out_buf, unsigned long* out_len);

// Performs HTTP POST with a JSON body. `extra_header` can include additional
// headers (e.g., Authorization). This function automatically sets
// "Content-Type: application/json" if not provided.
// On success returns 1 and sets *out_buf/*out_len. Caller must free via http_free.
int http_post_json_with_header(const char* url, const char* extra_header, const char* json_body,
                               char** out_buf, unsigned long* out_len);

// Frees buffer allocated by http_get_with_header.
void http_free(void* p);

#ifdef __cplusplus
}
#endif

#endif // WIN_HTTP_H
