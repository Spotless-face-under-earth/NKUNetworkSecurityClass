#include "DES.h"

#include <iostream>
#include <bitset>
#include <vector>
#include <string>

using namespace std;

// 初始置换表 (IP)
const int IP[64] = {
    58, 50, 42, 34, 26, 18, 10, 2,
    60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6,
    64, 56, 48, 40, 32, 24, 16, 8,
    57, 49, 41, 33, 25, 17, 9, 1,
    59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5,
    63, 55, 47, 39, 31, 23, 15, 7
};

// 逆初始置换表 (IP^-1)
const int IP_INV[64] = {
    40, 8, 48, 16, 56, 24, 64, 32,
    39, 7, 47, 15, 55, 23, 63, 31,
    38, 6, 46, 14, 54, 22, 62, 30,
    37, 5, 45, 13, 53, 21, 61, 29,
    36, 4, 44, 12, 52, 20, 60, 28,
    35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26,
    33, 1, 41, 9, 49, 17, 57, 25
};

// 扩展置换表 (E)
const int E[48] = {
    32, 1, 2, 3, 4, 5,
    4, 5, 6, 7, 8, 9,
    8, 9, 10, 11, 12, 13,
    12, 13, 14, 15, 16, 17,
    16, 17, 18, 19, 20, 21,
    20, 21, 22, 23, 24, 25,
    24, 25, 26, 27, 28, 29,
    28, 29, 30, 31, 32, 1
};

// S 盒
const int S_BOX[8][4][16] = {
    // S1
    {
        {14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7},
        {0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8},
        {4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0},
        {15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13}
    },
    // S2
    {
        {15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10},
        {3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5},
        {0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15},
        {13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9}
    },
    // S3
    {
        {10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8},
        {13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1},
        {13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7},
        {1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12}
    },
    // S4
    {
        {7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15},
        {13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9},
        {10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4},
        {3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14}
    },
    // S5
    {
        {2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9},
        {14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6},
        {4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14},
        {11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3}
    },
    // S6
    {
        {12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11},
        {10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8},
        {9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6},
        {4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13}
    },
    // S7
    {
        {4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1},
        {13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6},
        {1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2},
        {6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12}
    },
    // S8
    {
        {13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7},
        {1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2},
        {7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8},
        {2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11}
    }
};

// 置换函数 P
const int P[32] = {
    16, 7, 20, 21,
    29, 12, 28, 17,
    1, 15, 23, 26,
    5, 18, 31, 10,
    2, 8, 24, 14,
    32, 27, 3, 9,
    19, 13, 30, 6,
    22, 11, 4, 25
};

// 密钥置换表 (PC-1)
const int PC1[56] = {
    57, 49, 41, 33, 25, 17, 9,
    1, 58, 50, 42, 34, 26, 18,
    10, 2, 59, 51, 43, 35, 27,
    19, 11, 3, 60, 52, 44, 36,
    63, 55, 47, 39, 31, 23, 15,
    7, 62, 54, 46, 38, 30, 22,
    14, 6, 61, 53, 45, 37, 29,
    21, 13, 5, 28, 20, 12, 4
};

// 密钥置换表 (PC-2)
const int PC2[48] = {
    14, 17, 11, 24, 1, 5,
    3, 28, 15, 6, 21, 10,
    23, 19, 12, 4, 26, 8,
    16, 7, 27, 20, 13, 2,
    41, 52, 31, 37, 47, 55,
    30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53,
    46, 42, 50, 36, 29, 32
};

// 左移位数表
const int SHIFT[16] = {
    1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1
};

// 初始置换
bitset<64> initialPermutation(bitset<64> data) {
    bitset<64> result;
    for (int i = 0; i < 64; i++) {
        result[63 - i] = data[63 - (IP[i] - 1)];
    }
    return result;
}

// 逆初始置换
bitset<64> inverseInitialPermutation(bitset<64> data) {
    bitset<64> result;
    for (int i = 0; i < 64; i++) {
        result[63 - i] = data[63 - (IP_INV[i] - 1)];
    }
    return result;
}

// 扩展置换
bitset<48> expansionPermutation(bitset<32> data) {
    bitset<48> result;
    for (int i = 0; i < 48; i++) {
        result[47 - i] = data[31 - (E[i] - 1)];
    }
    return result;
}

// S 盒置换
bitset<32> sBoxSubstitution(bitset<48> data) {
    bitset<32> result;
    for (int i = 0; i < 8; i++) {
        int row = data[47 - (i * 6)] * 2 + data[47 - (i * 6 + 5)];
        int col = data[47 - (i * 6 + 1)] * 8 + data[47 - (i * 6 + 2)] * 4 +
                  data[47 - (i * 6 + 3)] * 2 + data[47 - (i * 6 + 4)];
        int val = S_BOX[i][row][col];
        result[31 - (i * 4)] = (val >> 3) & 1;
        result[31 - (i * 4 + 1)] = (val >> 2) & 1;
        result[31 - (i * 4 + 2)] = (val >> 1) & 1;
        result[31 - (i * 4 + 3)] = val & 1;
    }
    return result;
}

// P 置换
bitset<32> permutation(bitset<32> data) {
    bitset<32> result;
    for (int i = 0; i < 32; i++) {
        result[31 - i] = data[31 - (P[i] - 1)];
    }
    return result;
}

// 生成子密钥
vector<bitset<48>> generateSubKeys(bitset<64> key) {
    vector<bitset<48>> subKeys(16);
    bitset<56> permutedKey;
    for (int i = 0; i < 56; i++) {
        // PC-1 置换
        permutedKey[55 - i] = key[63 - (PC1[i] - 1)];
    }
    // 提取高 28 位和低 28 位
    bitset<28> left, right;
    for (int i = 0; i < 28; i++) {
        left[27 - i] = permutedKey[55 - i];         // 高 28 位
        right[27 - i] = permutedKey[27 - i];        // 低 28 位
    }
    for (int i = 0; i < 16; i++) {
        // 循环左移
        left = (left << SHIFT[i]) | (left >> (28 - SHIFT[i]));
        right = (right << SHIFT[i]) | (right >> (28 - SHIFT[i]));
        // 合并左右两部分
        bitset<56> combined = (left.to_ullong() << 28) | right.to_ullong();
        bitset<48> subKey;
        // PC-2 置换
        for (int j = 0; j < 48; j++) {
            subKey[47 - j] = combined[55 - (PC2[j] - 1)];
        }
        subKeys[i] = subKey;
    }
    return subKeys;
}

// DES 加密
bitset<64> desEncrypt(bitset<64> data, vector<bitset<48>> subKeys) {
    // 初始置换
    data = initialPermutation(data);

    // 提取高 32 位和低 32 位
    bitset<32> left, right;
    for (int i = 0; i < 32; i++) {
        left[31 - i] = data[63 - i];  // 高 32 位
        right[31 - i] = data[31 - i]; // 低 32 位
    }

    // 16 轮 Feistel 网络
    for (int i = 0; i < 16; i++) {
        bitset<32> temp = right;
        right = left ^ permutation(sBoxSubstitution(expansionPermutation(right) ^ subKeys[i]));
        left = temp;
    }

    // 合并左右两部分
    bitset<64> result = (right.to_ullong() << 32) | left.to_ullong();

    // 逆初始置换
    return inverseInitialPermutation(result);
}

// DES 解密
bitset<64> desDecrypt(bitset<64> data, vector<bitset<48>> subKeys) {
    // 初始置换
    data = initialPermutation(data);

    // 提取高 32 位和低 32 位
    bitset<32> left, right;
    for (int i = 0; i < 32; i++) {
        left[31 - i] = data[63 - i];  // 高 32 位
        right[31 - i] = data[31 - i]; // 低 32 位
    }

    // 16 轮 Feistel 网络（子密钥逆序使用）
    for (int i = 15; i >= 0; i--) {
        bitset<32> temp = right;
        right = left ^ permutation(sBoxSubstitution(expansionPermutation(right) ^ subKeys[i]));
        left = temp;
    }

    // 合并左右两部分
    bitset<64> result = (right.to_ullong() << 32) | left.to_ullong();

    // 逆初始置换
    return inverseInitialPermutation(result);
}

// int main() {
//     // 示例密钥和数据
//     bitset<64> key = 0x133457799BBCDFF1;
//     bitset<64> data = 0x0123456789ABCDEF;

//     // 生成子密钥
//     vector<bitset<48>> subKeys = generateSubKeys(key);

//     // 加密
//     bitset<64> encrypted = desEncrypt(data, subKeys);
//     cout << "Encrypted: " << hex << encrypted.to_ullong() << endl;

//     // 解密
//     bitset<64> decrypted = desDecrypt(encrypted, subKeys);
//     cout << "Decrypted: " << hex << decrypted.to_ullong() << endl;

//     return 0;
// }