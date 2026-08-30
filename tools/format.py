#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path

# File extensions to format
VALID_EXTENSIONS = {'.cpp', '.hpp', '.c', '.h', '.cc', '.cxx', '.inl'}

# Directories to skip
EXCLUDE_DIRS = {'.git', 'build', 'bin', 'out', 'third_party', 'external', 'build-gcc', 'build-clang'}

def format_files(root_dir="."):
    root_path = Path(root_dir).resolve()
    
    # Check if clang-format is installed
    try:
        subprocess.run(["clang-format", "--version"], capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("Error: 'clang-format' is not installed or not found in PATH.", file=sys.stderr)
        sys.exit(1)

    formatted_count = 0

    for dirpath, dirnames, filenames in os.walk(root_path):
        # Prune excluded directories in-place
        dirnames[:] = [d for d in dirnames if d not in EXCLUDE_DIRS]

        for file in filenames:
            ext = Path(file).suffix.lower()
            if ext in VALID_EXTENSIONS:
                file_path = Path(dirpath) / file
                # --style=file forces clang-format to locate your project's .clang-format
                cmd = ["clang-format", "-i", "--style=file", str(file_path)]
                
                try:
                    subprocess.run(cmd, check=True)
                    formatted_count += 1
                except subprocess.CalledProcessError as e:
                    print(f"Failed to format {file_path}: {e}", file=sys.stderr)

    print(f"Successfully formatted {formatted_count} files.")

if __name__ == "__main__":
    format_files()