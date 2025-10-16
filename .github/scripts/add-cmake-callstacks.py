
#!/usr/bin/env python

import sys
import json
import re

from pathlib import Path
from typing import Iterable

# Patterns
WARNING_OR_ERROR = re.compile('^CMake (Warning|Error) at (.*):([1-9][0-9]*).*$')
STACK_FRAME = re.compile('^[ \t]*(.*):([1-9][0-9]*) (.*)$')


def realpath(uri: str) -> Path:
    p = Path(uri)
    if not p.exists():
        raise RuntimeError(f'Path "{uri}" does not exist')
    return p.absolute()


def make_physicalLocation(uri: Path, startLine: int):
    return {
        'artifactLocation': {
            'uri': str(uri),
        },
        'region': {
            'startLine': startLine
        }
    }


def make_location(uri: Path, startLine: int, message: str|None = None):
    pl = make_physicalLocation(uri, startLine)
    if message:
        return {
            'physicalLocation': pl,
            'message': {
                'text': message
            }
        }
    else:
        return { 'physicalLocation': pl }


def is_same_location(location, uri: Path, startLine: int):
    try:
        pl = location['physicalLocation']
        loc_startLine = pl['region']['startLine']
        loc_uri = pl['artifactLocation']['uri']
        return uri.samefile(loc_uri) and startLine == loc_startLine
    except KeyError:
        return False


class State:
    def __init__(self, initial_parser):
        self._initial_parser = initial_parser
        self._parser = initial_parser

    def set_parser(self, parser):
        self._parser = parser

    def reset_parser(self):
        self._parser = self._initial_parser


class OutputParser:
    def __init__(self, sarif_data):
        assert(sarif_data)
        self.sarif_data = sarif_data

    def __call__(self, line: str, state: State):
        match = re.match(WARNING_OR_ERROR, line)
        if not match:
            return

        severity, uri, startLine = match.groups()
        assert(severity in ['Warning', 'Error'])
        uri = realpath(uri)
        startLine = int(startLine)

        result = self.find_result_at_location(uri, startLine)
        if result == None:
            raise RuntimeError(f'No result found for location {uri}:{startLine}')

        state.set_parser(MessageParser(result))

    def find_result_at_location(self, uri: Path, startLine: int):
        for run in self.sarif_data.get('runs', []):
            if not run: continue
            for result in run.get('results', []):
                for location in result.get('locations', []):
                    if is_same_location(location, uri, startLine):
                        return result


class MessageParser:
    def __init__(self, result):
        assert(result)
        self.result = result

    def __call__(self, line: str, state: State):
        if line.strip() == '':
            state.reset_parser()
            return

        if not line.startswith('Call Stack '):
            return

        assert(self.result)
        if 'stacks' not in self.result:
            self.result['stacks'] = []
        self.result['stacks'].append({
            'message': {
                'text': line.rstrip()
            },
            'frames': []
        })
        state.set_parser(StackParser(self.result['stacks'][-1]))


class StackParser:
    def __init__(self, stack):
        assert(stack)
        self.stack = stack

    def __call__(self, line: str, state: State):
        if line.strip() == '':
            state.reset_parser()
            return

        match = re.match(STACK_FRAME, line)
        if not match:
            return

        uri, startLine, message = match.groups()
        uri = realpath(uri)
        startLine = int(startLine)

        self.stack['frames'] += [{
            'location': make_location(uri, startLine, message),
        }]


def extract_callstacks(output: Iterable[str], sarif_data):
    top_level = OutputParser(sarif_data)
    state = State(top_level)
    for line in output:
        state._parser(line, state)


def main():
    with open(sys.argv[1]) as f:
        sarif_data = json.loads(f.read())

    with open(sys.argv[2]) as f:
        extract_callstacks(f.readlines(), sarif_data)

    print(json.dumps(sarif_data, indent=2))


if __name__ == '__main__':
    main()

