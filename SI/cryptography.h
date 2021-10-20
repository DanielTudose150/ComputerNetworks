#ifndef CRYPTHOGRAPHY_H_GUARD
#define CRYPTHOGRAPHY_H_GUARD

#include <openssl/aes.h>
#include <string.h>

void encryptBlockECB(const char* input, char* output, const char* key);
void decryptBlockECB(const char* input, char* output, const char* key);

void xor(const char* in1, const char* in2, char* out);

void encryptInitialCFB(const char* input, char* output,const char* key, const char* iv);
void encryptCFB(const char* input, char* output, const char* key, const char* ciphertext);

void decryptCFB(const char* input, char* output, const char* key, const char* prevCiphertext);

#endif