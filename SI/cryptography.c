#include "cryptography.h"

void encryptBlockECB(const char* input, char* output,const char* key)
{
    AES_KEY aesKey;
    AES_set_encrypt_key(key,128,&aesKey);
    AES_encrypt(input,output,&aesKey);
}

void decryptBlockECB(const char* input, char* output, const char* key)
{
    AES_KEY aesKey;
    AES_set_decrypt_key(key,128,&aesKey);
    AES_decrypt(input,output,&aesKey);
}

void xor(const char* in1, const char* in2, char* out)
{
    for(int i = strlen(in1) - 1; i >= 0; --i)
        out[i] = in1[i] ^ in2[i];
}

void encryptInitialCFB(const char* input, char* output,const char* key, const char* iv)
{
    AES_KEY aesKey;
    AES_set_encrypt_key(key,128,&aesKey);
    char block[17];
    AES_encrypt(iv,block,&aesKey);
    xor(input,block,output);
}

void encryptCFB(const char* input, char* output, const char* key, const char* ciphertext)
{
    AES_KEY aesKey;
    AES_set_encrypt_key(key,128,&aesKey);
    char block[17];
    AES_encrypt(ciphertext,block,&aesKey);
    xor(input,block,output);
}

void decryptCFB(const char* input, char* output, const char* key, const char* prevCiphertext)
{
    AES_KEY aesKey;
    AES_set_encrypt_key(key,128,&aesKey);
    char block[17];
    AES_encrypt(prevCiphertext,block,&aesKey);
    xor(input,block,output);
}