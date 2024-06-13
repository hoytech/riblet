#pragma once

#include <string_view>
#include <string>

#include "golpe.h"



template<typename T>
inline std::string_view to_sv(const T &v) {
    return std::string_view(reinterpret_cast<const char*>(std::addressof(v)), sizeof(v));
}

template<typename T>
inline T from_sv(std::string_view v) {
    if (v.size() != sizeof(T)) throw herr("from_sv");
    T ret;
    std::memcpy(&ret, const_cast<char*>(v.data()), sizeof(T));
    return ret;
}


inline uint8_t getByte(std::string_view &encoded) {
    if (encoded.size() < 1) throw herr("parse ends prematurely");
    uint8_t output = encoded[0];
    encoded = encoded.substr(1);
    return output;
}

inline std::string getBytes(std::string_view &encoded, size_t n) {
    if (encoded.size() < n) throw herr("parse ends prematurely");
    auto res = encoded.substr(0, n);
    encoded = encoded.substr(n);
    return std::string(res);
};


inline uint64_t decodeVarInt(std::string_view &encoded) {
    uint64_t res = 0;

    while (1) {
        if (encoded.size() == 0) throw herr("premature end of varint");
        uint64_t byte = encoded[0];
        encoded = encoded.substr(1);
        res = (res << 7) | (byte & 0b0111'1111);
        if ((byte & 0b1000'0000) == 0) break;
    }

    return res;
}

inline uint64_t decodeVarInt(const std::string &str) {
    auto sv = std::string_view(str);
    return decodeVarInt(sv);
}

inline std::string encodeVarInt(uint64_t n) {
    if (n == 0) return std::string(1, '\0');

    std::string o;

    while (n) {
        o.push_back(static_cast<unsigned char>(n & 0x7F));
        n >>= 7;
    }

    std::reverse(o.begin(), o.end());

    for (size_t i = 0; i < o.size() - 1; i++) {
        o[i] |= 0x80;
    }

    return o;
}

inline int64_t decodeVarIntZ(std::string_view &encoded) {
    uint64_t n = decodeVarInt(encoded);
    return int64_t(n >> 1) * (n & 1 ? -1 : 1);
}

inline std::string encodeVarIntZ(int64_t n) {
    return encodeVarInt((std::abs(n) << 1) | (n < 0 ? 1 : 0));
}


inline uint32_t getUint32LE(std::string_view &encoded) {
    std::string_view s = getBytes(encoded, 4);
    uint32_t v = from_sv<uint32_t>(s);
    // FIXME: byteswap on big-endian
    return v;
}

inline std::string encodeUint32LE(uint32_t v) {
    // FIXME: byteswap on big-endian
    std::string s = std::string(to_sv<uint32_t>(v));
    return s;
}
