#pragma once

#include <bitset>
#include <cmath>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/block.h>
#include <cryptoTools/Crypto/AES.h>
#include <emmintrin.h>
#include <tuple>
#include <vector>

using namespace osuCrypto;

std::tuple<std::vector<std::vector<u64>>, std::vector<std::vector<u64>>> genInputs(u64 n, u64 d);

inline u64 low(oc::block &blk)
{
    u64 low64 = ((u64 *)&blk)[0]; // 低 64 位

    return low64;
}

// Decompose the interval [start, end] using an improved method in appendix
inline std::vector<block> getIntervalPrefix(u64 start, u64 end)
{
    if (start > end) {
        throw std::runtime_error("decompose improve: end should >= start");
    }

    if (start == end) {
        return { block(0, start) };
    }

    u64 interval_len = end - start + 1; // interval length
    u64 bit_width = static_cast<u64>(std::log2(interval_len)) + 1;
    u64 aligned_start = start;

    // step 1
    // find aligned_start >= start, s.t. 2^(bit_width-1) | aligned_start
    while (aligned_start <= end && (aligned_start & ((1 << (bit_width - 1)) - 1))) {
        aligned_start++;
    }

    if (aligned_start > end) {
        throw std::runtime_error("decompose improve: can't find aligned_start");
    }

    // step 2
    // right_len = end - aligned_start + 1; left_len = aligned_start - start
    u64 right_len = end - aligned_start + 1; // right side length
    u64 left_len = aligned_start - start;    // left side length

    // convert to binary representation, bitset access is from low bit to high bit
    std::bitset<64> right_bits(right_len);
    std::bitset<64> left_bits(left_len);

    std::vector<block> results;       // result set
    u64 right_pos = aligned_start;    // move right
    u64 left_pos = aligned_start - 1; // move left

    // traverse from high bit to low bit
    for (u64 i = bit_width; i >= 1; i--) {
        if (right_bits[i - 1]) {
            // if right_len's i-th bit is 1
            results.push_back(block(i - 1, right_pos >> (i - 1)));
            right_pos += (1 << (i - 1));
        }
        if (left_bits[i - 1]) {
            // if left_len's i-th bit is 1
            results.push_back(block(i - 1, left_pos >> (i - 1)));
            left_pos -= (1 << (i - 1));
        }
    }

    return results;
}

inline std::vector<block> getPrefix(u64 x, int maxLen)
{
    std::vector<block> res;

    for (int len = 0; len < maxLen; len++) {
        res.push_back(block(len, x >> (len)));
    }

    return res;
}

void inline Hash(std::vector<block> &input)
{
    auto n8 = input.size() / 8 * 8;

    block mask = OneBlock ^ AllOneBlock; // notOneBlock

    auto r = input.data();

    for (u64 i = 0; i < n8; i += 8) {
        r[0] = r[0] & mask;
        r[1] = r[1] & mask;
        r[2] = r[2] & mask;
        r[3] = r[3] & mask;
        r[4] = r[4] & mask;
        r[5] = r[5] & mask;
        r[6] = r[6] & mask;
        r[7] = r[7] & mask;

        oc::mAesFixedKey.hashBlocks<8>(r, r);
        r += 8;
    }
    for (u64 i = n8; i < input.size(); i++) {
        input[i] = input[i] & mask;
        input[i] = oc::mAesFixedKey.hashBlock(input[i]);
    }
}