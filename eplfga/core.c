// eplfga/core.c
#include <stdio.h>
#include <string.h> // For strcpy, strcat if needed for version string

// ROCm HIP API header
#include <hip/hip_runtime_api.h>
// #include <hip/hip_version.h> // For HIP_VERSION_MAJOR, etc. if needed directly

// Define a structure to return detailed error information
typedef struct {
    int error_code; // ROCm error code (hipError_t)
    char error_message[256]; // String message from hipGetErrorString
} eplfga_error_t;


const char* get_eplfga_c_core_version() {
    return "eplfga C core v0.0.2";
}

// Function to get the HIP runtime version
// Returns 0 on success, populates runtime_version_major and runtime_version_minor.
// Returns non-zero hipError_t on failure.
int get_hip_runtime_version_c(int* runtime_version_major, int* runtime_version_minor, eplfga_error_t* err_info) {
    if (!runtime_version_major || !runtime_version_minor || !err_info) {
        if (err_info) {
            err_info->error_code = -1; // Indicate invalid arguments
            snprintf(err_info->error_message, sizeof(err_info->error_message), "Null pointer passed to get_hip_runtime_version_c");
        }
        return -1; // Or a specific error code for invalid args
    }

    int version = 0;
    hipError_t err = hipRuntimeGetVersion(&version);

    if (err == hipSuccess) {
        *runtime_version_major = version / 100; // HIP_VERSION_MAJOR equivalent calculation
        *runtime_version_minor = version % 100; // HIP_VERSION_MINOR equivalent calculation
        err_info->error_code = hipSuccess;
        snprintf(err_info->error_message, sizeof(err_info->error_message), "Success");
        return 0;
    } else {
        *runtime_version_major = -1;
        *runtime_version_minor = -1;
        err_info->error_code = err;
        snprintf(err_info->error_message, sizeof(err_info->error_message), "%s", hipGetErrorString(err));
        return err; // Return the hipError_t code
    }
}


// Placeholder for actual ROCm functionality
int initialize_rocm_environment_c(eplfga_error_t* err_info) {
    // For HIP, initialization is often implicit with the first API call,
    // or one can call hipInit(0).
    printf("Attempting to initialize ROCm environment (via hipInit)...\n");
    hipError_t err = hipInit(0); // hipInit must be called before any other hip API calls. flags argument must be 0.

    if (err_info) {
        err_info->error_code = err;
        snprintf(err_info->error_message, sizeof(err_info->error_message), "%s", hipGetErrorString(err));
    }

    if (err == hipSuccess) {
        printf("ROCm (HIP) initialized successfully.\n");
        return 0;
    } else {
        fprintf(stderr, "Failed to initialize ROCm (HIP): %s\n", hipGetErrorString(err));
        return err;
    }
}

int get_gpu_device_count_c(int* gpu_count, eplfga_error_t* err_info) {
    if (!gpu_count || !err_info) {
         if (err_info) {
            err_info->error_code = -1;
            snprintf(err_info->error_message, sizeof(err_info->error_message), "Null pointer passed to get_gpu_device_count_c");
        }
        return -1;
    }

    hipError_t err = hipGetDeviceCount(gpu_count);

    if (err_info) {
        err_info->error_code = err;
        snprintf(err_info->error_message, sizeof(err_info->error_message), "%s", hipGetErrorString(err));
    }

    if (err == hipSuccess) {
        return 0;
    } else {
        *gpu_count = 0; // Ensure count is 0 on error
        fprintf(stderr, "Failed to get GPU device count: %s\n", hipGetErrorString(err));
        return err;
    }
}

// Function to get device properties for a given device ID
// The caller is responsible for ensuring prop is a valid pointer to hipDeviceProp_t
int get_device_properties_c(int device_id, hipDeviceProp_t* prop, eplfga_error_t* err_info) {
    if (!prop || !err_info) {
        if (err_info) {
            err_info->error_code = -1;
            snprintf(err_info->error_message, sizeof(err_info->error_message), "Null pointer passed to get_device_properties_c for prop or err_info");
        }
        return -1;
    }

    hipError_t err = hipGetDeviceProperties(prop, device_id);

    if (err_info) {
        err_info->error_code = err;
        snprintf(err_info->error_message, sizeof(err_info->error_message), "%s", hipGetErrorString(err));
    }

    if (err == hipSuccess) {
        return 0;
    } else {
        // It's good practice to zero out the properties structure on error
        // to avoid returning stale or uninitialized data.
        memset(prop, 0, sizeof(hipDeviceProp_t));
        fprintf(stderr, "Failed to get device properties for device %d: %s\n", device_id, hipGetErrorString(err));
        return err;
    }
}

// Function to get memory information (total and free) for the current device
// The current device must be set prior to calling this if not device 0, e.g. using hipSetDevice()
// For simplicity, this function will operate on the current default device.
int get_device_memory_info_c(size_t* free_mem, size_t* total_mem, eplfga_error_t* err_info) {
    if (!free_mem || !total_mem || !err_info) {
         if (err_info) {
            err_info->error_code = -1;
            snprintf(err_info->error_message, sizeof(err_info->error_message), "Null pointer passed to get_device_memory_info_c");
        }
        return -1;
    }

    hipError_t err = hipMemGetInfo(free_mem, total_mem);

    if (err_info) {
        err_info->error_code = err;
        snprintf(err_info->error_message, sizeof(err_info->error_message), "%s", hipGetErrorString(err));
    }

    if (err == hipSuccess) {
        return 0;
    } else {
        *free_mem = 0;
        *total_mem = 0;
        fprintf(stderr, "Failed to get device memory info: %s\n", hipGetErrorString(err));
        return err;
    }
}


// TODO: Add functions for:
// - Potentially setting a device (hipSetDevice) if we want to target specific GPUs for subsequent calls.
// - More advanced memory management if required (hipMalloc, hipFree, hipMemcpy are fundamental but might be wrapped).
// - Basic performance/storage related utilities if they fit at C-level and are clearly defined.
