#include <array>
#include <cstdio>
#include <string>
#include <vector>

#include <openssl/aes.h>

#include "Enclave.h"
#include "Enclave_t.h"

static int constexpr LOOP_ITER = 1e7;

/*
 * printf:
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
    return 0;
}

void aes_openssl_sgx(uint8_t const *plaintext,
                     int ptSize,
                     uint8_t *ciphertext,
                     char const *key)
{
    AES_KEY aes_enc_key;
    AES_set_encrypt_key(reinterpret_cast<const uint8_t *>(key),
                        AES_BLOCK_SIZE * 8,
                        &aes_enc_key);

    std::array<uint8_t, AES_BLOCK_SIZE> iv;
    // set IV to 0
    iv.fill(0);
    // perform PKCS#7 padding on the plaintext
    int padding = AES_BLOCK_SIZE - (ptSize % AES_BLOCK_SIZE);
    std::vector<uint8_t> paddedPlaintext(plaintext, plaintext + ptSize);
    paddedPlaintext.insert(
        paddedPlaintext.end(), padding, static_cast<uint8_t>(padding));
    AES_cbc_encrypt(static_cast<const uint8_t *>(paddedPlaintext.data()),
                    ciphertext,
                    paddedPlaintext.size(),
                    &aes_enc_key,
                    iv.data(),
                    AES_ENCRYPT);
    // drop the return value, we don't need it
    // We just want to test encryption inside the enclave
}

void aes_openssl_keyFixed_ptFixed_sgx_looped(uint8_t const *plaintext,
                                             int ptSize,
                                             uint8_t *ciphertext,
                                             char const *key)
{
    for (int i = 0; i < LOOP_ITER; i++) {
        aes_openssl_sgx(plaintext, ptSize, ciphertext, key);
    }
}

void aes_openssl_keyVaries_ptFixed_sgx_looped(uint8_t const *plaintext,
                                              int ptSize,
                                              uint8_t *ciphertext,
                                              char pregeneratedKeys[][16])
{
    for (int i = 0; i < LOOP_ITER; i++) {
        aes_openssl_sgx(plaintext, ptSize, ciphertext, pregeneratedKeys[i]);
    }
}

void initialize_enclave() { printf("%s", "Enclave initialized\n"); }

void run_computations_enclave() {}