#!/bin/bash

msg() {
    echo -e "\033[32;1m$1\033[0m"
}

err() {
    echo -e "\033[31;1m$1\033[0m"
}

file_msg() {
    echo -e "\033[34;1m$1:\033[37;1m $2\033[0m"
}

file_err() {
    echo -e "\033[31;1m$1:\033[37;1m $2\033[0m"
}

if [ ! $(command -v dirname) ] ||
       [ ! $(command -v readlink) ] ||
       [ ! $(command -v valgrind) ]; then
    err "Missing dependencies. Exiting..."
    exit 1
fi

SCRIPT_DIR=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")
SL_BIN="$SCRIPT_DIR/../sl"

for file in $(ls "$SCRIPT_DIR"/*.lisp); do
    file_msg "Testing" "$file"

    input_str=""
    if [ "$(basename "$file")" == "io.lisp" ]; then
        input_str+="123"
        input_str+="(+ 1 2 3 (- 5 4))"
        input_str+="User string...\n"
        input_str+="Another delimited line. EXTRA"
    fi

    echo -e "$input_str" | \
        valgrind --leak-check=full   \
                 --track-origins=yes \
                 --error-exitcode=1  \
                 $SL_BIN $file
    valgrind_code=$?

    echo "-------------------------------------------------------------------"

    if [ $valgrind_code -ne 0 ]; then
        file_err "Detected valgrind error when parsing" "$file"
        exit 1
    fi
done

msg "No errors reported from valgrind."
