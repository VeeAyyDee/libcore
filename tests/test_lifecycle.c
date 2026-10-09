/* SPDX-License-Identifier: MPL-2.0 */
#include "libcore.h"
#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static bool enabled;
static bool allocation_fails;
static int open_result;
static uint32_t returned_handle;
static unsigned int allocations;
static unsigned int releases;
static unsigned int opens;
static unsigned int closes;
static void *live_allocation;
static uint32_t closed_handle;

bool libcore_platform_nvs_enabled(void)
{
    return enabled;
}

void *libcore_platform_allocate(size_t size)
{
    CHECK(size == 60);
    CHECK(live_allocation == NULL);
    ++allocations;
    if (allocation_fails) {
        return NULL;
    }
    live_allocation = malloc(size);
    CHECK(live_allocation != NULL);
    /* The implementation must zero even a dirty allocator result. */
    memset(live_allocation, 0xa5, size);
    return live_allocation;
}

void libcore_platform_release(void *allocation)
{
    CHECK(allocation != NULL);
    CHECK(allocation == live_allocation);
    ++releases;
    free(allocation);
    live_allocation = NULL;
}

int libcore_platform_open(uint32_t *handle)
{
    CHECK(live_allocation != NULL);
    CHECK(g_misc_nvs == NULL);
    CHECK((unsigned char *)handle == (unsigned char *)live_allocation + 56);
    const unsigned char *bytes = live_allocation;
    for (size_t i = 0; i < 60; ++i) {
        CHECK(bytes[i] == 0);
    }
    ++opens;
    /* A nonzero output on failure must also be closed before freeing. */
    *handle = returned_handle;
    return open_result;
}

void libcore_platform_close(uint32_t handle)
{
    CHECK(live_allocation != NULL);
    CHECK(handle != 0);
    ++closes;
    closed_handle = handle;
}

static void begin_case(void)
{
    CHECK(g_misc_nvs == NULL);
    CHECK(live_allocation == NULL);
    enabled = true;
    allocation_fails = false;
    open_result = 0;
    returned_handle = UINT32_C(0x12345678);
    allocations = 0;
    releases = 0;
    opens = 0;
    closes = 0;
    closed_handle = 0;
}

static uint32_t record_word(size_t offset)
{
    uint32_t value;
    memcpy(&value, (unsigned char *)g_misc_nvs + offset, sizeof(value));
    return value;
}

static void check_initial_record(uint32_t handle)
{
    CHECK(g_misc_nvs == live_allocation);
    const unsigned char *bytes = g_misc_nvs;
    for (size_t i = 0; i < 56; ++i) {
        CHECK(bytes[i] == 0);
    }
    CHECK(record_word(56) == handle);
}

static void test_initial_exports(void)
{
    CHECK(g_misc_nvs == NULL);
    CHECK(sizeof(g_log_mod) == 24);
    for (size_t i = 0; i < 6; ++i) {
        CHECK(g_log_mod[i] == 0);
    }
    CHECK(g_log_level == 0);
    CHECK(strcmp(libcore_reversion_git, "cleanroom-libcore-implementation-01") == 0);
}

static void test_success_and_repetition(void)
{
    begin_case();
    CHECK(misc_nvs_init() == 0);
    check_initial_record(returned_handle);
    void *original = g_misc_nvs;
    unsigned char snapshot[60];
    memset(g_misc_nvs, 0x6c, 56);
    memcpy(snapshot, g_misc_nvs, sizeof(snapshot));
    CHECK(misc_nvs_init() == 0);
    CHECK(g_misc_nvs == original);
    CHECK(memcmp(snapshot, g_misc_nvs, sizeof(snapshot)) == 0);
    CHECK(allocations == 1 && opens == 1);
    CHECK(misc_nvs_load(0) == 0x103);
    CHECK(memcmp(snapshot, g_misc_nvs, sizeof(snapshot)) == 0);
    misc_nvs_deinit();
    CHECK(g_misc_nvs == NULL && live_allocation == NULL);
    CHECK(closes == 1 && releases == 1 && closed_handle == returned_handle);
    misc_nvs_deinit();
    CHECK(closes == 1 && releases == 1);
    CHECK(misc_nvs_init() == 0);
    CHECK(allocations == 2 && opens == 2);
    misc_nvs_deinit();
    CHECK(closes == 2 && releases == 2);
}

static void test_disabled_nvs(void)
{
    begin_case();
    enabled = false;
    CHECK(misc_nvs_init() == 0);
    check_initial_record(0);
    CHECK(allocations == 1 && opens == 0);
    /* Enabling after acquisition must not create an owned handle. */
    enabled = true;
    misc_nvs_deinit();
    CHECK(releases == 1 && closes == 0);
}

static void test_allocation_failure_and_retry(void)
{
    begin_case();
    allocation_fails = true;
    CHECK(misc_nvs_init() == 0x101);
    CHECK(g_misc_nvs == NULL && live_allocation == NULL);
    CHECK(opens == 0 && releases == 0 && closes == 0);
    misc_nvs_deinit();
    allocation_fails = false;
    CHECK(misc_nvs_init() == 0);
    CHECK(allocations == 2 && opens == 1);
    misc_nvs_deinit();
}

static void test_open_failure_and_retry(void)
{
    begin_case();
    open_result = 0x1102;
    CHECK(misc_nvs_init() == 0x1102);
    CHECK(g_misc_nvs == NULL && live_allocation == NULL);
    CHECK(allocations == 1 && opens == 1 && releases == 1 && closes == 1);
    CHECK(closed_handle == returned_handle);
    CHECK(misc_nvs_restore() == 0x103);
    open_result = -37;
    CHECK(misc_nvs_init() == -37);
    CHECK(g_misc_nvs == NULL && live_allocation == NULL);
    CHECK(releases == 2 && closes == 2);
    open_result = 0;
    CHECK(misc_nvs_init() == 0);
    check_initial_record(returned_handle);
    misc_nvs_deinit();
    CHECK(releases == 3 && closes == 3);
}

static void test_direct_load(void)
{
    begin_case();
    allocation_fails = true;
    CHECK(misc_nvs_load(0) == 0x101);
    CHECK(g_misc_nvs == NULL);
    allocation_fails = false;
    open_result = 73;
    returned_handle = 0;
    CHECK(misc_nvs_load(0) == 73);
    CHECK(g_misc_nvs == NULL && live_allocation == NULL);
    CHECK(releases == 1 && closes == 0);
    open_result = 0;
    returned_handle = UINT32_C(0x12345678);
    CHECK(misc_nvs_load(0) == 0);
    check_initial_record(returned_handle);
    CHECK(misc_nvs_load(0) == 0x103);
    CHECK(misc_nvs_init() == 0x103);
    CHECK(allocations == 3 && opens == 2);
    misc_nvs_deinit();
    CHECK(releases == 2 && closes == 1);
    CHECK(misc_nvs_init() == 0);
    misc_nvs_deinit();
}

static void test_reset_preserves_bytes(void)
{
    begin_case();
    CHECK(misc_nvs_init() == 0);
    for (unsigned int mode = 0; mode <= UINT8_MAX; ++mode) {
        unsigned char expected[60];
        for (size_t i = 0; i < sizeof(expected); ++i) {
            expected[i] = (unsigned char)(i + mode);
        }
        memcpy(g_misc_nvs, expected, sizeof(expected));
        memset(expected + 8, 0, 4);
        int result = mode == 0 ? misc_nvs_restore() : misc_nvs_load((uint8_t)mode);
        CHECK(result == 0);
        CHECK(memcmp(expected, g_misc_nvs, sizeof(expected)) == 0);
    }
    CHECK(allocations == 1 && opens == 1 && releases == 0 && closes == 0);
    /* The original opened handle remains owned even if bytes 56..59 change. */
    misc_nvs_deinit();
    CHECK(closed_handle == returned_handle && closes == 1);
}

static void test_invalid_state(void)
{
    begin_case();
    CHECK(misc_nvs_restore() == 0x103);
    for (unsigned int mode = 1; mode <= UINT8_MAX; ++mode) {
        CHECK(misc_nvs_load((uint8_t)mode) == 0x103);
    }
    misc_nvs_deinit();
    misc_nvs_deinit();
    CHECK(allocations == 0 && opens == 0 && releases == 0 && closes == 0);
}

static void test_configuration_change(void)
{
    begin_case();
    CHECK(misc_nvs_init() == 0);
    enabled = false;
    CHECK(misc_nvs_init() == 0);
    CHECK(opens == 1);
    misc_nvs_deinit();
    CHECK(closes == 1 && closed_handle == returned_handle);
}

static void test_zero_opened_handle(void)
{
    begin_case();
    returned_handle = 0;
    CHECK(misc_nvs_init() == 0);
    check_initial_record(0);
    misc_nvs_deinit();
    CHECK(opens == 1 && closes == 0 && releases == 1);
}

int main(void)
{
    test_initial_exports();
    test_success_and_repetition();
    test_disabled_nvs();
    test_allocation_failure_and_retry();
    test_open_failure_and_retry();
    test_direct_load();
    test_reset_preserves_bytes();
    test_invalid_state();
    test_configuration_change();
    test_zero_opened_handle();
    CHECK(g_misc_nvs == NULL && live_allocation == NULL);
    puts("PASS: 10 lifecycle cases; all 255 nonzero modes checked in valid/invalid states");
    return EXIT_SUCCESS;
}
