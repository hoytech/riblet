#pragma once

#include <cmath>
#include <iostream>
#include <queue>
#include <optional>
#include <functional>

#include "golpe.h"

#include "Crypto.h"
#include "ParseUtils.h"


namespace riblet {

struct CodedSymbol {
    Hash hash;
    int64_t count = 0;
    std::string val;

    CodedSymbol() {
    }

    CodedSymbol(std::string_view valRaw, int64_t count = 1) : hash(valRaw), count(count) {
        val += to_sv<uint32_t>((uint32_t) valRaw.size()); // FIXME: check this size is reasonable, byteswap on big-endian
        val += valRaw;
    }

    CodedSymbol(std::string_view val, Hash hash, int64_t count) : hash(hash), count(count), val(val) {
    }

    void add(const CodedSymbol &other) {
        update(other, other.count);
    }

    void sub(const CodedSymbol &other) {
        update(other, -other.count);
    }

    void negate() {
        count *= -1;
    }

    bool isPure() const {
        if (count == 1 || count == -1) {
            auto v = decodeVal();
            if (!v) return false;

            return Hash(*v) == hash;
        }

        return false;
    }

    bool isZero() const {
        static const Hash emptyHash;
        return count == 0 && hash == emptyHash;
    }

    std::string getVal() const {
        auto v = decodeVal();
        if (!v) throw herr("val not decodeable (not pure?)");
        return std::string(*v);
    }

    std::string_view getHashSV() const {
        return std::string_view(reinterpret_cast<const char*>(hash.h), sizeof(hash.h));
    }

  private:
    void update(const CodedSymbol &other, int64_t inc) {
        if (other.val.size() > val.size()) val.resize(other.val.size(), '\0');
        for (size_t i = 0; i < other.val.size(); i++) val[i] ^= other.val[i];

        for (size_t i = 0; i < sizeof(hash.h); i++) hash.h[i] ^= other.hash.h[i];

        count += inc;
    }

    std::optional<std::string_view> decodeVal() const {
        auto v = std::string_view(val);

        if (v.size() < 4) throw herr("val unexpectedly small");
        auto size = from_sv<uint32_t>(v.substr(0, 4));
        v = v.substr(4);
        if (size > v.size()) return {};

        return v.substr(0, size);
    }
};


struct IndexGenerator {
    PRNG prng;
    uint64_t curr = 0;

    IndexGenerator(const CodedSymbol &sym) : prng(sym.hash.getPRNG()) {
    }

    void jump() {
        uint64_t rand = prng.rand64();

        // Magic formula from the RIBLT paper:
        curr += uint64_t(ceil((double(curr) + 1.5) * ((uint64_t(1)<<32)/std::sqrt(double(rand)+1) - 1)));
    }
};


struct SymbolQueue {
    struct QueueEntry {
        uint64_t codedStreamIndex;
        uint64_t symbolIndex;

        friend bool operator>(QueueEntry const &a, QueueEntry const &b) {
            return a.codedStreamIndex > b.codedStreamIndex;
        }
    };

    struct SymbolEntry {
        IndexGenerator gen;
        CodedSymbol sym;
    };

    std::vector<SymbolEntry> symbolVec;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> codedQueue;

    void enqueue(CodedSymbol &&sym, const IndexGenerator &gen) {
        codedQueue.emplace(gen.curr, symbolVec.size());
        symbolVec.emplace_back(gen, std::move(sym));
    }

    uint64_t nextCodedIndex() const {
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

    void add(CodedSymbol &&sym, const std::function<void(size_t)> &onSym = [](...){}) {
        IndexGenerator gen(sym);
        applySym(sym, gen, onSym);
        if (!doneExpanding) queue.enqueue(std::move(sym), gen);
    }

    void peel(const std::function<void(const CodedSymbol &sym)> &onSym) {
        std::vector<size_t> decodeable;

        for (; nextPeel < codedSymbols.size(); nextPeel++) {
            if (!codedSymbols[nextPeel].isPure()) continue;

            decodeable.push_back(nextPeel);

            while (decodeable.size()) {
                size_t index = decodeable.back();
                decodeable.pop_back();
                if (codedSymbols[index].isZero()) continue;

                onSym(codedSymbols[index]);

                auto sym = codedSymbols[index];
                sym.negate();
                add(std::move(sym), [&](size_t newIndex){
                    if (codedSymbols[newIndex].isPure()) decodeable.push_back(newIndex);
                });
            }
        }
    }

    bool isBalanced() {
        return codedSymbols.size() > 0 && codedSymbols[0].isZero();
    }

  private:
    void applySym(const CodedSymbol &sym, IndexGenerator &gen, const std::function<void(size_t)> &cb = [](size_t){}) {
        while (gen.curr < codedSymbols.size()) {
            codedSymbols[gen.curr].add(sym);
            cb(gen.curr);
            gen.jump();
        }
    }
};

}

using RIBLT = riblet::RIBLT;
