#ifndef DES_H
#define DES_H

#include <bitset>
#include <vector>

using namespace std;

bitset<64> initialPermutation(bitset<64> data);
bitset<64> finalPermutation(bitset<64> data);
bitset<48> expansionPermutation(bitset<32> halfBlock);
bitset<32> substitution(bitset<48> expandedBlock);
bitset<32> permutationFunction(bitset<32> substitutedBlock);
pair<bitset<32>, bitset<32>> feistelFunction(bitset<32> left, bitset<32> right, bitset<48> subkey);
vector<bitset<48>> generateSubKeys(bitset<64> key);
bitset<64> desEncrypt(bitset<64> plaintext, vector<bitset<48>> subkeys);
bitset<64> desDecrypt(bitset<64> ciphertext, vector<bitset<48>> subkeys);
bitset<64> keyPermutation(bitset<64> key);
bitset<28> leftCircularShift(bitset<28> halfKey, int shift);
bitset<48> keyCompressionPermutation(bitset<56> permutedKey);
bitset<64> stringToBitset(const string &input);
string bitsetToString(bitset<64> bits);

#endif // DES_H
