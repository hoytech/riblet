    riblet build file.csv
      -> creates file.csv.riblet

    riblet diff file.csv <(curl https://example.com/file.csv.riblet)

    riblet sync [--add-only] [--del-only] file.csv <(curl https://example.com/file.csv.riblet)


    riblet check file.csv

    riblet combine [--subtract] file.csv file2.csv

    riblet dump file.csv
