#include <fstream>
#include <iostream>
#include <memory>

#include <docopt.h>
#include <parallel_hashmap/phmap.h>
#include "golpe.h"

#include "ParseUtils.h"
#include "FileFormat.h"
#include "RIBLT.h"


static const char USAGE[] =
R"(
    Usage:
      build [--num=<num-syms>] [--rebuild] [--dup-check] [--dup-skip] <input> [<output>]
)";



void cmd_build(const std::vector<std::string> &subArgs) {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE, subArgs, true, "");

    std::string inputFile, outputFile;
    uint64_t numSyms = MAX_U64;

    if (!args["<input>"]) throw herr("must provide <input> argument (can be '-' for stdin)");
    inputFile = args["<input>"].asString();

    if (args["<output>"]) {
        outputFile = args["<output>"].asString();
    } else {
        if (inputFile == "-") throw herr("if <input> is '-' (stdin), an <output> is required (can also be '-' for stdout)");
        outputFile = inputFile + ".riblet";
    }

    if (args["--num"]) numSyms = args["--num"].asLong();

    if (outputFile != "-" && numSyms == MAX_U64) throw herr("can only use infinite symbol generation when the output is '-' (stdout)");


    phmap::flat_hash_set<riblet::Hash> seenHashes;
    bool dupCheck = args["--dup-check"].asBool();
    bool dupSkip = args["--dup-skip"].asBool();

    if (dupCheck && dupSkip) throw herr("pick only one of --dup-check and --dup-check");


    std::shared_ptr<std::istream> inputStream;

    if (inputFile == "-") {
        inputStream.reset(&std::cin, [](...){});
    } else {
        inputStream.reset(new std::ifstream(inputFile.c_str()));
    }


    RIBLT r;

    if (numSyms != MAX_U64) {
        r.expand(numSyms);
        r.setDoneExpanding();
    }

    std::string line;
    uint64_t numRecs = 0, lineNum = 0;

    while (*inputStream) {
        std::getline(*inputStream, line);
        if (line.size() == 0) continue;
        lineNum++;

        riblet::CodedSymbol cs(line);
        if (dupCheck || dupSkip) {
            if (seenHashes.contains(cs.hash)) {
                if (dupCheck) throw herr("duplicate detected on line ", lineNum);
                else continue;
            }
            seenHashes.insert(cs.hash);
        }

        numRecs++;
        r.add(std::move(cs));
    }


    riblet::FileWriter output(outputFile, args["--rebuild"].asBool());

    {
        std::vector<riblet::HeaderElem> headerElems = {
            { riblet::HEADER_TAG__BUILD_TIMESTAMP, encodeVarInt(::time(nullptr)) },
            { riblet::HEADER_TAG__INPUT_FILENAME, inputFile },
            { riblet::HEADER_TAG__INPUT_RECORDS, encodeVarInt(numRecs) },
        };

        if (numSyms != MAX_U64) {
            headerElems.push_back({ riblet::HEADER_TAG__OUTPUT_SYMBOLS, encodeVarInt(numSyms) });
        }

        output.writeHeader(headerElems);
    }


    if (numSyms == MAX_U64) {
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
