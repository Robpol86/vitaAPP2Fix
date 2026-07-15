/*
This file is part of vitaQmBluetooth.
Copyright © 2026 Robpol86

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

/******************************************************************************
 * @file
 * @brief Unit tests for bluetooth.c.
 ******************************************************************************/

#include <cmocka.h>
#include <fff.h>

// Include source code to test.
#define _PSP2KERN_KERNEL_CPU_H_      // Blocks vitasdk cpu.h
#define _PSP2KERN_KERNEL_SYSCLIB_H_  // Blocks vitasdk sysclib.h
#include "bluetooth.c"

// Setup mocks.
DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, ksceBtGetConfiguration);
FAKE_VALUE_FUNC(int, ksceBtGetConnectingInfo, unsigned int, unsigned int);
FAKE_VALUE_FUNC(int, ksceBtGetRegisteredInfo, int, int, SceBtRegisteredInfo*, SceSize);
FAKE_VALUE_FUNC(int, ksceBtStartConnect, unsigned int, unsigned int);
FAKE_VALUE_FUNC(int, ksceBtStartDisconnect, unsigned int, unsigned int);
FAKE_VALUE_FUNC(int, ksceKernelCopyToUser, void*, const void*, SceSize);
FAKE_VALUE_FUNC(int, ksceKernelDelayThread, SceUInt);
FAKE_VALUE_FUNC(int, ksceKernelGetThreadId);
FAKE_VALUE_FUNC(int, ksceRtcGetCurrentClock, SceDateTime*, int);
FAKE_VALUE_FUNC_VARARG(int, ksceKernelPrintf, const char*, ...);
FAKE_VOID_FUNC_VARARG(logfile_write_line, int, int, int, const char*, ...);

/**
 * Setup test fixture. Called before each test.
 */
static int setup(void** state) {
    (void)state;

    // Reset fff.
    RESET_FAKE(ksceBtGetConfiguration);
    FFF_RESET_HISTORY();

    return 0;
}

/**
 * Test.
 */
static void test_kvqmbt_bluetooth_state(void** state) {
    (void)state;

    // Test state == 0x9
    ksceBtGetConfiguration_fake.return_val = 0x9;
    bool ret = kvqmbt_bluetooth_state();
    assert_true(ret);

    // Test state == 0x0
    ksceBtGetConfiguration_fake.return_val = 0x0;
    ret = kvqmbt_bluetooth_state();
    assert_false(ret);
}

/**
 * Entrypoint for all tests in this file.
 */
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup(test_kvqmbt_bluetooth_state, setup),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
