# Implementation-side validation

Supervisor validation subsequently passed against ESP-IDF 6.0.3 / ESP32-S3:

- Real SDK reference and replacement builds. The replacement firmware map
  selects this archive and excludes the vendor libcore archive; four required
  functions and four exported data names/sizes were checked.
- Independent original-Xtensa-instruction execution versus compiled fresh C:
  12 lifecycle scenarios, 65 calls. Compared return values, all live record
  bytes, and allocation/NVS/free call order. External OS calls were mocked.
- Fresh on-board A/B: original and replacement each booted with blank test NVS
  and then warm NVS. All 124 checks across four boots passed: Wi-Fi init,
  passive beacon scan, restore/deinit, full cold calibration, saved 1,904-byte
  calibration, and partial calibration with the saved data after reboot.
- Original full 16 MiB flash restored and verified; board reset succeeded.

See `validation/results.json` for scope, hashes, and limitations. Tested runtime
source was frozen before flashing. Final licensing changed only SPDX comments
from the private draft's 0BSD designation to MPL-2.0; no runtime logic changed.
These bounded results do not establish concurrency or all-input equivalence.

Runtime and build hook are frozen for the supervisor's independent validation.

Host compiler: GNU C 13.3.0. Build system: CMake 3.28.3.

The final implementation-side run used:

```sh
cmake -S tests -B build-host -DLIBCORE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build-host --output-on-failure -V
```

Result: **2/2 CTest tests passed**, with zero compiler warnings (warnings are
errors), no AddressSanitizer or UndefinedBehaviorSanitizer reports:

- `libcore_lifecycle`: 10 lifecycle cases; all 255 nonzero modes checked in
  valid and invalid states. Includes nonzero failure-handle cleanup, zero
  failure outputs, allocation balance checks, and byte-for-byte reset checks.
- `libcore_cmake_hook`: replacement archive resolves inside the source
  component target's binary directory, including a path with spaces; no manual
  imported-target dependencies are introduced.

The initial attempt with `detect_leaks=1` failed in the LeakSanitizer runtime:
“LeakSanitizer does not work under ptrace (strace, gdb, etc).” Therefore this run
does not claim LeakSanitizer coverage. ASan/UBSan remain enabled, and the host
mock independently verifies allocation/release ownership and balance.

An initial CMake fixture failed while the supervisor corrected the hook to use
the CMake target's `BINARY_DIR` property. The fixture was updated to model this
public build API correction and passed. No runtime change resulted from it.

The implementation agent did not read vendor binaries, disassembly,
reconstructed implementations, private comparison tests, credentials, or other
agents' outputs; did not access networks or hardware; and did not inspect the
SDK installation. Target offsets and version are guarded by the source's
compile-time assertions. Real SDK, independent comparison, and hardware results
are supplied separately by the supervisor; the host results here make no
hardware-success claim.
