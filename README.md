# eplfga (Excellent Python Library For amdGPU Acceleration)

**Version: 0.0.2 (Alpha)**

## Overview

`eplfga` is a Python library designed to simplify the deployment and management of ROCm (Radeon Open Compute platform) environments on AMD GPUs. It targets both datacenter and consumer hardware, aiming to provide tools for environment interaction and information gathering, with a view towards future optimizations.

The library features a C core for direct ROCm API interactions (primarily using HIP), exposed to Python via `ctypes` bindings. This allows for low-level access while maintaining a Pythonic user interface.

## Current Features (Version 0.0.2 Alpha)

*   **ROCm/HIP Environment Interaction (via C Core & Python Bindings):**
    *   **Initialization:** Function to initialize the HIP runtime (`eplfga.initialize_rocm()`).
    *   **GPU Detection:** Get the number of available AMD GPUs (`eplfga.get_gpu_count()`).
    *   **Device Properties:** Retrieve detailed properties for each GPU, such as name, memory, clock speed, architecture (`eplfga.get_gpu_properties(device_id)`).
    *   **Memory Information:** Get total and free memory for the current GPU (`eplfga.get_gpu_memory_info()`).
    *   **Version Information:** Get HIP runtime version (`eplfga.get_hip_runtime_version()`) and the eplfga C core version (`eplfga.get_c_core_version()`).
*   **Python Utilities:**
    *   System information gathering (`eplfga.utils.get_system_info()`).
    *   Basic check for ROCm installation paths (`eplfga.utils.check_rocm_installation_path()`).
*   **Error Handling:**
    *   Custom exception `eplfga.EplfgaCoreError` raised for errors originating from the C core.
*   **Build System:**
    *   `setup.py` script that attempts to compile the C extension but allows Python-only installation if ROCm SDK/compiler is not found.

## Planned Features (Future Development)

*   **Advanced ROCm Environment Management:**
    *   More detailed verification of ROCm environment integrity.
    *   Functions to set the active GPU device.
*   **Performance & Optimization Utilities:**
    *   Wrappers for basic ROCm profiling tools or APIs.
    *   Exposing more GPU tuning parameters (if feasible and safe).
*   **Memory Management Enhancements:**
    *   Pythonic wrappers for `hipMalloc`, `hipFree`, `hipMemcpy`.
*   **Storage and Data Handling:**
    *   Utilities to assist with managing large datasets for GPU processing (details to be refined).

## Project Structure

```
eplfga/
├── eplfga/                 # Main library source code
│   ├── __init__.py         # Initializes the Python package, exposes API
│   ├── core.c              # C extension code for ROCm interaction
│   └── utils.py            # Python utility functions
├── tests/                  # Test suite
│   ├── __init__.py
│   └── test_eplfga.py      # Unit tests for the library
├── setup.py                # Build script for the library and C extensions
├── README.md               # This file
└── AGENTS.md               # (Optional) Instructions for AI development agents
```

## Installation

**Prerequisites:**
*   Python 3.7+
*   **For C Extensions (recommended for full functionality):**
    *   A C compiler (GCC or Clang).
    *   ROCm Toolkit: Installed and correctly configured. Ensure the `ROCM_PATH` environment variable is set (e.g., `/opt/rocm`) if ROCm is not in a standard location detected by the build script. The `setup.py` script will attempt to find ROCm headers and libraries.
*   **Without C Extensions:**
    *   If ROCm Toolkit or a C compiler is not available, the library can still be installed, but will operate in a Python-only mode with limited functionality (C core dependent features will raise `EplfgaCoreError`).

**Steps:**

```bash
# Clone the repository (if not already done)
# git clone https://github.com/example/eplfga.git
# cd eplfga

# Recommended: create a virtual environment
python -m venv .venv
source .venv/bin/activate # On Windows: .venv\Scripts\activate

# Install dependencies and build the C extension
pip install .

# For development (editable install):
pip install -e .
```

## Usage Example

```python
import eplfga
import eplfga.utils

def main():
    print(eplfga.hello())
    print(f"eplfga version: {eplfga.__version__}")

    print("\n--- System Information ---")
    system_info = eplfga.utils.get_system_info()
    for key, value in system_info.items():
        print(f"  {key}: {value}")

    rocm_path_check = eplfga.utils.check_rocm_installation_path()
    if rocm_path_check:
        print(f"ROCm installation detected by util at: {rocm_path_check}")
    else:
        print("ROCm installation not detected by util in default paths.")

    try:
        print("\n--- ROCm/HIP Information (from C Core) ---")

        # Initialize ROCm/HIP
        eplfga.initialize_rocm()
        print("ROCm/HIP initialized successfully.")

        # Get C Core and HIP Runtime versions
        c_core_ver = eplfga.get_c_core_version()
        hip_major, hip_minor = eplfga.get_hip_runtime_version()
        print(f"eplfga C Core Version: {c_core_ver}")
        print(f"HIP Runtime Version: {hip_major}.{hip_minor}")

        # Get GPU count
        gpu_count = eplfga.get_gpu_count()
        print(f"Number of GPUs detected: {gpu_count}")

        if gpu_count > 0:
            # Get properties for each GPU
            for i in range(gpu_count):
                print(f"\n--- Properties for GPU {i} ---")
                props = eplfga.get_gpu_properties(i)
                print(f"  Name: {props.get('name', 'N/A')}")
                print(f"  Total Global Memory: {props.get('totalGlobalMem', 0) / (1024**3):.2f} GB")
                print(f"  Compute Capability: {props.get('major', 0)}.{props.get('minor', 0)}")
                print(f"  Clock Rate: {props.get('clockRate', 0) / 1000:.2f} GHz")
                print(f"  GCN Arch Name: {props.get('gcnArchName', 'N/A')}")
                # Print more properties as needed by accessing the props dictionary

            # Get memory info for the current (default device 0 if not changed)
            # Note: To get memory info for a specific device, one would typically
            # need a `set_device(i)` function first. This example uses current device.
            print("\n--- Memory Info for Current GPU ---")
            mem_info = eplfga.get_gpu_memory_info() # device_id defaults to current
            print(f"  Total Memory: {mem_info.get('total', 0) / (1024**3):.2f} GB")
            print(f"  Free Memory: {mem_info.get('free', 0) / (1024**3):.2f} GB")

    except eplfga.EplfgaCoreError as e:
        print(f"\nROCm/HIP C Core Error: {e}")
        print("This is expected if the C extensions are not compiled or ROCm is not available.")
    except Exception as e:
        print(f"\nAn unexpected error occurred: {e}")

if __name__ == "__main__":
    main()
```

## Error Handling

Functions interacting with the C core will raise an `eplfga.EplfgaCoreError` if:
*   The C extension library (`_core.so`/`.dll`) could not be loaded.
*   A called C function returns an error (e.g., a HIP API error).

The error object contains `error_code` and `error_message` attributes from the C layer.

## Contributing

Contributions are welcome! Please refer to `CONTRIBUTING.md` (to be created) for guidelines.
This project is currently being bootstrapped by an AI agent.

## License

This project is intended to be licensed under the MIT License. (License file to be added).