#!/usr/bin/env bash
#
# Copyright 2025 8dcc
#
# This file is part of SL.
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <https://www.gnu.org/licenses/>.

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

dependencies=('dirname' 'readlink' 'valgrind')
for dependency in "${dependencies[@]}"; do
    if [ ! "$(command -v "$dependency")" ]; then
        err "The '$dependency' command is required. Exiting..."
        exit 1
    fi
done

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
