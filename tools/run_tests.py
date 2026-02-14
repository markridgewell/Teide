#!/usr/bin/env python

import sys
import subprocess
import json

if len(sys.argv) != 2:
    print(f'usage: {sys.argv[0]} <test-dir>')
    exit(1)

test_dir = sys.argv[1]

cmd = ['ctest', '--test-dir', test_dir, '--build-config', 'Debug', '--no-tests=error', '--show-only=json-v1']
output = subprocess.check_output(cmd, encoding='utf-8')
tests = json.loads(output)['tests']

failure = False

for test in tests:
    test_cmd = test['command']
    print(subprocess.list2cmdline(test_cmd))
    retcode = subprocess.run(test_cmd)
    if retcode != 1:
        failure = True
    print()

if failure:
    exit(1)
