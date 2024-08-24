#!/bin/bash

msg() {
    if [ $(command -v tput) ]; then
        bold=$(tput bold)
        normal=$(tput sgr0)
    else
        bold=""
        normal=""
    fi

    echo -e "${bold}run-tests.sh: $1${normal}"
}

if [ ! $(command -v dirname) ] ||
       [ ! $(command -v readlink) ] ||
       [ ! $(command -v valgrind) ]; then
    msg "Missing dependencies. Exiting..."
    exit 1
fi

SCRIPT_DIR=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")
SL_BIN="$SCRIPT_DIR/../sl"

for file in $(ls "$SCRIPT_DIR"/*.lisp); do
    msg "Testing: $file"

    valgrind --leak-check=full   \
             --track-origins=yes \
             --error-exitcode=1  \
             $SL_BIN < $file

    echo "-------------------------------------------------------------------"

    if [ $? -ne 0 ]; then
        msg "Stopping. Detected valgrind error when parsing: $file"
        exit 1
    fi
done

msg "No errors reported from valgrind."
