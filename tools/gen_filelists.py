
import argparse
import pathlib
import os
import subprocess
import sys
import threading
import filecmp
import socket

pause_updating = False

WATCH_HOST = '127.0.0.1'
WATCH_PORT = 65002

def is_source_file(filename):
    return os.path.splitext(filename)[1] in [".h", ".hpp", ".cpp", ".natvis"]


def add_source_files(source_files, dir, subdir):
    cwd = os.getcwd()
    os.chdir(dir)
    for root, _, files in os.walk(subdir):
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

    si = None
    if os.name == 'nt':
        si = subprocess.STARTUPINFO()
        si.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    subprocess.run(["cmake-format", "-i", tempfilename], startupinfo=si)

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


def watch_server(root_dir: pathlib.Path):
    from watchdog.observers import Observer
    from watchdog.observers import Observer
    observer = Observer()
    add_git_watch(observer, root_dir)
    for dir in find_project_dirs(root_dir):
        add_project_watch(observer, dir)
    observer.start()

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((WATCH_HOST, WATCH_PORT))
        s.listen()
        print('Server started')
        print(f'Listening on port {WATCH_PORT}')
        running = True
        while running:
            try:
                conn, addr = s.accept()
                with conn:
                    print(f"Connected by {addr}")
                    while True:
                        data = conn.recv(1024)
                        if not data:
                            break
                        if data == b'stop':
                            print('Stop requested...')
                            running = False
                            conn.sendall(b'Server stopped')
                        elif data == b'status':
                            conn.sendall(b'Server running')
                        else:
                            conn.sendall(b'Unknown command')
            except KeyboardInterrupt:
                running = False
    print('Shutting down')

    observer.stop()
    try:
        observer.join()
    except KeyboardInterrupt:
        pass


def watch_client(cmd):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((WATCH_HOST, WATCH_PORT))
            s.sendall(cmd.encode('utf-8'))
            return s.recv(1024).decode('utf-8')
    except ConnectionRefusedError:
        return None


def find_project_dirs(root_dir):
    if os.path.isfile(os.path.join(root_dir, "CMakeLists.txt")):
        yield root_dir

    for subdir in ["tests", "examples", "benchmarks", "tools", "libs", "extras"]:
        for root, dirs, files in os.walk(os.path.join(root_dir, subdir)):
            if "CMakeLists.txt" in files and ("src" in dirs or "include" in dirs):
                yield root


def main():
    parser = argparse.ArgumentParser(description='Generate FileLists.cmake files automatically for each project based on the source and header files found in each project directory.')
    def add_dir_argument(parser):
        parser.add_argument('-d', '--dir', help='Root directory to watch (default cwd)', default=os.getcwd(), type=pathlib.Path)
    parser.set_defaults(watch_mode=False)
    add_dir_argument(parser)

    watch_parser = parser.add_subparsers().add_parser('watch', help='Put the generator in watch mode. See watch --help for more info.', description='Start a watch server in the current process. or query/stop an existing server running in another process.', formatter_class=argparse.RawTextHelpFormatter)
    watch_parser.set_defaults(watch_mode=True)
    add_dir_argument(watch_parser)
    watch_command_parser = watch_parser.add_subparsers(metavar='[command]', dest='command')
    watch_command_parser.add_parser('start', help='Start the watch server (default)')
    watch_command_parser.add_parser('stop', help='Stop a previously run server process')
    watch_command_parser.add_parser('status', help='Get the status of the server')
    opts = parser.parse_args()

    if not os.path.isdir(opts.dir):
        print(f"{opts.dir} is not a directory")
        sys.exit(1)

    if opts.watch_mode:
        if not opts.command or opts.command == 'start':
            if not watch_client('status'):
                print('Starting server...')
                watch_server(opts.dir)
            else:
                print('Server already running')
        else:
            print(watch_client(opts.command) or 'Server not running')

    else:
        for dir in find_project_dirs(opts.dir):
            gen_filelist(dir)


if __name__ == "__main__":
    main()
