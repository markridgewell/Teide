
import os
import subprocess
import sys
import time
import threading
import filecmp


def is_source_file(filename):
    return os.path.splitext(filename)[1] in [".h", ".cpp", ".natvis"]


def add_source_files(source_files, dir, subdir):
    cwd = os.getcwd()
    os.chdir(dir)
    for root, dirs, files in os.walk(subdir):
        root = root.replace(os.sep, '/')
        for file in files:
            if is_source_file(file):
                new_file = root + "/" + file
                if ' ' in new_file:
                    new_file = f'"{new_file}"'
                source_files += [new_file]
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
    tempfilename = os.path.join(dir, "FileLists.cmake.temp")
    with open(tempfilename, "w") as f:
        f.write("set(sources")
        f.write("\n    ")
        f.write("\n    ".join(source_files) + ")\n")
        f.write("\n")
        f.write("set(test_sources")
        f.write("\n    ")
        f.write("\n    ".join(test_files) + ")\n")

    subprocess.run(["cmake-format", "-i", tempfilename], shell=True)

    if filecmp.cmp(outfilename, tempfilename, shallow=False):
        os.remove(tempfilename)
    else:
        os.remove(outfilename)
        os.rename(tempfilename, outfilename)
        print("Updated file list for", dir)


def watch_subdir(observer, event_handler, subdir):
    subpath = os.path.join(event_handler.dir, subdir)
    if os.path.exists(subpath):
        print(f"Watching subdir {subpath}")
        observer.schedule(event_handler, subpath, recursive=True)


def add_watch(observer, path):
    from watchdog.events import FileSystemEventHandler

    class FileWatcher(FileSystemEventHandler):
        def __init__(self, dir):
            print(f"Created a watcher for {dir}")
            self.dir = dir
            self.timer = None

        def on_created(self, event):
            if is_source_file(event.src_path):
                self._handle_event(event)

        def on_deleted(self, event):
            if event.is_directory or is_source_file(event.src_path):
                self._handle_event(event)

        def on_moved(self, event):
            if is_source_file(event.src_path) or is_source_file(event.dest_path):
                self._handle_event(event)

        def _handle_event(self, event):
            print(event)
            if self.timer:
                self.timer.cancel()
            self.timer = threading.Timer(10.0, gen_filelist, args=[self.dir])
            self.timer.start()

    event_handler = FileWatcher(path)
    watch_subdir(observer, event_handler, "src")
    watch_subdir(observer, event_handler, "include")
    watch_subdir(observer, event_handler, "tests")
    return event_handler


def find_project_dirs(root_dir):
    for root, dirs, files in os.walk(root_dir):
        if "CMakeLists.txt" in files:
            yield root


if __name__ == "__main__":
    watch_mode = len(sys.argv) >= 2 and sys.argv[1] == "watch"
    root_dir = os.getcwd()
    if watch_mode and len(sys.argv) >= 3:
        root_dir = sys.argv[2]
        if not os.path.isdir(root_dir):
            print(f"{root_dir} is not a directory")
            sys.exit(1)

    if watch_mode:
        from watchdog.observers import Observer
        observer = Observer()
        for dir in find_project_dirs(root_dir):
            add_watch(observer, dir)

        observer.start()
        try:
            while True:
                time.sleep(1000)
        finally:
            observer.stop()
            observer.join()
    else:
        for dir in find_project_dirs(root_dir):
            gen_filelist(dir)
