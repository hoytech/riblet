#include <iostream>

#include <docopt.h>
#include "golpe.h"

#include "pcg_random.hpp"


static const char USAGE[] =
R"(
    Usage:
      encode
)";



struct IndexGenerator {
    uint8_t hash[16];
    uint64_t prev;
};


struct CodedSymbol {
    std::string valAccum;
    uint8_t hashAccum[16] = {0};
    uint64_t count = 0;
};

struct Encoder {
    std::vector<CodedSymbol> codedSymbols;

    void allocate(uint64_t n) {
        codedSymbols.resize(n);
    }

    void add(std::string_view val) {
        // Hash
        // Make PGC64 instance
        // Iterate PRNG until index > codedSymbols
        //   append onto codedSymbols
    }
};

struct Decoder {
};


void cmd_encode(const std::vector<std::string> &subArgs) {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE, subArgs, true, "");

    uint64_t seed[2] = {1,3};
    pcg64 rng(seed[0], seed[1]);

    LW << rng();

/*
    std::string line;

    while (std::cin) {
        std::getline(std::cin, line);
    }
    */
}
