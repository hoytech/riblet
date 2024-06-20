    riblet build -n 1000 file.csv
      -> creates file.csv.riblet with 1k coded symbols

    riblet build file.csv -
      -> infinite stream to stdout

    riblet diff [--symmetric] file.csv <(curl https://example.com/file.csv.riblet)

    riblet sync [--add-only] [--del-only] file.csv <(curl https://example.com/file.csv.riblet)

    riblet dump file.csv


    ...maybe

    riblet check file.csv

    riblet combine [--subtract] file.csv file2.csv
        have to be same size streams? or just output min(file, file2)?



TODO HIGH

docs

tests

sum hashes instead of xor?

compression
  built-in, or piped to zstd/gzip?

header fields
  build timestamp
  filename
  num input records
  input size
  input hash
  num output symbols
  compression?



TODO LOW

optional dup checking during build

doneAdding in RIBLT: clears the codedSymbols vector as they are generated. maybe useful for '-' output?

-z for NUL separator instead of newline

verbose mode
  lists how many symbols/bytes needed
