#include <iostream>

#include <docopt.h>
#include "golpe.h"

#include "RIBLT.h"
#include "FileFormat.h"


static const char USAGE[] =
R"(
    Usage:
      diff <inputA> <inputB>
)";



void cmd_diff(const std::vector<std::string> &subArgs) {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE, subArgs, true, "");

    std::string inputSourceA = args["<inputA>"].asString();
    std::string inputSourceB = args["<inputB>"].asString();

    riblet::FileReader inputA(inputSourceA);
    riblet::FileReader inputB(inputSourceB);

    inputA.readHeader();
    inputB.readHeader();

    RIBLT r;

    while (1) {
        auto symAOptional = inputA.readSymbol();
        auto symBOptional = inputB.readSymbol();

        if (!symAOptional) throw herr("insufficient coded symbols from source A");
        if (!symBOptional) throw herr("insufficient coded symbols from source B");

        auto &symA = *symAOptional;
        auto &symB = *symBOptional;

        symA.sub(symB);
        r.push(symA);

        r.peel([](const auto &s){
            std::cout << (s.count == 1 ? '-' : '+') << s.getVal() << "\n";
        });

        if (r.isBalanced()) break;
    }
}
