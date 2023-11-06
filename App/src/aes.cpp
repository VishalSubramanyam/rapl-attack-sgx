#include <cstdarg>
#include <emmintrin.h>
#include <stdexcept>
#include <wmmintrin.h>

#include "Enclave_u.h"
#include "aes.hpp"

#ifndef LOGGING_NEEDED
#define LOG(fmt, ...)
#else
#define LOG(fmt, ...) log(__FUNCTION__, fmt, ##__VA_ARGS__)
#endif
void log(const char *caller, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    printf("[%s] ", caller);
    vprintf(format, args);
    printf("\n"); // Print a new line after the log message

    va_end(args);
}

namespace customAES
{
std::vector<uint8_t>
aes128_encrypt_cbc_openssl(std::vector<uint8_t> const &plaintext,
                           const char key[AES_KEY_SIZE])
{
    AES_KEY aes_enc_key;
    AES_set_encrypt_key(reinterpret_cast<const uint8_t *>(key),
                        AES_BLOCK_SIZE * 8,
                        &aes_enc_key);

    std::array<uint8_t, AES_BLOCK_SIZE> iv;
    // set IV to 0
    iv.fill(0);
    // perform PKCS#7 padding on the plaintext
    int padding = AES_BLOCK_SIZE - plaintext.size() % AES_BLOCK_SIZE;
    std::vector<uint8_t> paddedPlaintext = plaintext;
    paddedPlaintext.insert(
        paddedPlaintext.end(), padding, static_cast<uint8_t>(padding));
    std::vector<uint8_t> ciphertext;
    ciphertext.resize(paddedPlaintext.size()); // Provide space for padding

    AES_cbc_encrypt(reinterpret_cast<const uint8_t *>(paddedPlaintext.data()),
                    ciphertext.data(),
                    paddedPlaintext.size(),
                    &aes_enc_key,
                    iv.data(),
                    AES_ENCRYPT);

    return ciphertext;
}
#define _generate_round_key(roundKeys, roundNumber, roundConstant)             \
    {                                                                          \
        __m128i previous_round_key = roundKeys[roundNumber - 1];               \
        __m128i &current_round_key = roundKeys[roundNumber];                   \
        auto temp1 =                                                           \
            _mm_aeskeygenassist_si128(previous_round_key, roundConstant);      \
        temp1 = _mm_shuffle_epi32(temp1, 0xFF);                                \
        auto temp2 = _mm_load_si128(&previous_round_key);                      \
        temp2 = _mm_slli_si128(temp2, 0x4);                                    \
        previous_round_key = _mm_xor_si128(temp2, previous_round_key);         \
        temp2 = _mm_slli_si128(temp2, 0x4);                                    \
        previous_round_key = _mm_xor_si128(temp2, previous_round_key);         \
        temp2 = _mm_slli_si128(temp2, 0x4);                                    \
        previous_round_key = _mm_xor_si128(temp2, previous_round_key);         \
        current_round_key = _mm_xor_si128(previous_round_key, temp1);          \
    };

// function that takes 128-bit key as input and returns 11 round keys
// through its second parameter
// Addresses needed to be 16-byte aligned (for speed)
void set_encrypt_key(__m128i const *key, __m128i *roundKeys)
{
    // round constants for key scheduling in AES
    constexpr uint8_t rcon[10] = {
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};
    // step 1: copy the input secret key to the first 16 bytes of the round keys
    roundKeys[0] = *key;
    // generate all the round keys
    _generate_round_key(roundKeys, 1, 0x01);
    _generate_round_key(roundKeys, 2, 0x02);
    _generate_round_key(roundKeys, 3, 0x04);
    _generate_round_key(roundKeys, 4, 0x08);
    _generate_round_key(roundKeys, 5, 0x10);
    _generate_round_key(roundKeys, 6, 0x20);
    _generate_round_key(roundKeys, 7, 0x40);
    _generate_round_key(roundKeys, 8, 0x80);
    _generate_round_key(roundKeys, 9, 0x1b);
    _generate_round_key(roundKeys, 10, 0x36);
    return;
}

std::vector<uint8_t>
aes128_encrypt_cbc_aesni(std::vector<uint8_t> const &plaintext,
                         char const key[AES_KEY_SIZE])
{
    // generate round keys
    __m128i roundKeys[11];
    set_encrypt_key(reinterpret_cast<__m128i const *>(key), roundKeys);
    // perform AES encryption

    // perform PKCS#7 padding on the plaintext
    int padding = AES_BLOCK_SIZE - plaintext.size() % AES_BLOCK_SIZE;
    std::vector<uint8_t> paddedPlaintext = plaintext;
    paddedPlaintext.insert(
        paddedPlaintext.end(), padding, static_cast<uint8_t>(padding));

    std::vector<uint8_t> ciphertext;
    ciphertext.resize(paddedPlaintext.size()); // Provide space for padding

    __m128i *plaintextBlocks =
        reinterpret_cast<__m128i *>(paddedPlaintext.data());
    __m128i *ciphertextBlocks = reinterpret_cast<__m128i *>(ciphertext.data());

    // print padding info
    LOG("Padding size: %d", padding);

    // perform AES encryption
    __m128i emptyIV = _mm_setzero_si128();
    __m128i *initVec = &emptyIV;
    for (int i = 0; i < paddedPlaintext.size() / AES_BLOCK_SIZE; i++) {
        ciphertextBlocks[i] = _mm_xor_si128(plaintextBlocks[i], roundKeys[0]);
        ciphertextBlocks[i] = _mm_xor_si128(ciphertextBlocks[i], *initVec);
        for (int j = 1; j < 10; j++) {
            ciphertextBlocks[i] =
                _mm_aesenc_si128(ciphertextBlocks[i], roundKeys[j]);
        }
        ciphertextBlocks[i] =
            _mm_aesenclast_si128(ciphertextBlocks[i], roundKeys[10]);
        initVec = &ciphertextBlocks[i];
    }
    return ciphertext;
}
} // namespace customAES