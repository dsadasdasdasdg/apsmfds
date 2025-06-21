# eplfga/utils.py

import platform
import os

def get_system_info():
    """Returns basic system information."""
    return {
        "system": platform.system(),
        "release": platform.release(),
        "version": platform.version(),
        "machine": platform.machine(),
        "processor": platform.processor(),
        "python_version": platform.python_version()
    }

def check_rocm_installation_path():
    """
    Checks for standard ROCm installation paths.
    Returns the path if found, otherwise None.
    """
    default_paths = ["/opt/rocm", "/opt/rocm/", os.path.expanduser("~/.rocm")]
    for path in default_paths:
        if os.path.exists(path) and os.path.isdir(path):
            # Further checks can be added, e.g., for specific binaries or libraries
            if os.path.exists(os.path.join(path, "bin", "rocminfo")):
                return path
    return None

# Placeholder for future utility functions related to GPU management,
# logging, error handling, etc.

# Example of a more specific utility function that might be needed:
# def get_gpu_info():
#     """
#     Placeholder for a function that would parse output from a ROCm tool
#     or use the C bindings to get detailed GPU information.
#     This is a high-level Python utility.
#     """
#     # In a real scenario, this might call a function from the C extension
#     # or run a command like `rocminfo`.
#     print("Fetching GPU info (stubbed in utils.py).")
#     # For now, return mock data
#     return [{"id": 0, "name": "AMD Radeon GPU (mock)", "memory_total": "8192MB"}]

if __name__ == "__main__":
    print("System Information:")
    for key, value in get_system_info().items():
        print(f"  {key}: {value}")

    rocm_path = check_rocm_installation_path()
    if rocm_path:
        print(f"\nROCm installation found at: {rocm_path}")
    else:
        print("\nROCm installation not found in default locations.")
