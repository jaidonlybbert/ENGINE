import os
import shutil
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
    parser.add_argument(
        "--profile", type=str, default="default",
        help="Conan profile")
    parser.add_argument(
        "--graphviz", action="store_true",
        help="Generate a graphviz .dot file for dependency graph visualization")
    args = parser.parse_args()

    source_dir = os.path.abspath(".")
    docs_dir = os.path.join(source_dir, "docs")
    build_dir = os.path.join(source_dir, "build")
    graphviz_dir = os.path.join(build_dir, "graphviz")

    print(f"CWD: {source_dir}")

    os.makedirs(build_dir, exist_ok=True)
    os.makedirs(docs_dir, exist_ok=True)
    os.makedirs(graphviz_dir, exist_ok=True)

    # Install dependencies using conanfile.py
    run_command(
        ["conan", "install", source_dir,
         f"--profile:host={args.profile}",
         f"--profile:build={args.profile}",
         f"--settings:host=build_type={args.buildtype}",
         f"--settings:build=build_type={args.buildtype}",
         "--settings:host=compiler.cppstd=20",
         "--settings:build=compiler.cppstd=20",
         "--build", "missing"
         ], cwd=source_dir, env=env)

    # Configure with CMake using the generated toolchain
    config_args = ["cmake", "--preset", args.preset,
                   f"-DCMAKE_BUILD_TYPE={args.buildtype}"]

    if args.graphviz:
        config_args.append("--graphviz=build/graphviz/graph.dot")

    run_command(config_args, cwd=source_dir, env=env)

    # Build the project
    if "Windows" in platform.platform():
        run_command(
            ["cmake", "--build", "--preset", args.preset, "--config", args.buildtype], cwd=source_dir, env=env)
    else:
        run_command(
            ["cmake", "--build", "--preset", args.preset], cwd=source_dir, env=env)

    # Generate .dot file for dependency graph visualization
    if args.graphviz:
        # convert graphviz to svg
        run_command(["dot", "-Tsvg", "-o", "graph.svg",
                    "graph.dot"], cwd=graphviz_dir, env=env)
        svg_path = os.path.join(graphviz_dir, "graph.svg")
        shutil.copy(svg_path, docs_dir)

    # Run the executable
    executable = "Engine.exe" if "Windows" in platform.platform() else "Engine"
    exe_path = os.path.join(build_dir, args.buildtype, executable)
    if os.path.exists(exe_path):
        run_command([exe_path])
    else:
        print("Executable not found:", exe_path)


if __name__ == "__main__":
    main()
