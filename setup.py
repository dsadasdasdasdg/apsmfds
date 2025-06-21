# setup.py

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import os
import sys
import platform

# Project metadata
NAME = "eplfga"
VERSION = "0.0.2" # Should match eplfga/__init__.py
DESCRIPTION = "Excellent Python Library For amdGPU Acceleration (eplfga)"
AUTHOR = "AI Agent (Jules)"
AUTHOR_EMAIL = "jules@example.com" # Placeholder
URL = "https://github.com/example/eplfga" # Placeholder

# Try to read the README for long description
try:
    with open("README.md", "r", encoding="utf-8") as fh:
        LONG_DESCRIPTION = fh.read()
except FileNotFoundError:
    LONG_DESCRIPTION = DESCRIPTION

# Define the C extension module
# This will compile eplfga/core.c
extension_sources = ["eplfga/core.c"]

# Attempt to find ROCm include and library paths
rocm_path = os.environ.get("ROCM_PATH", "/opt/rocm")
rocm_include_path = os.path.join(rocm_path, "include")
rocm_lib_path = os.path.join(rocm_path, "lib")

# Compiler and linker arguments
# These might need to be adjusted based on the system and ROCm version
compile_args = []
link_args = []
libraries = []
library_dirs = []
include_dirs = ['eplfga'] # Include the directory where core.c might have local headers

if os.path.exists(rocm_path):
    print(f"Found ROCm path: {rocm_path}")
    include_dirs.append(rocm_include_path)
    include_dirs.append(os.path.join(rocm_include_path, "hip/hcc")) # For hip_runtime_api.h if using HIP
    library_dirs.append(rocm_lib_path)

    # Add common ROCm libraries. This list might need adjustment.
    # For HIP, 'amdhip64' is common. For other ROCm components, other libs might be needed.
    # libraries.append('amdhip64') # If using HIP directly
    # libraries.append('hsa-runtime64') # If using HSA directly

    if sys.platform == 'linux':
        link_args.append(f'-Wl,-rpath,{rocm_lib_path}')
else:
    print(f"Warning: ROCm path '{rocm_path}' not found. C extension compilation might fail or be limited.")
    print("Set ROCM_PATH environment variable if ROCm is installed in a non-standard location.")

# Define the extension
# The name '_core' is a convention for C extension modules.
# It will be importable as `from eplfga import _core`
core_module = Extension(
    "eplfga._core", # Name of the module when imported
    sources=extension_sources,
    include_dirs=include_dirs,
    library_dirs=library_dirs,
    libraries=libraries,
    extra_compile_args=compile_args,
    extra_link_args=link_args,
    language="c"
)

# Custom build_ext to handle potential compilation issues or alternative compilers
class CustomBuildExt(build_ext):
    def build_extensions(self):
        # You can add custom logic here, e.g., to select a compiler
        # or print more detailed logs.
        # Example: self.compiler.set_executable("compiler_fixup", "clang")
        try:
            super().build_extensions()
        except Exception as e:
            print(f"Error building C extension: {e}")
            print("Ensure you have a C compiler (gcc/clang) and ROCm development packages installed.")
            # Decide if you want to fail the build or allow it to proceed without the C extension
            # For now, let's make it optional if it fails to compile.
            # raise # Re-raise the exception to fail the build
            print("Proceeding with Python-only build due to C extension failure.")
            self.extensions = [] # Remove extensions if build fails


setup(
    name=NAME,
    version=VERSION,
    author=AUTHOR,
    author_email=AUTHOR_EMAIL,
    description=DESCRIPTION,
    long_description=LONG_DESCRIPTION,
    long_description_content_type="text/markdown",
    url=URL,
    packages=["eplfga"], # Find packages under the 'eplfga' directory
    ext_modules=[core_module], # C extensions
    cmdclass={
        'build_ext': CustomBuildExt,
    },
    classifiers=[
        "Development Status :: 3 - Alpha", # Indicates early development stage
        "Intended Audience :: Developers",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "License :: OSI Approved :: MIT License", # Choose an appropriate license
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: C",
        "Operating System :: POSIX :: Linux", # ROCm is primarily Linux-focused
    ],
    python_requires=">=3.7",
    install_requires=[
        # List Python dependencies here, e.g.,
        # "numpy>=1.18",
        # "torch", # if it's a PyTorch extension, for example
    ],
    # entry_points={
    #     'console_scripts': [
    #         'eplfga-cli=eplfga.cli:main', # If you plan a CLI tool
    #     ],
    # },
    zip_safe=False # C extensions often make zip_safe False
)

print("\n--- Setup Script Finished ---")
print("To build the C extension and install the package, run:")
print("  python setup.py build_ext --inplace  # (to build in place for development)")
print("  python setup.py install             # (to install into your Python environment)")
print("  pip install .                     # (recommended for modern Python packaging)")
print("  pip install -e .                  # (for editable/development mode install)")
