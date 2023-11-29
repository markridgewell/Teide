import argparse
import sys
import hashlib
import shutil
from pathlib import Path

def make_fileset(path: Path) -> set[str]:
    return set(x.name for x in path.iterdir())

def all_same(items: list) -> bool:
    return all(x == items[0] for x in items)

def hash_file(path: Path) -> bytes:
    buf_size = 65536
    hash = hashlib.md5()
    with open(path, 'rb') as f:
        while data := f.read(buf_size):
            hash.update(data)
    return hash.digest()

parser = argparse.ArgumentParser()
parser.add_argument('output_dir', type=Path)
parser.add_argument('reference_dir', type=Path)
opts = parser.parse_args()

output_dirs = [x for x in opts.output_dir.iterdir() if x.stem.startswith('RenderTestOutput-')]
if not output_dirs:
    print("No render test outputs found in given directory")
    sys.exit(1)

print(f"Found {len(output_dirs)} render test output directories:")
for child in output_dirs:
    print(" -", child.name)
print()

filesets = [make_fileset(x) for x in output_dirs]
if not all_same(filesets):
    print("Render tests have differing results, cannot update references")
    sys.exit(1)

out_images = [x for x in filesets[0] if x.endswith('.out.png')]

print("All directories contain the same output images:")
for name in out_images:
    print(" -", name)
print()

for name in out_images:
    hashes = [hash_file(x / name) for x in output_dirs]
    if not all_same(hashes):
        print("Output images have differing contents, cannot update references")
        sys.exit(1)

print("All output images are identical")
print()

print("Updating references...")
for name in out_images:
    ref_name = name.removesuffix('.out.png') + '.png'
    source = output_dirs[0] / name
    dest = opts.reference_dir / ref_name
    shutil.copy(source, dest)
