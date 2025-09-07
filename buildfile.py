import os
import platform
import subprocess
import argparse


def run_command(cmd, cwd=None, env=None):
    print(f"Running: {' '.join(cmd)}")
    subprocess.run(cmd, cwd=cwd, check=True, env=env)


def main():
    env = os.environ.copy()

    print(f"Platform is {platform.platform()}")
    parser = argparse.ArgumentParser(description="ENGINE build script")
    parser.add_argument(
        "--buildtype", type=str, default="Release",
        help="One of [Release, Debug, RelWithDebInfo, MinSizeRel]")
    parser.add_argument(
        "--preset", type=str, default="default",
        help="Preset defined in CMakeUserPresets.txt or CMakePresets.txt")

    args = parser.parse_args()
    source_dir = os.path.abspath(".")
    build_dir = os.path.join(source_dir, "build")

    print(f"CWD: {source_dir}")

    os.makedirs(build_dir, exist_ok=True)

    # Install dependencies using conanfile.py
    run_command(
        ["conan", "install", source_dir,
         "--build", "missing", "--settings=build_type=" + args.buildtype,
         "--settings=compiler.cppstd=20"], cwd=source_dir, env=env)

    # Configure with CMake using the generated toolchain
    run_command(
        ["cmake", "--preset", args.preset,
         f"-DCMAKE_BUILD_TYPE={args.buildtype}"], cwd=source_dir, env=env)

    # Build the project
    run_command(
        ["cmake", "--build", "--preset", args.preset], cwd=source_dir, env=env)

    # Run the executable
    executable = "Engine.exe" if "Windows" in platform.platform() else "Engine"
    exe_path = os.path.join(build_dir, args.buildtype, executable)
    if os.path.exists(exe_path):
        run_command([exe_path])
    else:
        print("Executable not found:", exe_path)


if __name__ == "__main__":
    main()
