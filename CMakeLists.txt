cmake_minimum_required(VERSION 3.20)

project(rapl_attack)
set(CMAKE_CXX_STANDARD 17)
set(SGX_MODE Debug CACHE STRING "SGX build mode: Debug; PreRelease; Release.")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package(SGX REQUIRED)
set(EDL_SEARCH_PATHS Enclave)

set(E_SRCS Enclave/src/Enclave.cpp)
set(ENCLAVE_INCLUDE_DIRS Enclave/include)
set(SRCS App/src/App.cpp App/src/utilities.cpp App/src/computations.cpp App/src/aes.cpp)
set(APP_INCLUDE_DIRS App/include)
if(SGX_HW AND SGX_MODE STREQUAL "Release")
    set(LDS Enclave/Enclave.lds)
else()
    set(LDS Enclave/Enclave_debug.lds)
endif()
# generate private key if it doesn't exist
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Enclave/Enclave_private.pem)
    message(STATUS "Generating private key for enclave")
    execute_process(COMMAND openssl genrsa -3 -out ${CMAKE_CURRENT_SOURCE_DIR}/Enclave/Enclave_private.pem 3072)
endif()

# don't warn me about deprecations
add_compile_options(-Wno-deprecated-declarations)


add_enclave_library(enclave SRCS ${E_SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS} LDSCRIPT ${LDS} USE_SGXSSL)
target_include_directories(enclave PRIVATE ${ENCLAVE_INCLUDE_DIRS})
enclave_sign(enclave KEY Enclave/Enclave_private.pem CONFIG Enclave/Enclave.config.xml)

add_untrusted_executable(rapl_attack SRCS ${SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS} USE_SGXSSL)
set_property(TARGET rapl_attack PROPERTY CXX_STANDARD 20)
# link OpenSSL
find_package(OpenSSL REQUIRED)
target_link_libraries(rapl_attack OpenSSL::Crypto)
target_include_directories(rapl_attack PRIVATE ${APP_INCLUDE_DIRS})
# enable aes-ni and avx2
target_compile_options(rapl_attack PRIVATE -masm=intel -mavx2 -maes)
add_dependencies(rapl_attack enclave-sign)
# produce the assembly for the object files as well as separate .s files
set_target_properties(rapl_attack PROPERTIES COMPILE_FLAGS "-save-temps -fverbose-asm")
# add_untrusted_executable(test_suite SRCS App/src/test_suite.cpp App/src/utilities.cpp App/src/computations.cpp App/src/aes.cpp EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS} USE_SGXSSL)
# target_link_libraries(test_suite OpenSSL::Crypto)
# target_include_directories(test_suite PRIVATE ${APP_INCLUDE_DIRS})
# target_compile_options(test_suite PRIVATE -masm=intel -mavx2 -mavx -msse2 -maes)
# set_property(TARGET test_suite PROPERTY CXX_STANDARD 20)