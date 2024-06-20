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


symbolLen == 0 means zero symbol
*/

namespace riblet {


const uint64_t HEADER_TAG__BUILD_TIMESTAMP = 1;
const uint64_t HEADER_TAG__INPUT_FILENAME = 2;
const uint64_t HEADER_TAG__INPUT_RECORDS = 3;
const uint64_t HEADER_TAG__INPUT_BYTES = 4;
const uint64_t HEADER_TAG__INPUT_HASH = 5;
const uint64_t HEADER_TAG__OUTPUT_SYMBOLS = 6;

const uint64_t HEADER_TAG__COMPRESSED_ZSTD = 100;



struct HeaderElem {
    uint64_t tag;
    std::string value;
};


struct FileWriter {
    FILE *f = nullptr;
    int64_t prevCount = 0;
    uint64_t symbolsWritten = 0;

    FileWriter(const std::string &filename, bool overwrite = false) {
        if (filename == "-") {
            f = ::stdout;
        } else {
            if (!overwrite && !access(filename.c_str(), F_OK)) throw herr("file '", filename, "' already exists: use --rebuild to overwrite");
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

    void writeHeader(const std::vector<HeaderElem> &elems = {}) {
        std::string headerPayload;

        for (const auto &e : elems) {
            headerPayload += encodeVarInt(e.tag);
            headerPayload += encodeVarInt(e.value.size());
            headerPayload += e.value;
        }

        std::string header = "RIBLT0";
        header += encodeUint32LE(headerPayload.size());
        header += headerPayload;

        fwrite(header.data(), 1, header.size(), f);
    }

    void writeSymbol(const CodedSymbol &sym) {
        if (sym.isZero()) {
            std::string output = encodeUint32LE(0);
            fwrite(output.data(), 1, output.size(), f);
            symbolsWritten++;
            return;
        }

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
    }
};

struct FileReader {
    FILE *f = nullptr;
    int64_t currCount = 0;
    size_t maxReadSize = 1024 * 1024;

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

    std::vector<HeaderElem> readHeader() {
        if (readBytes(6) != "RIBLT0") throw herr("not a RIBLT file");

        auto headerSizeOpt = readUint32LE();
        if (!headerSizeOpt) throw herr("unable to read header size");
        std::string headerStr = readBytes(*headerSizeOpt);
        auto h = std::string_view(headerStr);

        std::vector<HeaderElem> output;

        while (h.size()) {
            uint64_t tag = decodeVarInt(h);
            uint64_t length = decodeVarInt(h);
            std::string value = getBytes(h, length);

            output.emplace_back(tag, std::move(value));
        }

        return output;
    }

    std::optional<CodedSymbol> readSymbol() {
        auto symSizeOptional = readUint32LE();
        if (!symSizeOptional) return {};

        auto symSize = (size_t) *symSizeOptional;

        if (symSize == 0) return CodedSymbol();

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
        if (n > maxReadSize) throw herr("riblet record read too big: ", n);
        return n;
    }
};

}
