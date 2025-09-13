#!/bin/bash

if [[ $# -ne 2 ]]; then
    echo "usage: $0 <group-name> <command>" >&2
    exit 1
fi

echo "::group::$1"
$2
echo "::endgroup::"

