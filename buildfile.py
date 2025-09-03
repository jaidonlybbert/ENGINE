import os
import subprocess


env = os.environ.copy()
env["VULKAN_SDK"] = "/Users/jelly/VulkanSDK/1.3.283.0/macOS"
env["PATH"] += ":" + os.path.join(env["VULKAN_SDK"], "bin")
env["DYLD_LIBRARY_PATH"] = os.path.join(env["VULKAN_SDK"], "lib")
env["VK_ICD_FILENAMES"] = os.path.join(
    env["VULKAN_SDK"], "share", "vulkan", "icd.d", "MoltenVK_icd.json")
env["VK_LAYER_PATH"] = os.path.join(
    "VULKAN_SDK", "share", "vulkan", "explicit_layer.d")


def run_command(cmd, cwd=None):
    print(f"Running: {' '.join(cmd)}")
    subprocess.run(cmd, cwd=cwd, check=True, env=env)


def main():
    source_dir = os.path.abspath(".")
    build_dir = os.path.join(source_dir, "build")

    os.makedirs(build_dir, exist_ok=True)

    # Install dependencies using conanfile.py
    run_command(["conan", "install", source_dir,
                 "--build", "missing"], cwd=source_dir)

    # Configure with CMake using the generated toolchain
    run_command([
        "cmake",
        "-DCMAKE_TOOLCHAIN_FILE=" +
        os.path.join(build_dir, "Release", "generators",
                     "conan_toolchain.cmake"),
        "-DCMAKE_BUILD_TYPE=Release",
        source_dir
    ], cwd=build_dir)

    # Build the project
    run_command(["cmake", "--build", ".", "--config",
                "Release"], cwd=build_dir)

    # Run the executable
    exe_path = os.path.join(build_dir, "Release", "Engine.exe")
    if os.path.exists(exe_path):
        run_command([exe_path])
    else:
        print("Executable not found:", exe_path)


if __name__ == "__main__":
    main()
