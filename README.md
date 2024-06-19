    riblet build -n 1000 file.csv
      -> creates file.csv.riblet with 1k coded symbols

    riblet build file.csv -
      -> infinite stream to stdout

    riblet diff file.csv <(curl https://example.com/file.csv.riblet)

    riblet sync [--add-only] [--del-only] file.csv <(curl https://example.com/file.csv.riblet)

    riblet dump file.csv


    ...maybe

    riblet check file.csv

    riblet combine [--subtract] file.csv file2.csv
        have to be same size streams? or just output min(file, file2)?



TODO

docs

tests

optional dup checking during build

compression
  built-in, or piped to zstd/gzip?

build output, restricted to symbols/output size

verbose mode
  lists how many symbols/bytes needed

header fields
  build timestamp
  filename
  num input records
  input size
  input hash
  num output symbols

better prng

add hashes instead of xor?

doneAdding in RIBLT: clears the codedSymbols vector. maybe useful for '-' output?

-z for NUL separator instead of newline
