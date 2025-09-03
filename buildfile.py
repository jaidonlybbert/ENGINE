import os
import subprocess
from conan.api.conan_api import ConanAPI

def run_command(cmd, cwd=None):
    print(f"Running: {' '.join(cmd)}")
    subprocess.run(cmd, cwd=cwd, check=True)

def main():
    conan = ConanAPI()
    source_dir = os.path.abspath(".")
    build_dir = os.path.join(source_dir, "build")

    os.makedirs(build_dir, exist_ok=True)

    # Install dependencies using conanfile.py
    conan.install.install(path=source_dir, install_folder=build_dir, settings={"build_type": "Release"})

    # Configure with CMake using the generated toolchain
    run_command([
        "cmake",
        "-DCMAKE_TOOLCHAIN_FILE=" + os.path.join(build_dir, "conan_toolchain.cmake"),
        "-DCMAKE_BUILD_TYPE=Release",
        source_dir
    ], cwd=build_dir)

    # Build the project
    run_command(["cmake", "--build", ".", "--config", "Release"], cwd=build_dir)

    # Run the executable
    exe_path = os.path.join(build_dir, "Release", "Engine.exe")
    if os.path.exists(exe_path):
        run_command([exe_path])
    else:
        print("Executable not found:", exe_path)

if __name__ == "__main__":
    main()
