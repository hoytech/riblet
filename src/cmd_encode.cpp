#include <string.h>

#include <cmath>
#include <iostream>
#include <queue>
#include <functional>

#include <openssl/sha.h>

#include <docopt.h>
#include "golpe.h"


static const char USAGE[] =
R"(
    Usage:
      encode
)";



inline std::string dumpHash(uint8_t *hash) {
    return to_hex(std::string(reinterpret_cast<char*>(hash), 32));
}


/*
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
*/


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

struct CodedSymbol {
    std::string val;
    uint8_t hash[32] = {0};
    int64_t count = 0;

    CodedSymbol() {
    }

    CodedSymbol(std::string_view valRaw, int64_t count_ = 1) {
        val += to_sv<uint32_t>((uint32_t) valRaw.size()); // FIXME: bounds check this size, endianness
        val += valRaw;

        SHA256(reinterpret_cast<const unsigned char*>(valRaw.data()), valRaw.size(), hash);

        count = count_;
    }

    void add(const CodedSymbol &other) {
        if (other.val.size() > val.size()) val.resize(other.val.size(), '\0');
        for (size_t i = 0; i < other.val.size(); i++) val[i] ^= other.val[i];

        for (size_t i = 0; i < 32; i++) hash[i] ^= other.hash[i];

        count += other.count;
    }

    void negate() {
        count *= -1;
    }

    bool isPure() {
        if (count == 1 || count == -1) {
            auto v = std::string_view(val);

            if (v.size() < 4) throw herr("val unexpectedly small");
            auto size = from_sv<uint32_t>(v.substr(0, 4));
            v = v.substr(4);
            if (size > v.size()) return false;

            uint8_t tempHash[32];
            SHA256(reinterpret_cast<const unsigned char*>(v.data()), size, tempHash);
            return memcmp(hash, tempHash, 32) == 0;
        }

        return false;
    }

    std::string getVal() const {
        auto v = std::string_view(val);

        if (v.size() < 4) throw herr("val unexpectedly small");
        auto size = from_sv<uint32_t>(v.substr(0, 4));
        v = v.substr(4);
        if (size > v.size()) throw herr("val not decodeable (not pure?)");
        return std::string(v.substr(0, size));
    }
};


struct IndexGenerator {
    uint64_t curr = 0;
    uint64_t prng;

    IndexGenerator(const CodedSymbol &sym) {
        prng = *((uint64_t*)sym.hash); // FIXME: byteswap if big-endian
    }

    void jump() {
        prng *= 0xda942042e4dd58b5; // FIXME: use better PRNG

        curr += uint64_t(ceil((double(curr) + 1.5) * ((uint64_t(1)<<32)/std::sqrt(double(prng)+1) - 1)));
    }
};


struct SymbolQueue {
    struct QueueEntry {
        uint64_t codedStreamIndex;
        uint64_t symbolIndex;
    };

    struct SymbolEntry {
        IndexGenerator gen;
        CodedSymbol sym;
    };

    std::vector<SymbolEntry> symbolVec;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> codedQueue;

    void enqueue(const CodedSymbol &sym, const IndexGenerator &gen) {
        codedQueue.emplace(gen.curr, symbolVec.size());
        symbolVec.emplace_back(gen, sym);
    }

    uint64_t nextCodedIndex() {
        if (symbolVec.size() == 0) return std::numeric_limits<uint64_t>::max();
        return codedQueue.top().codedStreamIndex;
    }

    SymbolEntry &top() {
        if (symbolVec.size() == 0) throw herr("SymbolQueue empty");
        return symbolVec[codedQueue.top().symbolIndex];
    }

    void sort() {
        if (symbolVec.size() == 0) return;
        auto e = codedQueue.top();
        codedQueue.pop();
        e.codedStreamIndex = symbolVec[e.symbolIndex].gen.curr;
        codedQueue.push(e);
    }
};

inline bool operator>(SymbolQueue::QueueEntry const &a, SymbolQueue::QueueEntry const &b) {
    return a.codedStreamIndex > b.codedStreamIndex;
}


struct RIBLT {
    std::vector<CodedSymbol> codedSymbols;
    SymbolQueue queue;
    bool fixedSize = false;
    size_t nextPeel = 0;

  public:
    void expand(size_t n) {
        if (fixedSize) throw herr("can't expand fixed size vector");
        codedSymbols.resize(codedSymbols.size() + n);

        while (queue.nextCodedIndex() < codedSymbols.size()) {
            auto &top = queue.top();
            applySym(top.sym, top.gen);
            queue.sort();
        }
    }

    void push(const CodedSymbol &sym) {
        expand(1);
        codedSymbols.back().add(sym);
    }

    void setFixedSize() {
        fixedSize = true;
    }

    void add(const CodedSymbol &sym) {
        IndexGenerator gen(sym);
        applySym(sym, gen);
        if (!fixedSize) queue.enqueue(sym, gen);
    }

    void peel(std::function<void(const CodedSymbol &sym)> onSym) {
        for (; nextPeel < codedSymbols.size(); nextPeel++) {
            peelIndex(nextPeel, onSym);
        }
    }

    void peelIndex(size_t startIndex, std::function<void(const CodedSymbol &sym)> onSym) {
        std::vector<size_t> decodeable = {startIndex};

        while (decodeable.size()) {
            size_t index = decodeable.back();
            decodeable.pop_back();
            if (!codedSymbols[index].isPure()) continue;

            onSym(codedSymbols[index]);

            auto sym = codedSymbols[index];
            IndexGenerator gen(sym);
            sym.negate();
            applySym(sym, gen, [&](size_t newIndex){
                if (codedSymbols[newIndex].isPure()) decodeable.push_back(newIndex);
            });
        }
    }

  private:
    void applySym(const CodedSymbol &sym, IndexGenerator &gen, std::function<void(size_t)> cb = [](size_t){}) {
        while (gen.curr < codedSymbols.size()) {
            codedSymbols[gen.curr].add(sym);
            cb(gen.curr);
            gen.jump();
        }
    }
};

struct Decoder {
    // append a-b onto vector of codedSymbols
    // apply updates from priority queue for any pending peelings of this new index
    // if pure, peel it and add to priority queue for future peelings
    // repeat until first symbol is 0
};





void cmd_encode(const std::vector<std::string> &subArgs) {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE, subArgs, true, "");

    RIBLT e;

    e.add(CodedSymbol("a"));
    e.add(CodedSymbol("b"));
    e.add(CodedSymbol("c"));
    e.add(CodedSymbol("d"));

    e.expand(7);
    e.peel([](const auto &s){
        LW << "PEELED: " << s.getVal() << " / " << s.count;
    });

    /*
    std::string line;

    while (std::cin) {
        std::getline(std::cin, line);
        if (line.size() == 0) continue;
        e.add(CodedSymbol(line));
    }

    e.expand(20);
    */

    for (size_t i = 0; i < e.codedSymbols.size(); i++) {
        const auto &s = e.codedSymbols[i];
        LW << "CS " << i;
        LW << "  count = " << s.count;
        LW << "  val = '" << s.getVal() << "'";
    }
}
