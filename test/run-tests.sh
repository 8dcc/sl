#!/bin/bash

msg() {
    echo -e "\033[32;1m$1\033[0m"
}

err() {
    echo -e "\033[31;1m$1\033[0m" 1>&2
}

file_msg() {
    echo -e "\033[34;1m$1:\033[37;1m $2\033[0m"
}

file_err() {
    echo -e "\033[31;1m$1:\033[37;1m $2\033[0m" 1>&2
}

remove_colors() {
    # shellcheck disable=SC2001
    echo "$1" | sed 's/\x1B\[[0-9;]\{1,\}[A-Za-z]//g'
}

if [ ! "$(command -v dirname)" ] ||
       [ ! "$(command -v readlink)" ] ||
       [ ! "$(command -v valgrind)" ]; then
    err "Missing dependencies. Exiting..."
    exit 1
fi

SCRIPT_DIR=$(dirname -- "$(readlink -f -- "${BASH_SOURCE[0]}")")
SL_BIN="${SCRIPT_DIR}/../sl"
SL_FLAGS=(--no-stdlib --silent "${SCRIPT_DIR}/../stdlib.lisp")
DIFF_FLAGS=(--unified=0 --color)

for file in "$SCRIPT_DIR"/*.lisp; do
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
                 "$SL_BIN" "${SL_FLAGS[@]}" "$file" > /dev/null
    valgrind_code=$?

    echo "-------------------------------------------------------------------"

    if [ $valgrind_code -ne 0 ]; then
        file_err "Detected valgrind error when parsing" "$file"
        exit 1
    fi

    normal_output="$(echo -e "$input_str" | "$SL_BIN" "${SL_FLAGS[@]}" "$file" 2>&1 | sed "s/<primitive 0x[[:xdigit:]]\+>/<primitive 0xDEADBEEF>/g")"
    desired_output_file="${file}.expected"

    # FIXME: Don't call 'diff' twice, but still show colors when printing.
    diff <(remove_colors "$normal_output") "$desired_output_file" &>/dev/null
    diff_code=$?
    if [ $diff_code -eq 1 ]; then
        err "Output mismatch. Showing differences and stopping..."
        diff "${DIFF_FLAGS[@]}" <(echo "$normal_output") "$desired_output_file"
        exit 1
    elif [ $diff_code -ge 2 ]; then
        err "Error when running 'diff', aborting..."
        exit 1
    fi
done

msg "No errors reported from valgrind."
