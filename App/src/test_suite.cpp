#include <immintrin.h>
#include <stdio.h>

#include <chrono>
#include <vector>

#include "aes.hpp"
#include "computations.hpp"

// test if key scheduling works correctly against OpenSSL's AES_set_encrypt_key
// function
bool testKeyScheduling()
{
    // generate a random 128-bit key
    __m128i key = _mm_setzero_si128();
    uint8_t *keyPtr = reinterpret_cast<uint8_t *>(&key);
    RAND_bytes(keyPtr, AES_BLOCK_SIZE);
    // generate rounds keys, both using OpenSSL and custom code
    __m128i roundKeys[11];
    customAES::set_encrypt_key(&key, roundKeys);
    AES_KEY opensslKey;
    AES_set_encrypt_key(keyPtr, AES_BLOCK_SIZE * 8, &opensslKey);
    // compare the two
    for (int i = 0; i < 11; i++) {
        uint32_t customWord0 = _mm_extract_epi32(roundKeys[i], 0);
        uint32_t customWord1 = _mm_extract_epi32(roundKeys[i], 1);
        uint32_t customWord2 = _mm_extract_epi32(roundKeys[i], 2);
        uint32_t customWord3 = _mm_extract_epi32(roundKeys[i], 3);
        uint32_t opensslWord0 = opensslKey.rd_key[4 * i];
        uint32_t opensslWord1 = opensslKey.rd_key[4 * i + 1];
        uint32_t opensslWord2 = opensslKey.rd_key[4 * i + 2];
        uint32_t opensslWord3 = opensslKey.rd_key[4 * i + 3];
        if (customWord0 != opensslWord0 || customWord1 != opensslWord1 ||
            customWord2 != opensslWord2 || customWord3 != opensslWord3) {
            return false;
        }
    }
    return true;
}

// test AES encryption against OpenSSL's AES_cbc_encrypt function
bool testAESEncryption()
{
    std::string plaintext = "This is a plaintext";
    char key[AES_KEY_SIZE] = "123456789123456";
    // encrypt using OpenSSL's AES_cbc_encrypt function
    auto openSSLResult = customAES::aes128_encrypt_cbc_openssl(
        std::vector<uint8_t>(plaintext.begin(), plaintext.end()), key);
    auto customResult = customAES::aes128_encrypt_cbc_aesni(
        std::vector<uint8_t>(plaintext.begin(), plaintext.end()), key);
    // check if the results are the same
    if (openSSLResult.size() != customResult.size()) {
        return false;
    }
    for (int i = 0; i < openSSLResult.size(); i++) {
        if (openSSLResult[i] != customResult[i]) {
            // print both ciphertexts
            printf("OpenSSL ciphertext: ");
            for (int j = 0; j < openSSLResult.size(); j++) {
                printf("%02x", openSSLResult[j]);
            }
            printf("\n");
            printf("Custom ciphertext:  ");
            for (int j = 0; j < customResult.size(); j++) {
                printf("%02x", customResult[j]);
            }
            printf("\n");
            return false;
        }
    }
    return true;
}

// test the SGX encryption function I wrote inside the enclave
// compare results against OpenSSL's AES_cbc_encrypt function
bool testSGX_AESEncryption(sgx_enclave_id_t eid)
{
    std::string plaintext = "This is a plaintext";
    char key[AES_KEY_SIZE] = "123456789123456";
    // encrypt using OpenSSL's AES_cbc_encrypt function
    auto openSSLResult = customAES::aes128_encrypt_cbc_openssl(
        std::vector<uint8_t>(plaintext.begin(), plaintext.end()), key);
    auto customResult = customAES::aes128_encrypt_cbc_openssl_sgx(
        std::vector<uint8_t>(plaintext.begin(), plaintext.end()), key, eid);
    // check if the results are the same
    if (openSSLResult.size() != customResult.size()) {
        return false;
    }
    for (int i = 0; i < openSSLResult.size(); i++) {
        if (openSSLResult[i] != customResult[i]) {
            // print both ciphertexts
            printf("OpenSSL ciphertext: ");
            for (int j = 0; j < openSSLResult.size(); j++) {
                printf("%02x", openSSLResult[j]);
            }
            printf("\n");
            printf("Custom ciphertext:  ");
            for (int j = 0; j < customResult.size(); j++) {
                printf("%02x", customResult[j]);
            }
            printf("\n");
            return false;
        }
    }
    return true;
}

int main()
{
    // test key scheduling
    // if (!testKeyScheduling()) {
    //     printf("Key scheduling test failed\n");
    //     return 1;
    // }
    // printf("Key scheduling test passed\n");
    // test AES encryption
    if (!testAESEncryption()) {
        printf("AES encryption test failed\n");
        return 1;
    }
    printf(
        "AES-NI encryption code works correctly (compared against OpenSSL)\n");
    // test SGX AES encryption
    // create enclave
    sgx_enclave_id_t eid;
    sgx_status_t ret = SGX_SUCCESS;
    ret = sgx_create_enclave(
        "enclave.signed.so", SGX_DEBUG_FLAG, NULL, NULL, &eid, NULL);
    if (ret != SGX_SUCCESS) {
        printf("Failed to create enclave\n");
        return 1;
    }

    if (!testSGX_AESEncryption(eid)) {
        printf("SGX AES encryption test failed\n");
        return 1;
    }
    printf("SGX AES encryption test passed\n");
    return 0;
}
