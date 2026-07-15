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
 * @brief Unit tests for non-bulk state changes in qm_state.c.
 ******************************************************************************/

#include <cmocka.h>
#include <fff.h>

// Include source code to test.
#include "qm_state.c"

// Setup mocks.
DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, QuickMenuRebornSetWidgetColor, const char*, float, float, float, float);
FAKE_VALUE_FUNC(int, QuickMenuRebornSetWidgetLabel, const char*, const char*);
FAKE_VALUE_FUNC(int, kvqmbt_connect_device, unsigned int, unsigned int);
FAKE_VALUE_FUNC(int, kvqmbt_disconnect_device, unsigned int, unsigned int);
FAKE_VALUE_FUNC(int, sceKernelCreateLwMutex, SceKernelLwMutexWork*, const char*, unsigned int, int,
                const SceKernelLwMutexOptParam*);
FAKE_VALUE_FUNC(int, sceKernelDeleteLwMutex, SceKernelLwMutexWork*);
FAKE_VALUE_FUNC(int, sceKernelGetThreadId);
FAKE_VALUE_FUNC(int, sceKernelLockLwMutex, SceKernelLwMutexWork*, int, unsigned int*);
FAKE_VALUE_FUNC(int, sceKernelUnlockLwMutex, SceKernelLwMutexWork*, int);
FAKE_VALUE_FUNC(int, sceRtcGetCurrentClock, SceDateTime*, int);
FAKE_VALUE_FUNC_VARARG(int, sceClibPrintf, const char*, ...);
FAKE_VALUE_FUNC_VARARG(int, sceClibSnprintf, char*, SceSize, const char*, ...);
FAKE_VOID_FUNC_VARARG(logfile_write_line, int, int, int, const char*, ...);
void* sceClibMemcpy(void* d, const void* s, SceSize n) { return memcpy(d, s, n); }
void* sceClibMemset(void* d, int c, SceSize n) { return memset(d, c, n); }

/**
 * Setup test fixture. Called before each test.
 */
static int setup(void** state) {
    (void)state;

    // Reset state to empty.
    qm_state = (QmState){0};

    // Reset fff.
    FFF_RESET_HISTORY();

    return 0;
}

/**
 * Test.
 */
static void test_bt_on_off_on_one_device(void** state) {
    (void)state;

    // Add device.
    VqmbtDeviceInfo devices[VQMBT_MAX_DEVICES] = {
        [0] = {.name = "Test Device", .mac0 = 0x12345678, .mac1 = 0x9ABCDEF0, .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
    };
    bool changed = qm_state_update_ui(&(QmsRequest){
        .id = QMS_REQUEST_BULK_UPDATE,
        .bulk.bluetooth_on = true,
        .bulk.num_devices = 1,
        .bulk.devices = devices,
    });

    // Verify device added.
    assert_true(changed);
    assert_string_equal(qm_state.buttons[0].device.name, "Test Device");
    assert_int_equal(qm_state.buttons[0].device.mac0, 0x12345678);
    assert_int_equal(qm_state.buttons[0].device.mac1, 0x9ABCDEF0);
    assert_int_equal(qm_state.buttons[0].device.state, VQMBT_SCE_BT_STATE_DISCONNECTED);
    assert_int_equal(qm_state.buttons[0].btn_state, BTNSTATE_DISCONNECTED);
    assert_string_equal(qm_state.buttons[1].device.name, "");
    assert_int_equal(qm_state.buttons[1].device.mac0, 0);
    assert_int_equal(qm_state.buttons[1].device.mac1, 0);
    assert_int_equal(qm_state.buttons[1].device.state, VQMBT_SCE_BT_STATE_UNKNOWN0);
    assert_int_equal(qm_state.buttons[1].btn_state, BTNSTATE_SLOT_EMPTY_DISABLED);

    // Turn off bluetooth.
    changed = qm_state_update_ui(&(QmsRequest){.id = QMS_REQUEST_BLUETOOTH_OFF});

    // Verify bluetooth off.
    assert_true(changed);
    assert_string_equal(qm_state.buttons[0].device.name, "Test Device");
    assert_int_equal(qm_state.buttons[0].device.mac0, 0x12345678);
    assert_int_equal(qm_state.buttons[0].device.mac1, 0x9ABCDEF0);
    assert_int_equal(qm_state.buttons[0].device.state, VQMBT_SCE_BT_STATE_DISCONNECTED);
    assert_int_equal(qm_state.buttons[0].btn_state, BTNSTATE_BT_OFF_DISABLED);
    assert_string_equal(qm_state.buttons[1].device.name, "");
    assert_int_equal(qm_state.buttons[1].device.mac0, 0);
    assert_int_equal(qm_state.buttons[1].device.mac1, 0);
    assert_int_equal(qm_state.buttons[1].device.state, VQMBT_SCE_BT_STATE_UNKNOWN0);
    assert_int_equal(qm_state.buttons[1].btn_state, BTNSTATE_SLOT_EMPTY_DISABLED);

    // Turn on bluetooth.
    changed = qm_state_update_ui(&(QmsRequest){.id = QMS_REQUEST_BLUETOOTH_ON});

    // Verify bluetooth on.
    assert_true(changed);
    assert_string_equal(qm_state.buttons[0].device.name, "Test Device");
    assert_int_equal(qm_state.buttons[0].device.mac0, 0x12345678);
    assert_int_equal(qm_state.buttons[0].device.mac1, 0x9ABCDEF0);
    assert_int_equal(qm_state.buttons[0].device.state, VQMBT_SCE_BT_STATE_DISCONNECTED);
    assert_int_equal(qm_state.buttons[0].btn_state, BTNSTATE_DISCONNECTED);
    assert_string_equal(qm_state.buttons[1].device.name, "");
    assert_int_equal(qm_state.buttons[1].device.mac0, 0);
    assert_int_equal(qm_state.buttons[1].device.mac1, 0);
    assert_int_equal(qm_state.buttons[1].device.state, VQMBT_SCE_BT_STATE_UNKNOWN0);
    assert_int_equal(qm_state.buttons[1].btn_state, BTNSTATE_SLOT_EMPTY_DISABLED);
}

/**
 * Entrypoint for all tests in this file.
 */
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup(test_bt_on_off_on_one_device, setup),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
