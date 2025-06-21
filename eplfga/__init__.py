# eplfga/__init__.py
import ctypes
import os
import platform
from . import utils # Import utils to make them accessible via eplfga.utils

__version__ = "0.0.2" # Updated version

# --- CTypes Definitions ---

# Define a base exception for errors from the C core
class EplfgaCoreError(Exception):
    def __init__(self, error_code, error_message, function_name=""):
        self.error_code = error_code
        self.error_message = error_message
        self.function_name = function_name
        super().__init__(f"Error in C function '{function_name}': [{error_code}] {error_message}")

# Define eplfga_error_t structure (matches C)
class EplfgaError(ctypes.Structure):
    _fields_ = [
        ("error_code", ctypes.c_int),
        ("error_message", ctypes.c_char * 256)
    ]

    def check(self, function_name="<unknown_function>"):
        """Checks the error code and raises EplfgaCoreError if not success (0)."""
        # Assuming 0 is hipSuccess or general success for our C functions.
        # For hipError_t, hipSuccess is 0. For our own error codes, -1 was used for invalid args.
        if self.error_code != 0: # hipSuccess is 0
            raise EplfgaCoreError(self.error_code, self.error_message.decode('utf-8', 'replace'), function_name)

# Define hipDeviceProp_t structure (simplified, add more fields as needed)
# Based on common fields from hipDeviceProp_t in ROCm's hip_runtime_api.h
# Full structure is very large, this is a subset for demonstration.
# Ensure this matches the version/layout your ROCm headers use if you compile.
class HipDeviceProperties(ctypes.Structure):
    _fields_ = [
        ("name", ctypes.c_char * 256),
        ("totalGlobalMem", ctypes.c_size_t),
        ("sharedMemPerBlock", ctypes.c_size_t),
        ("regsPerBlock", ctypes.c_int),
        ("warpSize", ctypes.c_int),
        ("maxThreadsPerBlock", ctypes.c_int),
        ("maxThreadsDim", ctypes.c_int * 3),
        ("maxGridSize", ctypes.c_int * 3),
        ("clockRate", ctypes.c_int), # kHz
        ("memoryClockRate", ctypes.c_int), # kHz
        ("memoryBusWidth", ctypes.c_int), # bits
        ("totalConstMem", ctypes.c_size_t),
        ("major", ctypes.c_int), # Major compute capability
        ("minor", ctypes.c_int), # Minor compute capability
        ("multiProcessorCount", ctypes.c_int),
        ("l2CacheSize", ctypes.c_int),
        ("maxThreadsPerMultiProcessor", ctypes.c_int),
        ("computeMode", ctypes.c_int),
        ("clockInstructionRate", ctypes.c_int), # kHz
        ("arch", ctypes.c_void_p), # Placeholder for hip архітектура struct
        ("concurrentKernels", ctypes.c_int),
        ("pciDomainID", ctypes.c_int),
        ("pciBusID", ctypes.c_int),
        ("pciDeviceID", ctypes.c_int),
        ("maxSharedMemoryPerMultiProcessor", ctypes.c_size_t),
        ("isMultiGpuBoard", ctypes.c_int),
        ("canMapHostMemory", ctypes.c_int),
        ("gcnArch", ctypes.c_int), # Deprecated, use gcnArchName
        ("gcnArchName", ctypes.c_char * 256), # e.g., "gfx900", "gfx906"
        # Add more fields as necessary, this is a substantial but not exhaustive list.
        # Ensure to check ROCm documentation for exact field names and types for your target ROCm version.
    ]

    def __str__(self):
        fields_str = []
        for field_name, _ in self._fields_:
            value = getattr(self, field_name)
            if isinstance(value, bytes):
                value_str = value.decode('utf-8', 'replace')
            elif isinstance(value, ctypes.Array):
                value_str = list(value)
            else:
                value_str = value
            fields_str.append(f"  {field_name}: {value_str}")
        return "HipDeviceProperties:\n" + "\n".join(fields_str)

# --- Load C Library ---
_core = None
try:
    # Determine library extension based on OS
    if platform.system() == "Windows":
        lib_ext = ".dll"
    elif platform.system() == "Darwin": # macOS
        lib_ext = ".dylib"
    else: # Linux and other Unix-like
        lib_ext = ".so"

    # Try to find the library in the same directory as this file (e.g., when built with --inplace)
    # The name "eplfga._core" in setup.py results in a file like _core.cpython-3XX-x86_64-linux-gnu.so
    # We need to find the actual file name.
    package_dir = os.path.dirname(__file__)

    # A more robust way to find the .so file:
    built_lib_name = None
    for file in os.listdir(package_dir):
        if file.startswith("_core") and file.endswith(lib_ext):
            # Check if it contains cpython, indicating it's the extension module
            if "cpython-" in file: # e.g. _core.cpython-312-x86_64-linux-gnu.so
                 built_lib_name = file
                 break

    if built_lib_name:
        lib_path = os.path.join(package_dir, built_lib_name)
        if os.path.exists(lib_path):
            _core = ctypes.CDLL(lib_path)
        else:
            print(f"eplfga warning: C extension library '{built_lib_name}' not found at '{lib_path}'.")
    else:
        # Fallback for simpler names if the globbing above fails or for different build systems
        # This might be needed if the name isn't mangled with cpython tags.
        # This path is less likely to work with standard setuptools builds without extra configuration.
        # For development, `python setup.py build_ext --inplace` usually creates the tagged name.
        try:
            _core = ctypes.CDLL(os.path.join(package_dir, "_core" + lib_ext))
        except OSError:
            print(f"eplfga warning: C extension library '_core{lib_ext}' not found. Python-only features will work.")


    if _core:
        # --- Function Signatures (argtypes and restype) ---

        # const char* get_eplfga_c_core_version();
        _core.get_eplfga_c_core_version.restype = ctypes.c_char_p

        # int get_hip_runtime_version_c(int* runtime_version_major, int* runtime_version_minor, eplfga_error_t* err_info);
        _core.get_hip_runtime_version_c.argtypes = [ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int), ctypes.POINTER(EplfgaError)]
        _core.get_hip_runtime_version_c.restype = ctypes.c_int # Returns hipError_t (which is an int)

        # int initialize_rocm_environment_c(eplfga_error_t* err_info);
        _core.initialize_rocm_environment_c.argtypes = [ctypes.POINTER(EplfgaError)]
        _core.initialize_rocm_environment_c.restype = ctypes.c_int

        # int get_gpu_device_count_c(int* gpu_count, eplfga_error_t* err_info);
        _core.get_gpu_device_count_c.argtypes = [ctypes.POINTER(ctypes.c_int), ctypes.POINTER(EplfgaError)]
        _core.get_gpu_device_count_c.restype = ctypes.c_int

        # int get_device_properties_c(int device_id, hipDeviceProp_t* prop, eplfga_error_t* err_info);
        _core.get_device_properties_c.argtypes = [ctypes.c_int, ctypes.POINTER(HipDeviceProperties), ctypes.POINTER(EplfgaError)]
        _core.get_device_properties_c.restype = ctypes.c_int

        # int get_device_memory_info_c(size_t* free_mem, size_t* total_mem, eplfga_error_t* err_info);
        _core.get_device_memory_info_c.argtypes = [ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(EplfgaError)]
        _core.get_device_memory_info_c.restype = ctypes.c_int

except (OSError, AttributeError) as e:
    # AttributeError can happen if _core is None and we try to set argtypes
    print(f"eplfga warning: Failed to load or configure C extension library. Python-only features will work. Error: {e}")
    _core = None # Ensure _core is None if loading failed

# --- Python API Functions ---

def get_c_core_version():
    """Returns the version string of the compiled C core library."""
    if not _core:
        raise EplfgaCoreError(-1, "C core library not loaded.", "get_c_core_version")
    return _core.get_eplfga_c_core_version().decode('utf-8')

def get_hip_runtime_version():
    """
    Returns the HIP runtime version as a tuple (major, minor).
    Raises EplfgaCoreError on failure.
    """
    if not _core:
        raise EplfgaCoreError(-1, "C core library not loaded.", "get_hip_runtime_version")

    major = ctypes.c_int()
    minor = ctypes.c_int()
    err_info = EplfgaError()

    # The C function returns hipError_t, which is also captured in err_info.error_code
    _core.get_hip_runtime_version_c(ctypes.byref(major), ctypes.byref(minor), ctypes.byref(err_info))
    err_info.check("get_hip_runtime_version_c") # Will raise if err_info.error_code is not hipSuccess (0)

    return major.value, minor.value

def initialize_rocm():
    """
    Initializes the ROCm/HIP environment.
    Returns True on success.
    Raises EplfgaCoreError on failure.
    """
    if not _core:
        raise EplfgaCoreError(-1, "C core library not loaded.", "initialize_rocm")

    err_info = EplfgaError()
    _core.initialize_rocm_environment_c(ctypes.byref(err_info))
    err_info.check("initialize_rocm_environment_c")
    return True

def get_gpu_count():
    """
    Returns the number of detected AMD GPUs.
    Raises EplfgaCoreError on failure.
    """
    if not _core:
        raise EplfgaCoreError(-1, "C core library not loaded.", "get_gpu_count")

    count = ctypes.c_int()
    err_info = EplfgaError()
    _core.get_gpu_device_count_c(ctypes.byref(count), ctypes.byref(err_info))
    err_info.check("get_gpu_device_count_c")
    return count.value

def get_gpu_properties(device_id: int):
    """
    Returns a dictionary containing properties for the specified GPU device ID.
    Raises EplfgaCoreError on failure.
    """
    if not _core:
        raise EplfgaCoreError(-1, "C core library not loaded.", "get_gpu_properties")

    props_c = HipDeviceProperties()
    err_info = EplfgaError()
    _core.get_device_properties_c(device_id, ctypes.byref(props_c), ctypes.byref(err_info))
    err_info.check("get_device_properties_c")

    # Convert C structure to a Python dictionary for easier use
    py_props = {}
    for field_name, field_type in props_c._fields_:
        value = getattr(props_c, field_name)
        if isinstance(value, bytes):
            py_props[field_name] = value.decode('utf-8', 'replace').rstrip('\x00')
        elif isinstance(value, ctypes.Array):
            py_props[field_name] = list(value)
        else:
            py_props[field_name] = value
    return py_props

def get_gpu_memory_info(device_id: int = -1):
    """
    Returns total and free memory (in bytes) for a GPU.
    If device_id is -1 (default), uses the current device.
    Otherwise, it would ideally set the device first (TODO: add set_device capability).
    For now, it gets memory info for the *currently active* device context in HIP.
    Raises EplfgaCoreError on failure.
    """
    if not _core:
        raise EplfgaCoreError(-1, "C core library not loaded.", "get_gpu_memory_info")

    # TODO: Implement hipSetDevice(device_id) if device_id is not -1 and different from current.
    # This requires adding a wrapper for hipSetDevice and hipGetDevice.
    # For now, this function relies on the current device context.
    if device_id != -1:
        # Placeholder for future:
        # current_device = get_current_device() # Needs to be implemented
        # if current_device != device_id:
        #    set_device(device_id) # Needs to be implemented
        print(f"eplfga warning: get_gpu_memory_info called with device_id={device_id}, "
              "but changing device context is not yet implemented. Info will be for current active device.")


    free_mem = ctypes.c_size_t()
    total_mem = ctypes.c_size_t()
    err_info = EplfgaError()
    _core.get_device_memory_info_c(ctypes.byref(free_mem), ctypes.byref(total_mem), ctypes.byref(err_info))
    err_info.check("get_device_memory_info_c")
    return {"free": free_mem.value, "total": total_mem.value}


def hello():
    """A simple hello function to test the library structure."""
    message = "Hello from eplfga!"
    if _core:
        message += " C core partially loaded."
    else:
        message += " C core NOT loaded."
    return message

# Expose key functions at the package level for convenience
# (others can be accessed via eplfga.utils.func_name if not here)
__all__ = [
    "get_c_core_version",
    "get_hip_runtime_version",
    "initialize_rocm",
    "get_gpu_count",
    "get_gpu_properties",
    "get_gpu_memory_info",
    "hello",
    "EplfgaCoreError",
    "utils" # Expose the utils module
]

if _core:
    print("eplfga: C core bindings initialized.")
else:
    print("eplfga: Initialized in Python-only mode (C core not loaded).")
