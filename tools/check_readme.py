
import platform
import subprocess
import shlex

def is_hr(line):
    return line and all((c == "-" or c == "=") for c in line)

def is_current_platform(platform_name):
    return platform.system() == platform_name

if __name__ == "__main__":
    readme_filename = "README.md"
    with open(readme_filename) as readme:
        is_building_section = False
        platform_name = None
        last_line = ""
        for i, line in enumerate(readme.readlines()):
            line = line.rstrip()

            if is_hr(line):
                if last_line == "Building":
                    print(f"Found Building section on line {i}")
                elif is_building_section:
                    print(f"Leaving Building section on line {i}")
                is_building_section = last_line == "Building"
            elif is_building_section:
                if line.startswith("**Windows**:"):
                    platform_name = "Windows"
                    print(f"Setting platform to Windows on line {i+1}")
                elif line.startswith("**Linux**:"):
                    platform_name = "Linux"
                    print(f"Setting platform to Linux on line {i+1}")
                elif line.startswith("    $"):
                    cmd = line.split("$")[1].strip()
                    print(f"Found command `{cmd}` for platform {platform_name} on line {i+1}")
                    if is_current_platform(platform_name):
                        print("Executing...")
                        if subprocess.run(shlex.split(cmd)).returncode != 0:
                            print(f"\nReadme is out of date! The command `{cmd}` on line {i+1} failed to execute.")
                            exit(1)

            last_line = line
