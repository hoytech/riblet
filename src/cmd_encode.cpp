#include <cmath>
#include <iostream>

#include <openssl/sha.h>

#include <docopt.h>
#include "golpe.h"


static const char USAGE[] =
R"(
    Usage:
      encode
)";





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


struct CodedSymbol {
    std::string val;
    uint8_t hash[32] = {0};
    int64_t count = 0;

    CodedSymbol() {
    }

    CodedSymbol(std::string_view valRaw) {
        val = encodeVarInt(valRaw.size());
        val += valRaw;

        SHA256(reinterpret_cast<const unsigned char*>(val.data()), val.size(), hash);

        count = 1;
    }

    void add(const CodedSymbol &other) {
        update(other, 1);
    }

    void sub(const CodedSymbol &other) {
        update(other, -1);
    }

 private:
    void update(const CodedSymbol &other, int64_t sign) {
        if (other.val.size() > val.size()) val.resize(other.val.size(), '\0');
        for (size_t i = 0; i < other.val.size(); i++) val[i] ^= other.val[i];

        for (size_t i = 0; i < 32; i++) hash[i] ^= other.hash[i];

        count += sign * other.count;
    }
};


struct IndexGenerator {
    uint64_t prng;
    uint64_t lastIndex = 0;

    IndexGenerator(const CodedSymbol &sym) {
        prng = *((uint64_t*)sym.hash); // FIXME: make big-endian machines byteswap
    }

    uint64_t nextIndex() {
        prng *= 0xda942042e4dd58b5; // FIXME: use better PRNG

        lastIndex += uint64_t(ceil((double(lastIndex) + 1.5) * ((uint64_t(1)<<32)/std::sqrt(double(prng)+1) - 1)));
        return lastIndex;
    }
};



struct Encoder {
    std::vector<CodedSymbol> codedSymbols;

    void allocate(uint64_t n) {
        codedSymbols.resize(n);
    }

    void add(std::string_view val) {
        CodedSymbol sym(val);
        IndexGenerator gen(sym);

        uint64_t index = 0;

        while (index < codedSymbols.size()) {
            codedSymbols[index].add(val);
            index = gen.nextIndex();
        }
    }
};

struct Decoder {
};



//LW << to_hex(std::string(reinterpret_cast<char*>(hash), 32));


void cmd_encode(const std::vector<std::string> &subArgs) {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE, subArgs, true, "");

    Encoder e;
    e.allocate(20);

    std::string line;

    while (std::cin) {
        std::getline(std::cin, line);
        if (line.size() == 0) continue;
        e.add(line);
    }

    for (size_t i = 0; i < e.codedSymbols.size(); i++) {
        const auto &s = e.codedSymbols[i];
        LW << "CS " << i;
        LW << "  count = " << s.count;
        LW << "  val = '" << s.val << "'";
    }
}
