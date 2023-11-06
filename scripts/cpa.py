# Going to perform CPA on the AES-128 implementation

# Read binary file "aesniCPA.bin" into a byte array
# Binary format of the file:
# First 4 bytes -> Number of ciphertexts in the file
# Next 16 bytes -> Key
# Every 16 bytes after that is one ciphertext, load ciphertext into ciphertexts[] list

# Import libraries
import numpy as np
import matplotlib.pyplot as plt
import sys
import os

def inverseShiftRows(byteArray):
    # Inverse ShiftRows operation of AES-128
    # Input: 16 byte array
    # Output: 16 byte array
    # Shift row 1 right by 1
    byteArray[1], byteArray[5], byteArray[9], byteArray[13] = byteArray[13], byteArray[1], byteArray[5], byteArray[9]
    # Shift row 2 right by 2
    byteArray[2], byteArray[6], byteArray[10], byteArray[14] = byteArray[10], byteArray[14], byteArray[2], byteArray[6]
    byteArray[2], byteArray[6], byteArray[10], byteArray[14] = byteArray[10], byteArray[14], byteArray[2], byteArray[6]
    # Shift row 3 right by 3
    byteArray[3], byteArray[7], byteArray[11], byteArray[15] = byteArray[7], byteArray[11], byteArray[15], byteArray[3]
    byteArray[3], byteArray[7], byteArray[11], byteArray[15] = byteArray[7], byteArray[11], byteArray[15], byteArray[3]
    byteArray[3], byteArray[7], byteArray[11], byteArray[15] = byteArray[7], byteArray[11], byteArray[15], byteArray[3]
    return byteArray


with open("aesniCPA.bin", "rb") as f:
    # Read number of ciphertexts
    num_ciphertexts = int.from_bytes(f.read(4), byteorder='big')
    # Read key
    key = f.read(16)
    # Read ciphertexts
    ciphertexts = []
    for i in range(num_ciphertexts):
        ciphertexts.append(f.read(16))

# read energy readings from energy_readings.csv
# Each line contains an energy reading at a timepoint
with open("energy_readings.csv", "r") as f:
    energy_readings = f.readlines()

# Convert energy readings to float
energy_readings = [float(energy_reading) for energy_reading in energy_readings]
# Convert to difference array
energy_readings = np.diff(energy_readings)


# We are targeting last round of AES-128
# Need to find out C^-1(key[i] ^ C[i]) for all i after accounting for S-box and ShiftRows

# Inverse s-box of AES-128 indexed by byte
sboxInv = [
        0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
        0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
        0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
        0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
        0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
        0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
        0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
        0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
        0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
        0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
        0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
        0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
        0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
        0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
        0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
        0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
]

for byte_position in range(0, 16):
    # For each byte in the key, we need to calculate the correlation coefficient
    # between the hypothetical power consumption and the actual power consumption
    # We will do this for each hypothetical key byte
    # We will then take the key byte with the highest correlation coefficient as the correct key byte
    # We will then repeat this for all 16 key bytes

    # Create hypothetical power consumption array
    # for every ciphertext and every hypothetical key byte
    # powerEstimates[ciphertext][keyByte]
    powerEstimates = np.zeros((num_ciphertexts, 256))

    # For each hypothetical key byte
    for guessedKeyByte in range(0, 256):
        # For each ciphertext
        for index, ciphertext in enumerate(ciphertexts):
            # Calculate hypothetical power consumption
            # C^-1(key[i] ^ C[i])
            prev_state = sboxInv[guessedKeyByte ^ ciphertext[byte_position]]
            # Target index depends on shiftRows
            # Get target index based on byte_position
            if byte_position in [0, 4, 8, 12]:
                target_index = byte_position
            elif byte_position in [1, 5, 9, 13]:
                target_index = (byte_position + 4) % 16
            elif byte_position in [2, 6, 10, 14]:
                target_index = (byte_position + 8) % 16
            elif byte_position in [3, 7, 11, 15]:
                target_index = (byte_position + 12) % 16
            cur_state = ciphertext[target_index]
            # Calculate hypothetical power consumption
            powerEstimates[index][guessedKeyByte] = prev_state ^ cur_state
    
    # For each hypothetical key byte, and for each timepoint, calculate correlation coefficient
    # between hypothetical power consumption and actual power consumption across all ciphertexts
    # correlationMatrix[keyByte][timePoint]
    correlationMatrix = np.zeros((256, energy_readings.shape[0]))
    for keyByte in range(0, 256):
        for timePoint in range(0, energy_readings.shape[0]):
            correlationMatrix[keyByte][timePoint] = np.corrcoef(powerEstimates[:, keyByte], energy_readings[:, timePoint])[0][1]
    
    # Find key byte with highest correlation coefficient
    bestKeyByte = 0
    bestCorrelation = 0
    for keyByte in range(0, 256):
        if correlationMatrix[keyByte].max() > bestCorrelation:
            bestKeyByte = keyByte
            bestCorrelation = correlationMatrix[keyByte].max()
    print(f"Key byte {byte_position}: {bestKeyByte}")
