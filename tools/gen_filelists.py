
import os
import subprocess

def is_source_file(filename):
    return os.path.splitext(filename)[1] in [".h", ".cpp", ".natvis"]

def add_source_files(source_files, dir, subdir):
    cwd = os.getcwd()
    os.chdir(dir)
    for root, dirs, files in os.walk(subdir):
        root = root.replace(os.sep, '/')
        for file in files:
            if is_source_file(file):
                source_files += [root + "/" + file]
    os.chdir(cwd)

def gen_filelist(dir):
    source_files = []
    add_source_files(source_files, dir, "src")
    add_source_files(source_files, dir, "include")
    source_files.sort()

    test_files = []
    add_source_files(test_files, dir, "tests")
    test_files.sort()

    outfilename = os.path.join(dir, "FileLists.cmake")
    with open(outfilename, "w") as outfile:
        outfile.write("set(sources")
        outfile.write("\n    ")
        outfile.write("\n    ".join(source_files) + ")\n")
        outfile.write("\n")
        outfile.write("set(test_sources")
        outfile.write("\n    ")
        outfile.write("\n    ".join(test_files) + ")\n")

    subprocess.run(["cmake-format", "-i", outfilename])

if __name__ == "__main__":
    for root, dirs, files in os.walk("."):
        if "CMakeLists.txt" in files:
            gen_filelist(root)
