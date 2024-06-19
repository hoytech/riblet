#include <docopt.h>
#include "golpe.h"

#include "RIBLT.h"
#include "FileFormat.h"


static const char USAGE[] =
R"(
    Usage:
      dump <input>
)";



void cmd_dump(const std::vector<std::string> &subArgs) {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE, subArgs, true, "");

    std::string inputSource = args["<input>"].asString();

    riblet::FileReader input(inputSource);

    input.readHeader();

    uint64_t i = 0;

    while (1) {
        auto symOptional = input.readSymbol();
        if (!symOptional) break;
        auto &s = *symOptional;

        LW << "CS " << i;
        LW << "  val = '" << to_hex(s.val) << "'";
        LW << "  hash = '" << to_hex(s.getHashSV()) << "'";
        LW << "  count = " << s.count;

        i++;
    }
}
