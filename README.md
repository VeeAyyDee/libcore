# Original libcore replacement

This is an original readable C implementation of the specified ESP32-S3,
ESP-IDF 6.0.3 `libcore.a` Wi-Fi NVS lifecycle. It does not implement the MAC,
PHY, or the other Wi-Fi archives. The replacement revision string is
`cleanroom-libcore-implementation-01`; it is not a vendor commit identifier.

The implementation uses only the supervisor's behavioral contracts and the
provided public SDK declaration. Input hashes are recorded in `PROVENANCE.json`.
The supplied header is an interface reference; target builds include the actual
SDK header. New source, build files, tests, and documentation are MPL-2.0.

Fresh original/replacement hardware tests passed 124 checks across four cold
and warm boots, with NVS calibration persistence and full flash restoration.
Independent host comparisons matched 65 calls across 12 lifecycle scenarios.
See `VALIDATION.md` for exact scope and remaining limits.

MPL-2.0 requires recipients of distributed covered software to have access to
its covered source, including modifications. Separate application files can
remain proprietary; private changes need not be shared. Upstream contributions
are welcome but not required. The supplied SDK header remains Apache-2.0; see
`THIRD_PARTY_NOTICES.md` and `LICENSES/Apache-2.0.txt`.

## SDK integration

Keep this component's directory named `libcore`. Add it to the application's
`EXTRA_COMPONENT_DIRS` before `project()`, explicitly select it in `COMPONENTS`
or the application's component `REQUIRES`, and include the replacement hook
after `project()`:

```cmake
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "/absolute/path/to/libcore")
set(COMPONENTS main libcore)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(my_app)
include("/absolute/path/to/libcore/cmake/replace_core.cmake")
```

Alternatively retain an existing `COMPONENTS` list and add `libcore` to it, or
add `REQUIRES libcore` in the application's main component. Merely adding
`EXTRA_COMPONENT_DIRS` is insufficient when the app restricts its components.
The component itself declares `REQUIRES esp_wifi`.

The component name is `libcore` and its output name is `core`, producing
`libcore.a`. The hook obtains `COMPONENT_LIB` using
`idf_component_get_property`, reads the target's `BINARY_DIR` and `OUTPUT_NAME`, and changes
the existing `esp_wifi_core` imported target's `IMPORTED_LOCATION` to that
archive. It neither uses `$<TARGET_FILE:...>` nor adds an imported-target
dependency edge, avoiding a known cycle in the SDK's archive graph. Explicit
component selection makes the source archive part of the application build.

The adapter rejects targets other than ESP32-S3 and SDK versions other than
6.0.3. Compile-time assertions check the real SDK's allocation, free, NVS open,
and NVS close callback offsets. A minimal ordinary SDK application is in
`example/`; from that directory use `idf.py set-target esp32s3` and `idf.py build`
with ESP-IDF 6.0.3 activated. Its standard Wi-Fi initialization/deinitialization
requires no application API changes, credentials, connection, or NVS erase.

## ABI and behavior

`include/libcore.h` declares `misc_nvs_load`, `misc_nvs_init`,
`misc_nvs_deinit`, `misc_nvs_restore`, and the four exported data symbols.
`g_misc_nvs` starts null and points to a 60-byte allocation after acquisition:

| Byte range | Meaning |
| --- | --- |
| 0–7 | Reserved, preserved during reset |
| 8–11 | 32-bit reset word |
| 12–55 | Opaque, preserved during reset |
| 56–59 | 32-bit NVS handle, preserved during reset |

`g_log_mod` is six initially zero 32-bit words; `g_log_level` is an initially
zero 32-bit word. `libcore_reversion_git` is a pointer to the honest revision
string. Pointer data symbols are four bytes on the guarded target.

`load(0)` allocates and clears all 60 bytes. If menuconfig word 9 is nonzero,
the adapter opens namespace `misc` in read/write mode 1. Publication occurs
only after acquisition succeeds. Allocation failure returns `0x101`; an open
failure returns its exact error, closes a nonzero output handle, and frees the
temporary allocation. A later attempt may succeed.

Successful `init()` calls are idempotent. Nonzero `load` modes and `restore()`
clear only bytes 8–11. `deinit()` closes the owned nonzero handle, frees the
record, clears the pointer and initialization guard, and is safe to repeat.
NVS-disabled acquisition still allocates and clears the record without an NVS
open or close. The platform dependencies are the SDK globals `g_osi_funcs_p`
and `g_wifi_menuconfig`, plus standard C memory initialization; adapter helper
functions are supplied inside this archive. There is no logging dependency.

## Deliberate safer behavior and limits

- Reset before acquisition returns `0x103` instead of dereferencing null.
- Repeated direct `load(0)` returns `0x103` instead of overwriting an allocation.
- Failed direct loads are transactional and do not leak an allocation or a
  nonzero handle written by the failing open callback.
- Ownership is recorded separately, so changing the NVS configuration after
  initialization cannot suppress the necessary close. Changing the record's
  handle bytes also cannot redirect closure to an unowned handle.
- A successful direct load does not set `init()`'s guard. Calling `init()` on
  that existing direct-load record returns `0x103` and preserves it. Call
  `deinit()` before switching from direct acquisition to `init()`.

Only the normal serialized SDK lifecycle is supported. This code is not
thread-safe or reentrant; it assumes initialized SDK callbacks and valid
configuration storage. Consumers may change opaque record bytes, but must not
replace `g_misc_nvs` or free its allocation. No meaning is assigned to opaque
bits. The contract does not establish other archive behaviors or portability
to other SDK versions/targets. Hardware and real SDK integration must be
validated independently; host tests alone cannot establish either.

## Host validation

```sh
cmake -S tests -B build-host -DLIBCORE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build-host --output-on-failure -V
```

The host uses a separate small platform mock; it does not emulate 32-bit
function-pointer offsets. Strict warnings are errors. Ten lifecycle cases cover
initial data, success, disabled NVS, allocation/open errors, exact positive and
negative error propagation, failure handle cleanup, retries, repeated init and
deinit, reset byte preservation, configuration changes, zero handles, direct
loads, and invalid states. All 255 nonzero modes are checked both with and
without a record. The CMake test checks archive path resolution (including
spaces) and absence of manual imported-target dependencies against a fixture.
It does not replace a real IDF build.

On a ptrace-based sandbox, LeakSanitizer may terminate with “does not work under
ptrace.” Set `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` in that environment;
AddressSanitizer and UndefinedBehaviorSanitizer remain active, and the mock
still checks every allocation/free balance. See `VALIDATION.md` for the actual
implementation-side results and remaining validation limits.
