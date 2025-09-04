#include "win_secret.h"
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>

static int read_all(const char* path, unsigned char** out_buf, unsigned long* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return 0; }
    rewind(f);
    unsigned char* buf = (unsigned char*)malloc((size_t)sz);
    if (!buf) { fclose(f); return 0; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (n != (size_t)sz) { free(buf); return 0; }
    *out_buf = buf;
    *out_len = (unsigned long)sz;
    return 1;
}

int dpapi_encrypt_and_save(const char* path, const unsigned char* data, unsigned long len) {
    DATA_BLOB in = {0};
    in.pbData = (BYTE*)data;
    in.cbData = len;
    DATA_BLOB out = {0};
    // Optional entropy can be NULL; scope to current user by default
    if (!CryptProtectData(&in, L"ohmyzig", NULL, NULL, NULL, 0, &out)) {
        return 0;
    }
    int ok = 0;
    FILE* f = fopen(path, "wb");
    if (f) {
        size_t n = fwrite(out.pbData, 1, out.cbData, f);
        fclose(f);
        ok = (n == out.cbData) ? 1 : 0;
    }
    if (out.pbData) LocalFree(out.pbData);
    return ok;
}

int dpapi_load_and_decrypt(const char* path, unsigned char** out_buf, unsigned long* out_len) {
    if (!out_buf || !out_len) return 0;
    *out_buf = NULL;
    *out_len = 0;

    unsigned char* enc = NULL;
    unsigned long enc_len = 0;
    if (!read_all(path, &enc, &enc_len)) return 0;

    DATA_BLOB in = {0};
    in.pbData = enc;
    in.cbData = enc_len;
    DATA_BLOB out = {0};
    LPWSTR desc = NULL; // unused description
    BOOL ok = CryptUnprotectData(&in, &desc, NULL, NULL, NULL, 0, &out);
    if (desc) LocalFree(desc);
    free(enc);
    if (!ok) return 0;

    unsigned char* buf = (unsigned char*)malloc(out.cbData);
    if (!buf) { LocalFree(out.pbData); return 0; }
    memcpy(buf, out.pbData, out.cbData);
    *out_buf = buf;
    *out_len = out.cbData;
    LocalFree(out.pbData);
    return 1;
}

void secret_free(void* p) {
    if (p) free(p);
}

