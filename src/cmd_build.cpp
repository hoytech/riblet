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
      build [--num=<num-syms>] [--rebuild] <input> [<output>]
)";



void cmd_build(const std::vector<std::string> &subArgs) {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE, subArgs, true, "");

    std::string inputFile, outputFile;
    uint64_t numRecs = MAX_U64;

    if (!args["<input>"]) throw herr("must provide <input> argument (can be '-' for stdin)");
    inputFile = args["<input>"].asString();

    if (args["<output>"]) {
        outputFile = args["<output>"].asString();
    } else {
        if (inputFile == "-") throw herr("if <input> is '-' (stdin), an <output> is required (can also be '-' for stdout)");
        outputFile = inputFile + ".riblet";
    }

    if (args["--num"]) numRecs = args["--num"].asLong();

    if (outputFile != "-" && numRecs == MAX_U64) throw herr("can only use infinite symbol generation when the output is '-' (stdout)");



    std::shared_ptr<std::istream> inputStream;

    if (inputFile == "-") {
        inputStream.reset(&std::cin, [](...){});
    } else {
        inputStream.reset(new std::ifstream(inputFile.c_str()));
    }


    RIBLT r;

    if (numRecs != MAX_U64) {
        r.expand(numRecs);
        r.setDoneExpanding();
    }

    std::string line;

    while (*inputStream) {
        std::getline(*inputStream, line);
        if (line.size() == 0) continue;
        r.add(riblet::CodedSymbol(line));
    }


    riblet::FileWriter output(outputFile, args["--rebuild"].asBool());

    output.writeHeader();


    if (numRecs == MAX_U64) {
        size_t curr = 0;

        while (1) {
            r.expand(100);
            for (; curr < r.codedSymbols.size(); curr++) {
                output.writeSymbol(r.codedSymbols[curr]);
            }
        }
    } else {
        for (size_t i = 0; i < r.codedSymbols.size(); i++) {
            output.writeSymbol(r.codedSymbols[i]);
        }
    }
}
