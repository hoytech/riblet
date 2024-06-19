#include <string.h>

#include <cmath>
#include <iostream>
#include <queue>
#include <functional>

#include <openssl/sha.h>

#include <docopt.h>
#include "golpe.h"

#include "RIBLT.h"


static const char USAGE[] =
R"(
    Usage:
      encode
)";





void cmd_encode(const std::vector<std::string> &subArgs) {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE, subArgs, true, "");

    RIBLT e1;
    RIBLT e2;

    e1.add(riblet::CodedSymbol("a"));
    e1.add(riblet::CodedSymbol("c"));
    e1.add(riblet::CodedSymbol("d"));
    e1.add(riblet::CodedSymbol("zz"));

    e2.add(riblet::CodedSymbol("a"));
    e2.add(riblet::CodedSymbol("b"));
    e2.add(riblet::CodedSymbol("c"));
    e2.add(riblet::CodedSymbol("d"));

    e1.expand(100);
    e2.expand(100);

    RIBLT e;

    for (size_t i = 0; i < 15; i++) {
        auto sym = e1.codedSymbols[i];
        sym.sub(e2.codedSymbols[i]);
        e.push(sym);

        e.peel([](const auto &s){
            LW << "PEELED: " << s.getVal() << " / " << s.count;
        });

        if (e.isBalanced()) break;
    }

    if (!e.isBalanced()) LE << "Couldn't peel";


    /*
    RIBLT e;

    e.add(CodedSymbol("a"));
    e.add(CodedSymbol("b"));
    e.add(CodedSymbol("c"));
    e.add(CodedSymbol("d"));

    e.expand(7);
    e.peel([](const auto &s){
        LW << "PEELED: " << s.getVal() << " / " << s.count;
    });
    */

    /*
    std::string line;

    while (std::cin) {
        std::getline(std::cin, line);
        if (line.size() == 0) continue;
        e.add(CodedSymbol(line));
    }

    e.expand(20);
    */

    for (size_t i = 0; i < 15; i++) {
        const auto &s = e1.codedSymbols[i];
        LW << "CS " << i;
        LW << "  val = '" << to_hex(s.val) << "'";
        LW << "  hash = '" << to_hex(s.getHashSV()) << "'";
        LW << "  count = " << s.count;
    }
}
