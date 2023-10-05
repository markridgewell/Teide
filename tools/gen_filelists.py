
import os
import subprocess
import sys
import time
import threading
import filecmp

pause_updating = False

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

    outfilename = os.path.join(dir, "FileLists.cmake")
    tempfilename = os.path.join(dir, "FileLists.cmake.temp")
    with open(tempfilename, "w") as f:
        f.write("set(sources")
        f.write("\n    ")
        f.write("\n    ".join(source_files) + ")\n")

    subprocess.run(["cmake-format", "-i", tempfilename], shell=True)

    if os.path.exists(outfilename) and filecmp.cmp(outfilename, tempfilename, shallow=False):
        os.remove(tempfilename)
    else:
        if os.path.exists(outfilename):
            os.remove(outfilename)
        os.rename(tempfilename, outfilename)
        print("Updated file list for", dir)


def watch_subdir(observer, event_handler, subdir):
    subpath = os.path.join(event_handler.dir, subdir)
    if os.path.exists(subpath):
        print(f"Watching subdir {subpath}")
        observer.schedule(event_handler, subpath, recursive=True)


def add_git_watch(observer, path):
    from watchdog.events import FileSystemEventHandler

    class GitWatcher(FileSystemEventHandler):
        def __init__(self, dir):
            print(f"Created a git watcher for {dir}")
            self.dir = dir

        def on_created(self, event):
            if os.path.basename(event.src_path) == "index.lock":
                self._lockfile_created()

        def on_deleted(self, event):
            if os.path.basename(event.src_path) == "index.lock":
                self._lockfile_deleted()

        def on_moved(self, event):
            if os.path.basename(event.src_path) == "index.lock":
                self._lockfile_deleted()
            if os.path.basename(event.dest_path) == "index.lock":
                self._lockfile_created()

        def _lockfile_created(self):
            print("Lockfile created")
            global pause_updating
            pause_updating = True

        def _lockfile_deleted(self):
            print("Lockfile deleted")
            global pause_updating
            pause_updating = False

    event_handler = GitWatcher(path)
    watch_subdir(observer, event_handler, ".git")
    return event_handler


def add_project_watch(observer, path):
    from watchdog.events import FileSystemEventHandler

    class FileWatcher(FileSystemEventHandler):
        def __init__(self, dir):
            print(f"Created a project watcher for {dir}")
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
            global pause_updating
            if pause_updating:
                print("(ignored)")
                return
            if self.timer:
                self.timer.cancel()
            self.timer = threading.Timer(1.0, gen_filelist, args=[self.dir])
            self.timer.start()

    event_handler = FileWatcher(path)
    watch_subdir(observer, event_handler, "src")
    watch_subdir(observer, event_handler, "include")
    return event_handler


def find_project_dirs(root_dir):
    if os.path.isfile(os.path.join(root_dir, "CMakeLists.txt")):
        yield root_dir

    for subdir in ["tests", "examples", "benchmarks", "tools", "libs", "extras"]:
        for root, dirs, files in os.walk(os.path.join(root_dir, subdir)):
            if "CMakeLists.txt" in files and ("src" in dirs or "include" in dirs):
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
        add_git_watch(observer, root_dir)
        for dir in find_project_dirs(root_dir):
            add_project_watch(observer, dir)

        observer.start()
        try:
            while True:
                time.sleep(1000)
        except KeyboardInterrupt:
            pass
        finally:
            observer.stop()
            try:
                observer.join()
            except KeyboardInterrupt:
                pass
    else:
        for dir in find_project_dirs(root_dir):
            gen_filelist(dir)
