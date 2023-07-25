
import subprocess
import json
from pathlib import Path


def git(args, cwd=None):
    return subprocess.run(["git"] + args, cwd=cwd, capture_output=True).stdout.decode("utf-8").rstrip()


def modify_json(new_commit):
    with open("vcpkg.json", "r") as fp:
        contents = json.load(fp)

    contents["builtin-baseline"] = new_commit

    with open("vcpkg.json", "w") as fp:
        json.dump(contents, fp, indent=2)
        fp.write("\n")


def main():
    vcpkg_dir = Path("external", "vcpkg")

    old_commit = git(["rev-parse", "HEAD"], cwd=vcpkg_dir)
    git(["checkout", "master"], cwd=vcpkg_dir)
    git(["pull"], cwd=vcpkg_dir)
    new_commit = git(["rev-parse", "HEAD"], cwd=vcpkg_dir)

    if old_commit != new_commit:
        modify_json(new_commit)
        print(f"Updated vcpkg baseline from {old_commit} to {new_commit}")


if __name__ == "__main__":
    main()
