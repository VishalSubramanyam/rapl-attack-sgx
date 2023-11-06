#pragma once
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <wmmintrin.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "sgx_urts.h"

#define ALIGNED16 __attribute__((aligned(16)))
#define AES_KEY_SIZE 16

namespace customAES
{
std::vector<uint8_t>
aes128_encrypt_cbc_openssl(const std::vector<uint8_t> &plaintext,
                           const char key[AES_KEY_SIZE]);

// Takes 128-bit key as input and returns 11 round keys
// through its second parameter
// Addresses needed to be 16-byte aligned (for speed)
void set_encrypt_key(__m128i const *key, __m128i *roundKeys);

std::vector<uint8_t>
aes128_encrypt_cbc_aesni(const std::vector<uint8_t> &plaintext,
                         const char key[AES_KEY_SIZE]);

// SGX Enclave Version
// T-table OpenSSL 3.0.x version


template<bool LOOPED_INSIDE>
std::vector<uint8_t>
aes128_encrypt_cbc_openssl_sgx(std::vector<uint8_t> const &plaintext,
                               char const key[AES_KEY_SIZE],
                               char pregeneratedKeys[][AES_KEY_SIZE],
                               sgx_enclave_id_t const eid)
{
    // call enclave ecall aes_openssl_sgx
    // calculate the size of ciphertext based on padded plaintext size
    auto ciphertextSize =
        plaintext.size() +
        (AES_BLOCK_SIZE - (plaintext.size() % AES_BLOCK_SIZE));
    std::vector<uint8_t> ciphertext(ciphertextSize);
    sgx_status_t ret;
    if constexpr (!LOOPED_INSIDE) {
        ret = aes_openssl_sgx(
            eid, plaintext.data(), plaintext.size(), ciphertext.data(), key);
    } else {
        if (pregeneratedKeys == nullptr) {
            ret = aes_openssl_keyFixed_ptFixed_sgx_looped(
                eid, plaintext.data(), plaintext.size(), ciphertext.data(), key);
        } else {
            ret = aes_openssl_keyVaries_ptFixed_sgx_looped(
                eid, plaintext.data(), plaintext.size(), ciphertext.data(), pregeneratedKeys);
        }
    }
    // check ret sgx_status
    if (ret != SGX_SUCCESS) {
        throw std::runtime_error(
            "aes_openssl_sgx failed inside aes128_encrypt_cbc_openssl_sgx");
    }
    return ciphertext;
}

} // namespace customAES
