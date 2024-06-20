#include <docopt.h>
#include "golpe.h"

#include "RIBLT.h"
#include "FileFormat.h"


static const char USAGE[] =
R"(
    Usage:
      dump [--header-only] [--symbols-only] <input>
)";



void cmd_dump(const std::vector<std::string> &subArgs) {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE, subArgs, true, "");

    std::string inputSource = args["<input>"].asString();

    riblet::FileReader input(inputSource);


    auto headerElems = input.readHeader();

    if (!args["--symbols-only"].asBool()) {
        std::cout << "RIBLT Header" << "\n";
        for (const auto &e : headerElems) {
            if (e.tag == riblet::HEADER_TAG__BUILD_TIMESTAMP) {
                std::cout << "  Build Timestamp = " << decodeVarInt(e.value) << "\n";
            } else if (e.tag == riblet::HEADER_TAG__INPUT_FILENAME) {
                std::cout << "  Input Filename = \"" << e.value << "\"\n";
            } else if (e.tag == riblet::HEADER_TAG__INPUT_RECORDS) {
                std::cout << "  Input Records = " << decodeVarInt(e.value) << "\n";
            } else if (e.tag == riblet::HEADER_TAG__OUTPUT_SYMBOLS) {
                std::cout << "  Output Symbols = " << decodeVarInt(e.value) << "\n";
            } else {
                std::cout << "  Unknown tag(" << e.tag << ") = 0x" << to_hex(e.value) << "\n";
            }
        }

        std::cout << "-----------------------\n";
    }

    if (args["--header-only"].asBool()) return;

    uint64_t i = 0;

    while (1) {
        auto symOptional = input.readSymbol();
        if (!symOptional) break;
        auto &s = *symOptional;

        std::cout << "Symbol " << i << "\n";
        std::cout << "  val = '" << to_hex(s.val) << "'" << "\n";
        std::cout << "  hash = '" << to_hex(s.hash.sv()) << "'" << "\n";
        std::cout << "  count = " << s.count << "\n";

        i++;
    }
}
