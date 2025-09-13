#!/bin/bash

if [[ $# -ne 1 ]]; then
    echo "usage: $0 <group-name>" >&2
    exit 1
fi

function group() {
    echo "::group::$1"
}

function endgroup() {
    echo "::endgroup::"
}

