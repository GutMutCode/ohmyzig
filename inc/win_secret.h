// Windows DPAPI helpers to encrypt/decrypt secrets to/from a file.
#ifndef WIN_SECRET_H
#define WIN_SECRET_H

#ifdef __cplusplus
extern "C" {
#endif

// Encrypts (CryptProtectData) the given memory and writes it to a file at path.
// Returns 1 on success, 0 on failure.
int dpapi_encrypt_and_save(const char* path, const unsigned char* data, unsigned long len);

// Loads file at path and decrypts (CryptUnprotectData) to an allocated buffer.
// On success returns 1 and sets *out_buf/*out_len. Caller must free with secret_free().
int dpapi_load_and_decrypt(const char* path, unsigned char** out_buf, unsigned long* out_len);

// Frees buffers allocated by dpapi_load_and_decrypt.
void secret_free(void* p);

#ifdef __cplusplus
}
#endif

#endif // WIN_SECRET_H

