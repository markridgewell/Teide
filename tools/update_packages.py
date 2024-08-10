
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

    old_release = git(["describe", "--tags"], cwd=vcpkg_dir)
    old_commit = git(["rev-parse", "HEAD"], cwd=vcpkg_dir)
    git(["switch", "master"], cwd=vcpkg_dir)
    git(["pull"], cwd=vcpkg_dir)
    new_release = git(["describe", "--tags", "--abbrev=0"], cwd=vcpkg_dir)
    git(["switch", "--detach", new_release])
    new_commit = git(["rev-parse", "HEAD"], cwd=vcpkg_dir)

    if old_commit != new_commit:
        modify_json(new_commit)
        print(f"Updated vcpkg from {old_release} to {new_release}\nBaseline is now {new_commit}")


if __name__ == "__main__":
    main()
