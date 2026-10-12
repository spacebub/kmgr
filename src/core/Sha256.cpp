// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <stdexcept>

#include "Sha256.h"

namespace {
// clang-format off
constexpr std::array<uint32_t, 64> ROUND_CONSTANTS = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};
// clang-format on

constexpr size_t BLOCK = 64;

uint32_t rotate(const uint32_t value, const unsigned bits) {
    return (value >> bits) | (value << (32U - bits));
}

class Hasher {
    // clang-format off
    std::array<uint32_t, 8> _state = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    };
    // clang-format on
    std::array<unsigned char, BLOCK> _block{};
    size_t _filled = 0;
    uint64_t _length = 0;

    void compress(const unsigned char *chunk) {
        uint32_t schedule[64];

        for (size_t word = 0; word < 16; ++word) {
            const unsigned char *bytes = chunk + (4 * word);

            schedule[word] = static_cast<uint32_t>(bytes[0]) << 24 | static_cast<uint32_t>(bytes[1]) << 16
                             | static_cast<uint32_t>(bytes[2]) << 8 | static_cast<uint32_t>(bytes[3]);
        }

        for (size_t word = 16; word < 64; ++word) {
            const uint32_t near = schedule[word - 15];
            const uint32_t far = schedule[word - 2];
            const uint32_t sigma0 = rotate(near, 7) ^ rotate(near, 18) ^ (near >> 3);
            const uint32_t sigma1 = rotate(far, 17) ^ rotate(far, 19) ^ (far >> 10);

            schedule[word] = schedule[word - 16] + sigma0 + schedule[word - 7] + sigma1;
        }

        uint32_t a = _state[0];
        uint32_t b = _state[1];
        uint32_t c = _state[2];
        uint32_t d = _state[3];
        uint32_t e = _state[4];
        uint32_t f = _state[5];
        uint32_t g = _state[6];
        uint32_t h = _state[7];

        for (size_t round = 0; round < 64; ++round) {
            const uint32_t big1 = rotate(e, 6) ^ rotate(e, 11) ^ rotate(e, 25);
            const uint32_t choose = (e & f) ^ (~e & g);
            const uint32_t first = h + big1 + choose + ROUND_CONSTANTS[round] + schedule[round];
            const uint32_t big0 = rotate(a, 2) ^ rotate(a, 13) ^ rotate(a, 22);
            const uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t second = big0 + majority;

            h = g;
            g = f;
            f = e;
            e = d + first;
            d = c;
            c = b;
            b = a;
            a = first + second;
        }

        _state[0] += a;
        _state[1] += b;
        _state[2] += c;
        _state[3] += d;
        _state[4] += e;
        _state[5] += f;
        _state[6] += g;
        _state[7] += h;
    }

public:
    void update(const unsigned char *data, size_t size) {
        _length += size;

        while (size > 0) {
            const size_t take = std::min(size, BLOCK - _filled);

            memcpy(_block.data() + _filled, data, take);
            _filled += take;
            data += take;
            size -= take;

            if (_filled == BLOCK) {
                compress(_block.data());
                _filled = 0;
            }
        }
    }

    std::string finish() {
        const uint64_t bits = _length * 8;
        const unsigned char one = 0x80;
        const unsigned char zero = 0;

        update(&one, 1);

        while (_filled != BLOCK - 8) {
            update(&zero, 1);
        }

        unsigned char length[8];

        for (unsigned byte = 0; byte < 8; ++byte) {
            length[byte] = static_cast<unsigned char>(bits >> (56U - (8U * byte)));
        }

        update(length, 8);

        std::string digest;

        for (const uint32_t word : _state) {
            digest += std::format("{:08x}", word);
        }

        return digest;
    }
};
}

std::string Sha256::text(const std::string_view data) {
    Hasher hasher;

    hasher.update(reinterpret_cast<const unsigned char *>(data.data()), data.size());

    return hasher.finish();
}

std::string Sha256::file(const std::string &path) {
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open " + path + " for hashing");
    }

    Hasher hasher;
    std::array<char, 1 << 16> buffer{};

    while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
        hasher.update(reinterpret_cast<const unsigned char *>(buffer.data()), static_cast<size_t>(file.gcount()));
    }

    if (file.bad()) {
        throw std::runtime_error("Could not read " + path + " for hashing");
    }

    return hasher.finish();
}
