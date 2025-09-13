#!/bin/bash

if [[ $# -ne 1 ]]; then
    echo usage: $0 preset
    exit 1
fi
preset=$1

function get_logs {
    for file in $(grep '^  [^\s].*\.log$' $1); do
        echo ${file}
        get_logs ${file}
    done
}

log_files=$(get_logs build/${preset}/vcpkg-manifest-install.log | sort | uniq)

. .github/scripts/with_group.sh

for file in $log_files; do
    if [ -s ${file} ]; then
        # print the file contents, excluding any line containing 'TryCompile-'
        # this is to reduce the number of false positive errors
        group "${file}"
        sed -n '/TryCompile-/!p' ${file}
        endgroup
    fi
done
