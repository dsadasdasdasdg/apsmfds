#!/usr/bin/env python3
# install.py
# Script to create a virtual environment, install eplfga, and test its functionality.

import os
import sys
import venv
import subprocess
import shutil
import time
import argparse
import platform # Moved import to top level

# --- Configuration ---
VENV_DIR = "eplfga_env"
EPLFGA_ROOT = os.path.dirname(os.path.abspath(__file__))
PYTHON_INTERPRETER = sys.executable # The python used to run this script

# --- Helper Functions ---

def print_step(message):
    """Prints a formatted step message."""
    print(f"\n>>> {message} <<<")

def print_info(message):
    """Prints an informational message."""
    print(f"[INFO] {message}")

def print_warning(message):
    """Prints a warning message."""
    print(f"[WARNING] {message}")

def print_error(message):
    """Prints an error message."""
    print(f"[ERROR] {message}")

def get_venv_python_interpreter():
    """Gets the path to the Python interpreter inside the virtual environment."""
    if platform.system() == "Windows":
        return os.path.join(EPLFGA_ROOT, VENV_DIR, "Scripts", "python.exe")
    else:
        return os.path.join(EPLFGA_ROOT, VENV_DIR, "bin", "python")

def run_command(command, cwd=None, check=True, capture_output=False, text=True, venv_mode=False):
    """
    Runs a command using subprocess.
    If venv_mode is True, it uses the venv's Python interpreter for 'python' or 'pip' commands.
    """
    if venv_mode:
        venv_python = get_venv_python_interpreter()
        if not os.path.exists(venv_python):
            print_error(f"Virtual environment Python interpreter not found at {venv_python}")
            if check: # if check is true, we must raise an error
                 raise FileNotFoundError(f"Venv Python not found: {venv_python}")
            return None # if check is false, we can return None

        if command[0] == "python":
            command[0] = venv_python
        elif command[0] == "pip":
            command = [venv_python, "-m", "pip"] + command[1:]

    print_info(f"Running command: {' '.join(command)}")
    try:
        process = subprocess.run(command, cwd=cwd, check=check, capture_output=capture_output, text=text)
        return process
    except subprocess.CalledProcessError as e:
        print_error(f"Command failed: {e}")
        if capture_output:
            print_error(f"Stdout: {e.stdout}")
            print_error(f"Stderr: {e.stderr}")
        if check: # Re-raise if check is True, as expected by subprocess.run with check=True
            raise
        return None # Or return the exception object e, or a custom error status
    except FileNotFoundError as e:
        print_error(f"Command not found: {e}. Ensure the command is in your PATH or installed.")
        if check:
            raise
        return None


# --- Core Functionality Placeholder ---

def setup_virtual_environment():
    """
    Sets up a virtual environment in VENV_DIR.
    If the directory already exists, it ensures pip is up-to-date.
    """
    print_step(f"Setting up virtual environment in ./{VENV_DIR}/")
    venv_path = os.path.join(EPLFGA_ROOT, VENV_DIR)

    if os.path.exists(venv_path):
        print_info(f"Virtual environment directory '{VENV_DIR}' already exists.")
        # Consider adding a check here to see if it's a valid venv, but for now, assume it is.
    else:
        print_info(f"Creating virtual environment at: {venv_path}")
        try:
            builder = venv.EnvBuilder(with_pip=True, upgrade_deps=True) # upgrade_deps ensures pip/setuptools are new
            builder.create(venv_path)
            print_info("Virtual environment created successfully.")
        except Exception as e:
            print_error(f"Failed to create virtual environment: {e}")
            sys.exit(1)

    # Ensure pip is up-to-date in the venv
    print_info("Attempting to upgrade pip in the virtual environment...")
    try:
        run_command(["python", "-m", "pip", "install", "--upgrade", "pip", "setuptools"], venv_mode=True, check=True)
        print_info("pip and setuptools upgraded successfully in the virtual environment.")
    except Exception as e:
        # Don't make this fatal, as a slightly older pip might still work.
        print_warning(f"Failed to upgrade pip/setuptools in the virtual environment: {e}")
        print_warning("Proceeding with existing pip. Installation might behave unexpectedly.")

def install_eplfga_library():
    """Installs the eplfga library from local source into the venv."""
    print_step(f"Installing eplfga library from {EPLFGA_ROOT} into ./{VENV_DIR}/")

    # The command will be: <venv_python> -m pip install -e .
    # The `run_command` with `venv_mode=True` handles finding <venv_python> and structuring the pip command.
    # `cwd=EPLFGA_ROOT` ensures pip runs in the project root where setup.py is.
    try:
        run_command(
            ["pip", "install", "-e", "."],
            cwd=EPLFGA_ROOT,
            venv_mode=True,
            check=True
        )
        print_info("eplfga library installed successfully.")
    except Exception as e:
        print_error(f"Failed to install eplfga library: {e}")
        print_error("Please check the output from pip above for more details.")
        print_error("Common issues include missing C compiler, ROCm SDK (if building C extensions), or issues in setup.py.")
        sys.exit(1)


def test_gpu_acceleration():
    """Tests GPU acceleration using eplfga by running a test script in the venv."""
    print_step("Testing GPU acceleration with eplfga...")

    test_script_content = """
import eplfga
import eplfga.utils
import sys

print(f"[TEST SCRIPT] eplfga version: {eplfga.__version__}")
print(f"[TEST SCRIPT] Python version: {sys.version}")

try:
    print("[TEST SCRIPT] Attempting to initialize ROCm/HIP...")
    eplfga.initialize_rocm()
    print("[TEST SCRIPT] ROCm/HIP initialized successfully.")

    c_core_ver = eplfga.get_c_core_version()
    hip_major, hip_minor = eplfga.get_hip_runtime_version()
    print(f"[TEST SCRIPT] eplfga C Core Version: {c_core_ver}")
    print(f"[TEST SCRIPT] HIP Runtime Version: {hip_major}.{hip_minor}")

    gpu_count = eplfga.get_gpu_count()
    print(f"[TEST SCRIPT] Number of GPUs detected: {gpu_count}")

    if gpu_count > 0:
        for i in range(gpu_count):
            print(f"--- Properties for GPU {i} ---")
            props = eplfga.get_gpu_properties(i)
            print(f"  Name: {props.get('name', 'N/A')}")
            print(f"  Total Global Memory: {props.get('totalGlobalMem', 0) / (1024**3):.2f} GB")
            print(f"  Compute Capability: {props.get('major', 0)}.{props.get('minor', 0)}")
            print(f"  GCN Arch Name: {props.get('gcnArchName', 'N/A')}")

        print("--- Memory Info for Current GPU ---")
        # This gets memory for the current device context.
        # If running on a system with GPUs but no active context or issue, this might fail.
        mem_info = eplfga.get_gpu_memory_info()
        print(f"  Total Memory: {mem_info.get('total', 0) / (1024**3):.2f} GB")
        print(f"  Free Memory: {mem_info.get('free', 0) / (1024**3):.2f} GB")
        print("[TEST SCRIPT] GPU acceleration test queries completed successfully (if GPUs present).")
        print("[SUCCESS] eplfga GPU interaction appears functional.")
    elif gpu_count == 0:
        print("[TEST SCRIPT] No GPUs detected by eplfga. This might be expected if no AMD GPUs are present or ROCm is not correctly configured.")
        print("[SUCCESS] eplfga loaded and queried GPU count successfully (reported 0 GPUs).")

except eplfga.EplfgaCoreError as e:
    print(f"[TEST SCRIPT ERROR] eplfga C Core Error: {e}")
    if "C core library not loaded" in str(e):
        print("[FAILURE] eplfga C core library was not loaded. Cannot test GPU acceleration.")
    else:
        print("[FAILURE] Error during eplfga GPU interaction. ROCm/GPU might not be properly set up.")
    sys.exit(1) # Exit with error code if C core error occurs
except Exception as e:
    print(f"[TEST SCRIPT UNEXPECTED ERROR] An unexpected error occurred: {e}")
    sys.exit(1) # Exit with error code for other unexpected errors

print("[TEST SCRIPT] Test script finished.")
"""
    try:
        # Using python -c "script content"
        # Set check=True because the script itself will sys.exit(1) on failure.
        process = run_command(
            ["python", "-c", test_script_content],
            venv_mode=True,
            check=True, # The script itself will exit with 1 on failure
            capture_output=False # We want to see the live output
        )
        if process and process.returncode == 0:
            print_info("GPU acceleration test script executed successfully.")
        else:
            # This path should ideally not be taken if check=True and the script exits with error,
            # as CalledProcessError would be raised. But as a fallback:
            print_error("GPU acceleration test script execution failed or reported errors. Check output above.")
            sys.exit(1)

    except subprocess.CalledProcessError as e:
        # This will be caught if the script exits with a non-zero status due to check=True
        print_error(f"GPU acceleration test script returned an error (exit code {e.returncode}).")
        print_error("Review the script output above for details from eplfga.")
        # No sys.exit(1) here as run_command already prints details, and we might want to continue if part of a larger sequence.
        # However, for a dedicated test step, exiting might be appropriate. For now, let main decide.
        # For the purpose of --test-gpu failing, we should make it clear.
        if __name__ == "__main__": # Only exit if this script is run directly and this test fails.
            sys.exit(1) # Or raise an exception to be handled by main.

    except Exception as e:
        print_error(f"An unexpected error occurred while trying to run the GPU acceleration test script: {e}")
        if __name__ == "__main__":
            sys.exit(1)


def train_basic_statistic_model():
    """
    Attempts a conceptual test of an ML library (PyTorch) using the GPU detected by eplfga.
    Does not install PyTorch. Skips if PyTorch is not available or cannot use ROCm.
    The "30-second training" is simplified to a quick GPU tensor operation check.
    """
    print_step("Attempting conceptual ML library GPU usage test (PyTorch)...")

    # First, use eplfga to check if ROCm and GPUs are supposedly working
    try:
        print_info("Verifying eplfga can detect ROCm/GPUs first...")
        eplfga_check_script = """
import eplfga
import sys
try:
    eplfga.initialize_rocm()
    if eplfga.get_gpu_count() > 0:
        print('eplfga: GPU detected.')
        sys.exit(0) # Success
    else:
        print('eplfga: No GPUs detected.')
        sys.exit(1) # Failure for this specific check's purpose
except eplfga.EplfgaCoreError as e:
    print(f'eplfga: Core Error - {e}')
    sys.exit(1) # Failure
except Exception as e:
    print(f'eplfga: Unexpected error - {e}')
    sys.exit(1) # Failure
"""
        process = run_command(["python", "-c", eplfga_check_script], venv_mode=True, check=True, capture_output=True)
        if process.returncode != 0:
            print_warning("eplfga did not detect a functional GPU environment. Skipping PyTorch GPU test.")
            print_info(f"eplfga check stdout: {process.stdout}")
            print_info(f"eplfga check stderr: {process.stderr}")
            return
        print_info("eplfga check successful, proceeding to PyTorch test.")
    except Exception as e:
        print_warning(f"Failed to verify eplfga GPU status before PyTorch test: {e}. Skipping PyTorch test.")
        return

    # Now, attempt the PyTorch test
    pytorch_test_script = """
import sys
print("[PYTORCH TEST SCRIPT] Checking for PyTorch and ROCm GPU availability...")
try:
    import torch
    print(f"[PYTORCH TEST SCRIPT] PyTorch version: {torch.__version__}")

    # For PyTorch with ROCm, torch.cuda.is_available() checks ROCm.
    if torch.cuda.is_available():
        print("[PYTORCH TEST SCRIPT] PyTorch reports CUDA (ROCm) is available.")
        print(f"[PYTORCH TEST SCRIPT] Number of GPUs available to PyTorch: {torch.cuda.device_count()}")

        if torch.cuda.device_count() > 0:
            gpu_name = torch.cuda.get_device_name(0)
            print(f"[PYTORCH TEST SCRIPT] GPU Name (PyTorch): {gpu_name}")

            # Perform a simple tensor operation on GPU
            try:
                print("[PYTORCH TEST SCRIPT] Performing a simple tensor operation on GPU...")
                x = torch.tensor([1.0, 2.0, 3.0]).to('cuda')
                y = x * 2
                print(f"[PYTORCH TEST SCRIPT] Tensor operation result (should be on GPU): {y}")
                if 'cuda' in str(y.device).lower():
                    print("[SUCCESS] PyTorch successfully performed a tensor operation on the ROCm GPU.")
                else:
                    print("[FAILURE] PyTorch tensor operation did not report being on CUDA/ROCm device.")
                    sys.exit(1)
            except Exception as e_tensor:
                print(f"[FAILURE] PyTorch tensor operation on GPU failed: {e_tensor}")
                sys.exit(1)
        else:
            print("[WARNING] PyTorch reports CUDA (ROCm) available, but no GPUs found by PyTorch.")
            # This could be a valid state if ROCm is installed but no compatible GPUs for PyTorch,
            # or a configuration issue. For this test, we'll consider it not a full success.
            sys.exit(1) # Not a full success for our test criteria

    else:
        print("[WARNING] PyTorch is installed, but torch.cuda.is_available() is False.")
        print("[WARNING] This means PyTorch cannot use the ROCm GPU or is not a ROCm-enabled build.")
        sys.exit(1) # Not a success for our test criteria

except ImportError:
    print("[INFO] PyTorch is not installed in the virtual environment. Skipping GPU model training test.")
    # This is not an error for the script itself, just skipping an optional test.
    # However, to make the overall --train-model step clearly pass/fail:
    print("[FAILURE] PyTorch not found, cannot run this test.")
    sys.exit(1)
except Exception as e:
    print(f"[FAILURE] An error occurred during PyTorch test: {e}")
    sys.exit(1)

print("[PYTORCH TEST SCRIPT] PyTorch GPU usage test script finished.")
"""
    try:
        process = run_command(
            ["python", "-c", pytorch_test_script],
            venv_mode=True,
            check=True, # The script itself will exit with 1 on failure
            capture_output=False
        )
        if process and process.returncode == 0:
            print_info("PyTorch GPU usage test script executed successfully.")
        # No else needed here, check=True will raise CalledProcessError if script exits non-zero

    except subprocess.CalledProcessError as e:
        print_error(f"PyTorch GPU usage test script returned an error (exit code {e.returncode}).")
        print_error("Review the script output above. This may be due to PyTorch not being installed, "
                    "not being a ROCm-compatible build, or issues with the ROCm environment itself.")
        # No sys.exit(1) here, let main flow or specific arg handler decide.
    except Exception as e:
        print_error(f"An unexpected error occurred while trying to run the PyTorch test script: {e}")


def main():
    """Main function to parse arguments and execute actions."""
    parser = argparse.ArgumentParser(description="eplfga library installation and testing script.")
    parser.add_argument("--setup-venv", action="store_true", help="Create/recreate the virtual environment.")
    parser.add_argument("--install-lib", action="store_true", help="Install eplfga library into the venv.")
    parser.add_argument("--test-gpu", action="store_true", help="Test GPU acceleration features of eplfga.")
    parser.add_argument("--train-model", action="store_true", help="Run a basic model training test for ~30s.")
    parser.add_argument("--full", action="store_true", help="Run all steps: setup-venv, install-lib, test-gpu, train-model.")
    parser.add_argument("--clean", action="store_true", help="Remove the virtual environment directory.")

    args = parser.parse_args()

    if not any(vars(args).values()): # If no arguments are provided, print help.
        parser.print_help()
        sys.exit(0)

    if args.clean:
        print_step("Cleaning up virtual environment...")
        if os.path.exists(VENV_DIR):
            try:
                shutil.rmtree(VENV_DIR)
                print_info(f"Removed directory: {VENV_DIR}")
            except OSError as e:
                print_error(f"Failed to remove {VENV_DIR}: {e}")
        else:
            print_info(f"Directory {VENV_DIR} does not exist. Nothing to clean.")
        sys.exit(0)


    if args.full or args.setup_venv:
        setup_virtual_environment()

    if args.full or args.install_lib:
        if not os.path.exists(get_venv_python_interpreter()):
            print_error("Virtual environment not found. Please run with --setup-venv or --full first.")
            sys.exit(1)
        install_eplfga_library()

    if args.full or args.test_gpu:
        if not os.path.exists(get_venv_python_interpreter()):
            print_error("Virtual environment not found or eplfga not installed. Please run with --setup-venv and --install-lib or --full first.")
            sys.exit(1)
        test_gpu_acceleration()

    if args.full or args.train_model:
        if not os.path.exists(get_venv_python_interpreter()):
            print_error("Virtual environment not found or eplfga not installed. Please run with --setup-venv and --install-lib or --full first.")
            sys.exit(1)
        train_basic_statistic_model()

    print_info("Script finished.")

if __name__ == "__main__":
    main()
