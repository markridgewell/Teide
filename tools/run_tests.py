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
data = json.loads(output)

num_failures = 0

test_env = os.environ.copy()
test_env["GTEST_COLOR"] = "1"

tests = data['tests']
sequence = []

def find_test(name):
    global tests
    for test in tests:
        if test['name'] == name:
            return test
    raise KeyError

def get_dependencies(test):
    if 'properties' not in test:
        return []
    for prop in test['properties']:
        if prop['name'] == 'DEPENDS':
            return prop['value']
    return []

def add_to_sequence(test_name):
    global sequence
    if test_name in sequence:
        return
    test = find_test(test_name)
    for dep in get_dependencies(test):
        add_to_sequence(dep)
    sequence += [test_name]

for test in tests:
    add_to_sequence(test['name'])

print('Test sequence:', sequence)
print()

for test_name in sequence:
    test = find_test(test_name)
    test_cmd = test['command']
    print(f'Running test {test_name}')
    # print(subprocess.list2cmdline(test_cmd), flush=True)
    retcode = subprocess.run(test_cmd, env=test_env).returncode
    if retcode != 0:
        num_failures = num_failures + 1
        print(f'Test {test_name} failed with exit code {retcode}')
    else:
        print(f'Test {test_name} passed')
    print()

print(f'Failed tests: {num_failures}')
exit(num_failures)
