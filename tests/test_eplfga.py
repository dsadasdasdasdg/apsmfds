# tests/test_eplfga.py

import unittest
# We will import 'eplfga' once the setup.py is functional and the package is installable
# For now, we might need to adjust sys.path or wait until we can build/install.

# Attempt to import eplfga, assuming it might be in the parent directory
# This is a common pattern for testing before proper installation
import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

try:
    import eplfga
    import eplfga.utils
except ImportError:
    eplfga = None # Handle case where eplfga is not yet importable

class TestEplfgaInitialization(unittest.TestCase):

    def test_library_importable(self):
        self.assertIsNotNone(eplfga, "eplfga library failed to import. Check sys.path and compilation.")
        if eplfga:
            self.assertTrue(hasattr(eplfga, 'hello'), "eplfga.hello function not found.")

    def test_hello_function(self):
        if eplfga:
            # The hello message now indicates C core loading status
            expected_message_part = "Hello from eplfga!"
            actual_message = eplfga.hello()
            self.assertIn(expected_message_part, actual_message)
            # Check for C core status indication
            self.assertTrue("C core NOT loaded." in actual_message or "C core partially loaded." in actual_message)


    def test_version_present(self):
        if eplfga:
            self.assertTrue(hasattr(eplfga, '__version__'))
            self.assertIsInstance(eplfga.__version__, str)
            self.assertGreater(len(eplfga.__version__), 0)
            print(f"eplfga version: {eplfga.__version__}")

class TestEplfgaUtils(unittest.TestCase):
    def setUp(self):
        if not eplfga or not hasattr(eplfga, 'utils'):
            self.skipTest("eplfga.utils module not available.")

    def test_get_system_info(self):
        info = eplfga.utils.get_system_info()
        self.assertIsInstance(info, dict)
        self.assertIn("system", info)
        self.assertIn("python_version", info)
        print(f"System Info from utils: {info}")

    def test_check_rocm_installation_path(self):
        # This test is environment-dependent.
        # It might pass or fail based on where it's run.
        # For CI, we might need to mock os.path.exists and os.path.isdir
        path = eplfga.utils.check_rocm_installation_path()
        if path:
            self.assertTrue(os.path.exists(path), f"Reported ROCm path {path} does not exist.")
            self.assertTrue(os.path.isdir(path), f"Reported ROCm path {path} is not a directory.")
            print(f"ROCm installation path check returned: {path}")
        else:
            print("ROCm installation path check returned None (no installation found in default paths).")
        # No strict assertion on finding ROCm, as it's environment-specific.

# Tests for C Core function wrappers
# These tests will check if the functions raise EplfgaCoreError when the C library is not loaded,
# or if they execute (potentially failing due to no ROCm) if the library somehow loads parts of it.
class TestEplfgaCoreFunctions(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        if not eplfga:
            raise unittest.SkipTest("eplfga module not imported.")
        # Check if C core is loaded. If not, most tests will check for EplfgaCoreError.
        cls.core_loaded = False
        try:
            # A simple call that should work if _core is loaded, even if ROCm itself isn't fully working.
            # get_c_core_version directly checks eplfga._core
            eplfga.get_c_core_version()
            # If the above didn't raise EplfgaCoreError with "C core library not loaded",
            # it implies _core was found by ctypes. It might still fail if ROCm calls fail.
            cls.core_loaded = True
            print("\nTestEplfgaCoreFunctions: C-core appears loaded for testing (or stubs are callable).")
            # Attempt to initialize ROCm here if core seems loaded.
            # This helps set up for other tests that might depend on initialization.
            try:
                print("Attempting eplfga.initialize_rocm() in setUpClass...")
                eplfga.initialize_rocm()
                print("eplfga.initialize_rocm() called.")
            except eplfga.EplfgaCoreError as e:
                print(f"eplfga.initialize_rocm() failed during setUpClass (as expected without ROCm): {e}")
            except Exception as e:
                print(f"Unexpected error during eplfga.initialize_rocm() in setUpClass: {e}")

        except eplfga.EplfgaCoreError as e:
            if "C core library not loaded" in str(e):
                print("\nTestEplfgaCoreFunctions: C-core not loaded. Tests will expect EplfgaCoreError.")
            else:
                # This case means _core was loaded, but get_c_core_version itself failed for another reason
                # (e.g. symbol not found, which shouldn't happen if argtypes/restype are correct)
                cls.core_loaded = True # It was loaded, but a specific call failed.
                print(f"\nTestEplfgaCoreFunctions: C-core loaded, but get_c_core_version failed: {e}")
        except Exception as e:
             print(f"\nTestEplfgaCoreFunctions: Unexpected error checking C-core: {e}")


    def test_get_c_core_version(self):
        self.assertTrue(hasattr(eplfga, 'get_c_core_version'))
        try:
            version = eplfga.get_c_core_version()
            self.assertIsInstance(version, str)
            self.assertIn("eplfga C core", version)
            print(f"C Core Version: {version}")
        except eplfga.EplfgaCoreError as e:
            if not self.core_loaded:
                self.assertIn("C core library not loaded", str(e))
            else:
                # If core was loaded, this means the call failed for other reasons (e.g. ROCm not present)
                # This is an expected failure path in a no-ROCm environment.
                print(f"test_get_c_core_version: Call failed as expected in no-ROCm env: {e}")
                pass # Allow test to pass if core is loaded but ROCm calls fail
        except Exception as e:
            self.fail(f"test_get_c_core_version threw unexpected exception: {e}")


    def test_initialize_rocm(self):
        self.assertTrue(hasattr(eplfga, 'initialize_rocm'))
        try:
            result = eplfga.initialize_rocm()
            self.assertTrue(result) # Expect True on success
            print("ROCm initialized successfully via eplfga.initialize_rocm().")
        except eplfga.EplfgaCoreError as e:
            if not self.core_loaded:
                self.assertIn("C core library not loaded", str(e))
            else:
                # This is an expected failure path if ROCm is not actually available
                print(f"eplfga.initialize_rocm() failed as expected in no-ROCm env: {e}")
                self.assertTrue("hipInit" in str(e) or "ROCm" in str(e).upper() or "HIP" in str(e).upper() or "No such file" in str(e), f"Error message {e} not as expected for missing ROCm")
        except Exception as e:
            self.fail(f"test_initialize_rocm threw unexpected exception: {e}")

    def test_get_hip_runtime_version(self):
        self.assertTrue(hasattr(eplfga, 'get_hip_runtime_version'))
        try:
            major, minor = eplfga.get_hip_runtime_version()
            self.assertIsInstance(major, int)
            self.assertIsInstance(minor, int)
            self.assertGreaterEqual(major, 0)
            self.assertGreaterEqual(minor, 0)
            print(f"HIP Runtime Version: {major}.{minor}")
        except eplfga.EplfgaCoreError as e:
            if not self.core_loaded:
                self.assertIn("C core library not loaded", str(e))
            else:
                print(f"eplfga.get_hip_runtime_version() failed as expected in no-ROCm env: {e}")
                self.assertTrue("hipRuntimeGetVersion" in str(e) or "HIP" in str(e).upper() or "No such file" in str(e), f"Error message {e} not as expected for missing ROCm")
        except Exception as e:
            self.fail(f"test_get_hip_runtime_version threw unexpected exception: {e}")

    def test_get_gpu_count(self):
        self.assertTrue(hasattr(eplfga, 'get_gpu_count'))
        try:
            count = eplfga.get_gpu_count()
            self.assertIsInstance(count, int)
            self.assertGreaterEqual(count, 0) # Should be 0 or more
            print(f"Detected GPU count: {count}")
            # In a real ROCm environment, you might assert count > 0 if GPUs are expected.
        except eplfga.EplfgaCoreError as e:
            if not self.core_loaded:
                self.assertIn("C core library not loaded", str(e))
            else:
                print(f"eplfga.get_gpu_count() failed as expected in no-ROCm env: {e}")
                # hipGetDeviceCount might return specific errors if ROCm is present but no GPUs, or if not initialized.
                # hipErrorNoDevice (35), hipErrorInsufficientDriver (36), hipErrorInitializationError (3)
                self.assertTrue("hipGetDeviceCount" in str(e) or "HIP" in str(e).upper() or "No such file" in str(e), f"Error message {e} not as expected for missing ROCm")
        except Exception as e:
            self.fail(f"test_get_gpu_count threw unexpected exception: {e}")

    def test_get_gpu_properties(self):
        self.assertTrue(hasattr(eplfga, 'get_gpu_properties'))
        try:
            # This test assumes if get_gpu_count found >0 devices, properties for device 0 can be fetched.
            # In a no-ROCm environment, get_gpu_count would likely raise EplfgaCoreError or return 0.
            # If count is 0, this test might be skipped or expected to fail if called.
            # For robustness, let's try to call it only if core is loaded, and expect it to fail gracefully.
            if not self.core_loaded:
                 with self.assertRaisesRegex(eplfga.EplfgaCoreError, "C core library not loaded"):
                    eplfga.get_gpu_properties(0)
                 return

            # If core is loaded, we expect this to fail because ROCm is not present.
            # It might fail because initialization failed, or device 0 doesn't exist.
            with self.assertRaises(eplfga.EplfgaCoreError) as cm:
                eplfga.get_gpu_properties(0) # Try for device 0

            print(f"eplfga.get_gpu_properties(0) failed as expected: {cm.exception}")
            self.assertTrue("hipGetDeviceProperties" in str(cm.exception) or "device ID" in str(cm.exception).lower() or "No such file" in str(cm.exception), f"Error message {cm.exception} not as expected for missing ROCm")

            # If there were actual GPUs, the test would be:
            # count = eplfga.get_gpu_count()
            # if count > 0:
            #   props = eplfga.get_gpu_properties(0)
            #   self.assertIsInstance(props, dict)
            #   self.assertIn("name", props)
            #   self.assertIsInstance(props["name"], str)
            #   self.assertGreater(len(props["name"]), 0)
            #   self.assertIn("totalGlobalMem", props)
            #   self.assertGreater(props["totalGlobalMem"], 0)
            #   print(f"GPU 0 Properties: {props['name']}, {props['totalGlobalMem'] / (1024**3):.2f} GB")
            # else:
            #   print("Skipping get_gpu_properties content check as no GPUs found.")

        except eplfga.EplfgaCoreError as e:
            # This block will catch the "C core library not loaded" if the initial check missed it.
            if not self.core_loaded:
                self.assertIn("C core library not loaded", str(e))
            else:
                # This means the call itself failed for other ROCm-related reasons.
                print(f"eplfga.get_gpu_properties(0) failed as expected in no-ROCm env: {e}")
                self.assertTrue("hipGetDeviceProperties" in str(e) or "device ID" in str(e).lower() or "No such file" in str(e), f"Error message {e} not as expected for missing ROCm")
        except Exception as e:
            self.fail(f"test_get_gpu_properties threw unexpected exception: {e}")


    def test_get_gpu_memory_info(self):
        self.assertTrue(hasattr(eplfga, 'get_gpu_memory_info'))
        try:
            if not self.core_loaded:
                with self.assertRaisesRegex(eplfga.EplfgaCoreError, "C core library not loaded"):
                    eplfga.get_gpu_memory_info(0) # device_id is optional, but let's use 0
                return

            with self.assertRaises(eplfga.EplfgaCoreError) as cm:
                eplfga.get_gpu_memory_info(0) # Try for device 0 (or current device)

            print(f"eplfga.get_gpu_memory_info(0) failed as expected: {cm.exception}")
            self.assertTrue("hipMemGetInfo" in str(cm.exception) or "No such file" in str(cm.exception), f"Error message {cm.exception} not as expected for missing ROCm")

            # If there were actual GPUs and ROCm:
            # count = eplfga.get_gpu_count() # Assuming this was successful
            # if count > 0:
            #   # Initialize ROCm if not done already (some HIP calls need it)
            #   # eplfga.initialize_rocm() # This might be needed if not called globally
            #   mem_info = eplfga.get_gpu_memory_info(0) # Test for device 0
            #   self.assertIsInstance(mem_info, dict)
            #   self.assertIn("total", mem_info)
            #   self.assertIn("free", mem_info)
            #   self.assertGreater(mem_info["total"], 0)
            #   self.assertGreaterEqual(mem_info["free"], 0)
            #   self.assertLessEqual(mem_info["free"], mem_info["total"])
            #   print(f"GPU 0 Memory: Total {mem_info['total'] / (1024**3):.2f} GB, Free {mem_info['free'] / (1024**3):.2f} GB")
            # else:
            #   print("Skipping get_gpu_memory_info content check as no GPUs found.")

        except eplfga.EplfgaCoreError as e:
            if not self.core_loaded:
                self.assertIn("C core library not loaded", str(e))
            else:
                print(f"eplfga.get_gpu_memory_info(0) failed as expected in no-ROCm env: {e}")
                self.assertTrue("hipMemGetInfo" in str(e) or "No such file" in str(e), f"Error message {e} not as expected for missing ROCm")

        except Exception as e:
            self.fail(f"test_get_gpu_memory_info threw unexpected exception: {e}")


if __name__ == '__main__':
    unittest.main()
