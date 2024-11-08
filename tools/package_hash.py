
import argparse
import tempfile
import subprocess
import http.client
import sys
from pathlib import Path
from urllib.request import urlretrieve


args = argparse.ArgumentParser()
args.add_argument('repo')
args.add_argument('ref')
opts = args.parse_args()

with tempfile.TemporaryDirectory() as dirname:
    url = f'https://github.com/{opts.repo}/archive/{opts.ref}.tar.gz'
    filename = Path(dirname) / 'file.tar.gz'
    try:
        response = urlretrieve(url, filename)
    except http.client.IncompleteRead:
        print("Error downloading file. Please try again.")
        sys.exit(1);
    print(subprocess.run(['external/vcpkg/vcpkg', 'hash', filename], capture_output=True).stdout.decode("utf-8").rstrip())
