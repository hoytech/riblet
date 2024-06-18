#include <fstream>
#include <iostream>
#include <memory>

#include <docopt.h>
#include "golpe.h"

#include "RIBLT.h"
#include "FileFormat.h"


static const char USAGE[] =
R"(
    Usage:
      build [--num=<num-syms>] [--size=<size-bytes>] [--rebuild] <input> [<output>]
)";



void cmd_build(const std::vector<std::string> &subArgs) {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE, subArgs, true, "");

    std::string inputSource = args["<input>"].asString();

    std::string outputSink = inputSource + ".riblet";


    std::shared_ptr<std::istream> input;

    if (inputSource == "-") {
        input.reset(&std::cin, [](...){});
    } else {
        input.reset(new std::ifstream(inputSource.c_str()));
    }



    RIBLT r;

    std::string line;

    while (*input) {
        std::getline(*input, line);
        if (line.size() == 0) continue;
        LW << line;
        r.add(riblet::CodedSymbol(line));
    }

    r.expand(1000);


    std::ifstream output(outputSink.c_str(), std::ios_base::out | std::ios_base::binary );

    for (size_t i = 0; i < r.codedSymbols.size(); i++) {
    }
}
