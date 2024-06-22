#pragma once

#include <cstring>
#include <string_view>

#include <openssl/sha.h>


namespace riblet {

struct PRNG {
    uint64_t state;

    PRNG(uint64_t state) : state(state) {
    }

    // *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
    // Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

    uint32_t rand32() {
        uint64_t oldstate = state;
        state = oldstate * 6364136223846793005ULL + 1;
        uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
        uint32_t rot = oldstate >> 59u;
        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }

    uint64_t rand64() {
        uint64_t o = rand32();
        o = (o << 32) | rand32();
        return o;
    }
};

struct Hash {
    uint8_t h[16];

    Hash() : h{0} {
    }

    Hash(std::string_view val) {
        uint8_t tmp[32];
        SHA256(reinterpret_cast<const unsigned char*>(val.data()), val.size(), tmp);
        static_assert(sizeof(h) <= sizeof(tmp));
        std::memcpy(h, tmp, sizeof(h));
    }

    static Hash raw(std::string_view s) {
        Hash o;

        if (s.size() != sizeof(o.h)) throw herr("invalid hash size");
        std::memcpy(o.h, s.data(), sizeof(o.h));

        return o;
    }

    PRNG getPRNG() const {
        static_assert(sizeof(h) >= sizeof(uint64_t));
        return PRNG(*((uint64_t*)h)); // FIXME: byteswap if big-endian
    }

    bool operator==(const Hash &o) const {
        return std::memcmp(h, o.h, sizeof(h)) == 0;
    }
};

}

// Inject riblet::Hash specialization of std::hash, so we can use it in a flat_hash_set
namespace std {
    template<> struct hash<riblet::Hash> {
        std::size_t operator()(riblet::Hash const &p) const {
            static_assert(sizeof(p.h) >= sizeof(size_t));
            return *((size_t*)&p.h);
        }
    };
}
