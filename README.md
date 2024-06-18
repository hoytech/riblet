    riblet build -n 1000 file.csv
      -> creates file.csv.riblet with 1k coded symbols

    riblet build --stdout file.csv
      -> infinite stream to stdout

    riblet diff file.csv <(curl https://example.com/file.csv.riblet)

    riblet sync [--add-only] [--del-only] file.csv <(curl https://example.com/file.csv.riblet)


    riblet check file.csv

    riblet combine [--subtract] file.csv file2.csv

    riblet dump file.csv
