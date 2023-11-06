#include "computations.hpp"

#include <fcntl.h>
#include <immintrin.h>
#include <openssl/aes.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <bit>
#include <iostream>
using std::cout, std::endl;

#include "Enclave_u.h"
#include "aes.hpp"



static int constexpr LOOP_ITER = 1e7;
static uint32_t constexpr LONG_LOOP_ITER = 4e9;
static char pregeneratedKeys[LOOP_ITER][16];
void Computation::init(
    const time_point &spyStart,
    uint samplingInt,
    sgx_enclave_id_t eid
)
{
    spyStartTime = spyStart;
    samplingInterval = samplingInt;
    isInitialized = true;
    Computation::eid = eid;
    // fill in pregenerated keys
    int fd = open("/dev/random", O_RDONLY);
    if (read(fd, pregeneratedKeys, sizeof(pregeneratedKeys)) < 0) {
        throw std::runtime_error(
            "Error reading from /dev/random" + std::string(strerror(errno))
        );
    }
    close(fd);
}

void emptyComputation()
{
    for (int i = 0; i < 100 * LOOP_ITER; i++) {
        // do nothing
    }
}
void aesOpenSSLComputation()
{
    // perform AES computation using the function from aes.hpp
    // use some plaintext and some secret key
    std::string plaintext = "This is a plaintext";
    char key[AES_KEY_SIZE] = "123456789123456";
    for (int i = 0; i < LOOP_ITER; i++) {
        std::vector<uint8_t> ciphertext = customAES::aes128_encrypt_cbc_openssl(
            std::vector<uint8_t>(plaintext.begin(), plaintext.end()), key
        );
    }
}

void aesniComputation()
{
    std::string plaintext = "This is a plaintext";
    char key[AES_KEY_SIZE] = "123456789123456";
    for (int i = 0; i < LOOP_ITER; i++) {
        std::vector<uint8_t> ciphertext = customAES::aes128_encrypt_cbc_aesni(
            std::vector<uint8_t>(plaintext.begin(), plaintext.end()), key
        );
    }
}

void aesniKeyFixedPtFixed()
{
    char key[AES_KEY_SIZE] = "123456789123456";
    std::string plaintext = "123456789123456";
    for (int i = 0; i < LOOP_ITER; i++) {
        std::vector<uint8_t> ciphertext = customAES::aes128_encrypt_cbc_aesni(
            std::vector<uint8_t>(plaintext.begin(), plaintext.end()), key
        );
    }
}

void aesniKeyVariesPtFixed()
{
    std::string plaintext = "123456789123456";
    for (int i = 0; i < LOOP_ITER; i++) {
        std::vector<uint8_t> ciphertext = customAES::aes128_encrypt_cbc_aesni(
            std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
            pregeneratedKeys[i]
        );
    }
}

void aesOpenSSLKeyFixedPtFixed()
{
    char key[AES_KEY_SIZE] = "123456789123456";
    std::string plaintext = "123456789123456";
    for (int i = 0; i < LOOP_ITER; i++) {
        std::vector<uint8_t> ciphertext = customAES::aes128_encrypt_cbc_openssl(
            std::vector<uint8_t>(plaintext.begin(), plaintext.end()), key
        );
    }
}

void aesOpenSSLKeyVariesPtFixed()
{
    std::string plaintext = "123456789123456";
    for (int i = 0; i < LOOP_ITER; i++) {
        std::vector<uint8_t> ciphertext = customAES::aes128_encrypt_cbc_openssl(
            std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
            pregeneratedKeys[i]
        );
    }
}

void aesniCPA()
{
    std::string key = "123456789123456";
    int const NUM_PLAINTEXTS = 1e6;
    std::vector<std::string> plaintexts(NUM_PLAINTEXTS, std::string(16, '\0'));
    auto fd = open("/dev/random", O_RDONLY);
    for (int i = 0; i < NUM_PLAINTEXTS; i++) {
        std::ignore = read(fd, plaintexts[i].data(), 16);
    }
    std::vector<std::vector<uint8_t>> ciphertexts;
    close(fd);
    for (int i = 0; i < NUM_PLAINTEXTS; i++) {
        std::vector<uint8_t> ciphertext = customAES::aes128_encrypt_cbc_aesni(
            std::vector<uint8_t>(
                plaintexts[i].data(), plaintexts[i].data() + 16
            ),
            key.c_str()
        );
        ciphertexts.push_back(std::move(ciphertext));
    }
    // put the plaintexts and ciphertexts in a file
    std::ofstream fout("aesniCPA.bin", std::ios::binary);
    fout.write(key.data(), key.size());
    for (int i = 0; i < NUM_PLAINTEXTS; i++) {
        fout.write(
            reinterpret_cast<char const *>(ciphertexts[i].data()),
            ciphertexts[i].size()
        );
    }
    fout.close();
}

void avx2Computation()
{
    __m256i a = _mm256_set_epi32(8, 7, 6, 5, 4, 3, 2, 1);
    __m256i b = _mm256_set_epi32(1, 1, 1, 1, 1, 1, 1, 1);
    asm("vmovdqa ymm0, %0\n\t"
        "vmovdqa ymm1, %1\n\t"
        :
        : "m"(a), "m"(b)
        : "ymm0", "ymm1");
    // Run only the vpaddd instruction inside a loop. The vmovdqu instructions
    // should be placed outside the loop
    for (int i = 0; i < LOOP_ITER; i++) {
        asm("vdivpd ymm0, ymm0, ymm1\n\t" : : : "ymm0", "ymm1");
    }
    asm("vmovdqu %0, ymm0\n\t" : "=m"(a) : : "ymm0");
}
void mixedSIMDComputation()
{
    // use a mix of 128 and 256 bit registers
    __m128i arrayA_128bit = _mm_set_epi32(4, 3, 2, 1);
    __m128i arrayB_128bit = _mm_set_epi32(1, 1, 1, 1);
    __m256i a = _mm256_set_epi32(8, 7, 6, 5, 4, 3, 2, 1);
    __m256i b = _mm256_set_epi32(1, 1, 1, 1, 1, 1, 1, 1);
    asm("vmovdqa ymm0, %0\n\t"
        "vmovdqa ymm1, %1\n\t"
        :
        : "m"(a), "m"(b)
        : "ymm0", "ymm1");
    asm("vmovdqu xmm2, %0\n\t"
        "vmovdqu xmm3, %1\n\t"
        :
        : "m"(arrayA_128bit), "m"(arrayB_128bit)
        : "xmm2", "xmm3");
    for (int i = 0; i < LOOP_ITER; i++) {
        asm("divpd xmm2, xmm3\n\t" : : : "xmm0", "xmm1");
        asm("vdivpd ymm0, ymm0, ymm1\n\t" : : : "ymm0", "ymm1");
    }
}
void aesOpenSSLKeyFixedPtFixedSGX()
{
    std::string plaintext = "123456789123456";
    char key[AES_KEY_SIZE] = "123456789123456";
    for (int i = 0; i < LOOP_ITER; i++) {
        customAES::aes128_encrypt_cbc_openssl_sgx<false>(
            std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
            key,
            nullptr,
            Computation::get_eid()
        );
    }
}
void aesOpenSSLKeyVariesPtFixedSGX()
{
    std::string plaintext = "123456789123456";
    for (int i = 0; i < LOOP_ITER; i++) {
        customAES::aes128_encrypt_cbc_openssl_sgx<false>(
            std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
            pregeneratedKeys[i],
            nullptr,
            Computation::get_eid()
        );
    }
}

void aesOpenSSLKeyFixedPtFixedLoopedSGX()
{
    std::string plaintext = "123456789123456";
    char key[AES_KEY_SIZE] = "123456789123456";
    customAES::aes128_encrypt_cbc_openssl_sgx<true>(
        std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
        key,
        nullptr,
        Computation::get_eid()
    );
}

void aesOpenSSLKeyVariesPtFixedLoopedSGX()
{
    std::string plaintext = "123456789123456";
    customAES::aes128_encrypt_cbc_openssl_sgx<true>(
        std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
        nullptr,
        pregeneratedKeys,
        Computation::get_eid()
    );
}

void multiplyFull()
{
    uint32_t constexpr FIXED_OPERAND = 0xFFFFFFFF;
    uint32_t constexpr HAMMING_WEIGHT = std::popcount(FIXED_OPERAND) + std::popcount(8u);
    for (uint32_t i = 0; i < LONG_LOOP_ITER; i++) {
        // inline assembly to perform multiplication of 0xFFFFFFFF with
        // randomOperand[i] in intel syntax
        asm("mov eax, %0\n\t"
            "mov ebx, %1\n\t"
            "mul ebx\n\t"
            :
            : "i"(FIXED_OPERAND), "i"(8)
            : "eax", "ebx");
    }
    cout << "Hamming weight multiplyFull: " << HAMMING_WEIGHT << endl;
}

void multiplyHalf()
{
    uint32_t constexpr FIXED_OPERAND = 0x0F0F0F0F;
    uint32_t constexpr HAMMING_WEIGHT = std::popcount(FIXED_OPERAND) + std::popcount(8u);
    for (uint32_t i = 0; i < LONG_LOOP_ITER; i++) {
        // inline assembly to perform multiplication of 0xFFFFFFFF with
        // randomOperand[i] in intel syntax
        asm("mov eax, %0\n\t"
            "mov ebx, %1\n\t"
            "mul ebx\n\t"
            :
            : "i"(FIXED_OPERAND), "i"(8)
            : "eax", "ebx");
    }
    cout << "Hamming weight multiplyHalf: " << HAMMING_WEIGHT << endl;
}

void multiplyQuarter()
{
    uint32_t constexpr FIXED_OPERAND = 0x03030303;
    uint32_t constexpr HAMMING_WEIGHT = std::popcount(FIXED_OPERAND) + std::popcount(8u);
    for (uint32_t i = 0; i < LONG_LOOP_ITER; i++) {
        // inline assembly to perform multiplication of 0xFFFFFFFF with
        // randomOperand[i] in intel syntax
        asm("mov eax, %0\n\t"
            "mov ebx, %1\n\t"
            "mul ebx\n\t"
            :
            : "i"(FIXED_OPERAND), "i"(8)
            : "eax", "ebx");
    }
    cout << "Hamming weight multiplyQuarter: " << HAMMING_WEIGHT << endl;
}

// clang-format off
std::unordered_map<std::string, std::function<Computation(void)>> const computationParser{
    {"empty", []() { return Computation("empty", emptyComputation); }},
    {"aesOpenSSLComputation", []() { return Computation("aesOpenSSLComputation", aesOpenSSLComputation); }},
    {"avx2Computation", []() { return Computation("avx2Computation", avx2Computation); }},
    {"mixedSIMDComputation", []() { return Computation("mixedSIMDComputation", mixedSIMDComputation); }},
    {"aesniComputation", []() { return Computation("aesniComputation", aesniComputation); }},
    {"aesniKeyFixedPtFixed", []() { return Computation("aesniKeyFixedPtFixed", aesniKeyFixedPtFixed); }},
    {"aesniKeyVariesPtFixed", []() { return Computation("aesniKeyVariesPtFixed", aesniKeyVariesPtFixed); }},
    {"aesOpenSSLKeyFixedPtFixed", []() { return Computation("aesOpenSSLKeyFixedPtFixed", aesOpenSSLKeyFixedPtFixed); }},
    {"aesOpenSSLKeyVariesPtFixed", []() { return Computation("aesOpenSSLKeyVariesPtFixed", aesOpenSSLKeyVariesPtFixed); }},
    {"aesniCPA", []() { return Computation("aesniCPA", aesniCPA); }},
    {"aesOpenSSLKeyFixedPtFixedSGX", []() { return Computation("aesOpenSSLKeyFixedPtFixedSGX", aesOpenSSLKeyFixedPtFixedSGX); }},
    {"aesOpenSSLKeyVariesPtFixedSGX", []() { return Computation("aesOpenSSLKeyVariesPtFixedSGX", aesOpenSSLKeyVariesPtFixedSGX); }},
    {"aesOpenSSLKeyFixedPtFixedLoopedSGX", []() { return Computation("aesOpenSSLKeyFixedPtFixedLoopedSGX", aesOpenSSLKeyFixedPtFixedLoopedSGX); }},
    {"aesOpenSSLKeyVariesPtFixedLoopedSGX", []() { return Computation("aesOpenSSLKeyVariesPtFixedLoopedSGX", aesOpenSSLKeyVariesPtFixedLoopedSGX); }},
    {"multiplyFull", []() { return Computation("multiplyFull", multiplyFull); }},
    {"multiplyHalf", []() { return Computation("multiplyHalf", multiplyHalf); }},
    {"multiplyQuarter", []() { return Computation("multiplyQuarter", multiplyQuarter); }}
};
// clang-format on