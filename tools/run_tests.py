#!/usr/bin/env python

import sys
import subprocess
import json
import os

if len(sys.argv) != 2:
    print(f'usage: {sys.argv[0]} <test-dir>')
    exit(1)

test_dir = sys.argv[1]

cmd = ['ctest', '--test-dir', test_dir, '--build-config', 'Debug', '--no-tests=error', '--show-only=json-v1']
output = subprocess.check_output(cmd, encoding='utf-8')
tests = json.loads(output)['tests']

exit_code = 0

test_env = os.environ.copy()
test_env["GTEST_COLOR"] = "1"

for test in tests:
    test_name = test['name']
    test_cmd = test['command']
    print(f'Running test {test_name}')
    print(subprocess.list2cmdline(test_cmd), flush=True)
    retcode = subprocess.run(test_cmd, env=test_env).returncode
    if retcode != 0:
        exit_code = 1
        print(f'Test {test_name} failed with exit code {retcode}')
    else:
        print(f'Test {test_name} passed')
    print()

exit(exit_code)
