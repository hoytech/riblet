#pragma once

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <string>
#include <optional>

#include "golpe.h"

#include "ParseUtils.h"
#include "RIBLT.h"



/*
RibletFile := "RIBLT0" || <Header> || <CodedSymbol>*

Header := <headerLen (uint32LE)> || <TLV*>

TLV := <tag (Varint)> || <length (Varint)> || <value (bytes)>

CodedSymbol := <symbolLen (uint32LE)> || <countDelta (VarintZ)> || <hash (Byte[32])> || <value (bytes)>

Varint := <Digit+128>* <Digit>
*/

namespace riblet {





struct FileWriter {
    FILE *f = nullptr;
    int64_t prevCount = 0;
    uint64_t symbolsWritten = 0;
    uint64_t bytesWritten = 0;

    FileWriter(const std::string &filename, bool overwrite = false) {
        if (filename == "-") {
            f = ::stdout;
        } else {
            if (!overwrite && !access(filename.c_str(), X_OK)) throw herr("file '", filename, "' already exists");
            f = ::fopen(filename.c_str(), "wb");
            if (f == nullptr) throw herr("unable to open file '", filename, "' for writing: ", strerror(errno));
        }
    }

    ~FileWriter() {
        if (f) {
            fclose(f);
            f = nullptr;
        }
    }

    void writeHeader() {
        fwrite("RIBLT0\0\0\0\0", 1, 10, f);
        // FIXME: add header to bytesWritten
    }

    void writeSymbol(const CodedSymbol &sym) {
        int64_t countDelta = sym.count - prevCount;
        prevCount = sym.count;

        std::string encodedSym;

        encodedSym += encodeVarIntZ(countDelta);
        encodedSym += sym.getHashSV();
        encodedSym += sym.val;

        std::string sizeOutput = encodeUint32LE(encodedSym.size());

        fwrite(sizeOutput.data(), 1, sizeOutput.size(), f);
        fwrite(encodedSym.data(), 1, encodedSym.size(), f);

        symbolsWritten++;
        bytesWritten += sizeOutput.size() + encodedSym.size();
    }
};

struct FileReader {
    FILE *f = nullptr;
    int64_t currCount = 0;

    FileReader(const std::string &filename) {
        if (filename == "-") {
            f = ::stdin;
        } else {
            f = ::fopen(filename.c_str(), "rb");
            if (f == nullptr) throw herr("unable to open file '", filename, "' for reading: ", strerror(errno));
        }
    }

    ~FileReader() {
        if (f) {
            fclose(f);
            f = nullptr;
        }
    }

    void readHeader() {
        (void)readBytes(10);
    }

    std::optional<CodedSymbol> readSymbol() {
        auto symSizeOptional = readUint32LE();
        if (!symSizeOptional) return {};

        auto symSize = (size_t) *symSizeOptional;
        auto buffer = readBytes(symSize);
        auto v = std::string_view(buffer);

        int64_t countDelta = decodeVarIntZ(v);
        auto hash = getBytes(v, 32);

        currCount += countDelta;

        CodedSymbol sym(v, hash, currCount);

        return sym;
    }

  private:
    std::string readBytes(size_t n) {
        std::string buffer(n, '\0');

        size_t actuallyRead = fread(buffer.data(), 1, n, f);
        if (actuallyRead != n) throw herr("premature end of riblet stream");

        return buffer;
    }

    std::optional<uint32_t> readUint32LE() {
        std::string buffer(4, '\0');
        size_t actuallyRead = fread(buffer.data(), 1, 4, f);
        if (actuallyRead == 0) return {};
        else if (actuallyRead != 4) throw herr("premature end of riblet stream");

        auto v = std::string_view(buffer);
        size_t n = getUint32LE(v);
        return n;
    }
};

}