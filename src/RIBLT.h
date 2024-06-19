#pragma once

#include <cmath>
#include <cstring>
#include <iostream>
#include <queue>
#include <functional>

#include <openssl/sha.h>

#include "golpe.h"

#include "ParseUtils.h"


namespace riblet {


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

    CodedSymbol(std::string_view val_, std::string_view hash_, int64_t count_) {
        if (hash_.size() != 32) throw herr("invalid hash size");

        val += val_;
        std::memcpy(hash, hash_.data(), 32);

        count = count_;
    }

    void add(const CodedSymbol &other) {
        update(other, other.count);
    }

    void sub(const CodedSymbol &other) {
        update(other, -other.count);
    }

    void update(const CodedSymbol &other, int64_t inc) {
        if (other.val.size() > val.size()) val.resize(other.val.size(), '\0');
        for (size_t i = 0; i < other.val.size(); i++) val[i] ^= other.val[i];

        for (size_t i = 0; i < 32; i++) hash[i] ^= other.hash[i];

        count += inc;
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
            return std::memcmp(hash, tempHash, 32) == 0;
        }

        return false;
    }

    bool isZero() const {
        uint8_t empty[32] = {0};
        return count == 0 && memcmp(hash, empty, 32) == 0;
    }

    std::string getVal() const {
        auto v = std::string_view(val);

        // FIXME: code duplication
        if (v.size() < 4) throw herr("val unexpectedly small");
        auto size = from_sv<uint32_t>(v.substr(0, 4));
        v = v.substr(4);
        if (size > v.size()) throw herr("val not decodeable (not pure?)");
        return std::string(v.substr(0, size));
    }

    std::string_view getHashSV() const {
        return std::string_view(reinterpret_cast<const char*>(hash), 32);
    }
};


struct IndexGenerator {
    uint64_t curr = 0;
    uint64_t prng;

    IndexGenerator(const CodedSymbol &sym) {
        prng = *((uint64_t*)sym.hash); // FIXME: byteswap if big-endian, use from_sv
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

    void bubbleUp() {
        if (symbolVec.size() == 0) throw herr("can't bubbleUp empty queue");
        auto e = codedQueue.top();
        codedQueue.pop();
        e.codedStreamIndex = symbolVec[e.symbolIndex].gen.curr;
        codedQueue.push(e);
    }

    void clear() {
        symbolVec.clear();
        symbolVec.shrink_to_fit();

        decltype(codedQueue) emptyQueue;
        codedQueue.swap(emptyQueue);
    }
};

inline bool operator>(SymbolQueue::QueueEntry const &a, SymbolQueue::QueueEntry const &b) {
    return a.codedStreamIndex > b.codedStreamIndex;
}


struct RIBLT {
    std::vector<CodedSymbol> codedSymbols;
    SymbolQueue queue;
    bool doneExpanding = false;
    size_t nextPeel = 0;

  public:
    void expand(size_t n) {
        if (doneExpanding) throw herr("can't expand fixed size vector");
        codedSymbols.resize(codedSymbols.size() + n);

        while (queue.nextCodedIndex() < codedSymbols.size()) {
            auto &top = queue.top();
            applySym(top.sym, top.gen);
            queue.bubbleUp();
        }
    }

    void push(const CodedSymbol &sym) {
        expand(1);
        codedSymbols.back().add(sym);
    }

    void setDoneExpanding() {
        doneExpanding = true;
        queue.clear();
    }

    void add(const CodedSymbol &sym) {
        IndexGenerator gen(sym);
        applySym(sym, gen);
        if (!doneExpanding) queue.enqueue(sym, gen);
    }

    void peel(std::function<void(const CodedSymbol &sym)> onSym) {
        for (; nextPeel < codedSymbols.size(); nextPeel++) {
            peelIndex(nextPeel, onSym);
        }
    }

    void peelIndex(size_t startIndex, std::function<void(const CodedSymbol &sym)> onSym) {
        std::vector<size_t> decodeable = {startIndex};

        while (decodeable.size()) {
            LW << "DING: " << decodeable.size();
            size_t index = decodeable.back();
            decodeable.pop_back();
            if (!codedSymbols[index].isPure()) continue;

            onSym(codedSymbols[index]);

            auto sym = codedSymbols[index];
            IndexGenerator gen(sym);
            sym.negate();
            applySym(sym, gen, [&](size_t newIndex){
                if (codedSymbols[newIndex].isPure()) {
                LW << "PURE " << newIndex;
                decodeable.push_back(newIndex);
                }
            });
        }
    }

    bool isBalanced() {
        return codedSymbols.size() > 0 && codedSymbols[0].isZero();
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

}

using RIBLT = riblet::RIBLT;
