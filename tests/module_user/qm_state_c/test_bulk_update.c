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
 * @brief Unit tests for QMS_REQUEST_BULK_UPDATE state changes in qm_state.c.
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
static void test_bulk_from_clean_one_device(void** state) {
    (void)state;

    // Run.
    const VqmbtDeviceInfo devices[VQMBT_MAX_DEVICES] = {
        [0] = {.name = "Test Device", .mac0 = 0x12345678, .mac1 = 0x9ABCDEF0, .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
    };
    const bool changed = qm_state_update_ui(&(QmsRequest){
        .id = QMS_REQUEST_BULK_UPDATE,
        .bulk.bluetooth_on = true,
        .bulk.num_devices = 1,
        .bulk.devices = devices,
    });

    // Verify.
    assert_true(changed);
    assert_string_equal(qm_state.buttons[0].device.name, "Test Device");
    assert_int_equal(qm_state.buttons[0].device.mac0, 0x12345678);
    assert_int_equal(qm_state.buttons[0].device.mac1, 0x9ABCDEF0);
    assert_int_equal(qm_state.buttons[0].device.state, VQMBT_SCE_BT_STATE_DISCONNECTED);
    assert_int_equal(qm_state.buttons[0].btn_state, BTNSTATE_DISCONNECTED);
    for (int i = 1; i < VQMBT_MAX_DEVICES; i++) {
        assert_string_equal(qm_state.buttons[i].device.name, "");
        assert_int_equal(qm_state.buttons[i].device.mac0, 0);
        assert_int_equal(qm_state.buttons[i].device.mac1, 0);
        assert_int_equal(qm_state.buttons[i].device.state, VQMBT_SCE_BT_STATE_UNKNOWN0);
        assert_int_equal(qm_state.buttons[i].btn_state, BTNSTATE_SLOT_EMPTY_DISABLED);
    }
}

/**
 * Test.
 */
static void test_bulk_from_clean_one_device_bt_off(void** state) {
    (void)state;

    // Run.
    const VqmbtDeviceInfo devices[VQMBT_MAX_DEVICES] = {
        [0] = {.name = "Test Device", .mac0 = 0x12345678, .mac1 = 0x9ABCDEF0, .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
    };
    const bool changed = qm_state_update_ui(&(QmsRequest){
        .id = QMS_REQUEST_BULK_UPDATE,
        .bulk.bluetooth_on = false,
        .bulk.num_devices = 1,
        .bulk.devices = devices,
    });

    // Verify.
    assert_true(changed);
    assert_string_equal(qm_state.buttons[0].device.name, "Test Device");
    assert_int_equal(qm_state.buttons[0].device.mac0, 0x12345678);
    assert_int_equal(qm_state.buttons[0].device.mac1, 0x9ABCDEF0);
    assert_int_equal(qm_state.buttons[0].device.state, VQMBT_SCE_BT_STATE_DISCONNECTED);
    assert_int_equal(qm_state.buttons[0].btn_state, BTNSTATE_BT_OFF_DISABLED);
    for (int i = 1; i < VQMBT_MAX_DEVICES; i++) {
        assert_string_equal(qm_state.buttons[i].device.name, "");
        assert_int_equal(qm_state.buttons[i].device.mac0, 0);
        assert_int_equal(qm_state.buttons[i].device.mac1, 0);
        assert_int_equal(qm_state.buttons[i].device.state, VQMBT_SCE_BT_STATE_UNKNOWN0);
        assert_int_equal(qm_state.buttons[i].btn_state, BTNSTATE_SLOT_EMPTY_DISABLED);
    }
}

/**
 * Test.
 */
static void test_bulk_from_clean_one_device_already_connected(void** state) {
    (void)state;

    // Run.
    const VqmbtDeviceInfo devices[VQMBT_MAX_DEVICES] = {
        [0] = {.name = "Test Device", .mac0 = 0x12345678, .mac1 = 0x9ABCDEF0, .state = VQMBT_SCE_BT_STATE_CONNECTED},
    };
    const bool changed = qm_state_update_ui(&(QmsRequest){
        .id = QMS_REQUEST_BULK_UPDATE,
        .bulk.bluetooth_on = true,
        .bulk.num_devices = 1,
        .bulk.devices = devices,
    });

    // Verify.
    assert_true(changed);
    assert_string_equal(qm_state.buttons[0].device.name, "Test Device");
    assert_int_equal(qm_state.buttons[0].device.mac0, 0x12345678);
    assert_int_equal(qm_state.buttons[0].device.mac1, 0x9ABCDEF0);
    assert_int_equal(qm_state.buttons[0].device.state, VQMBT_SCE_BT_STATE_CONNECTED);
    assert_int_equal(qm_state.buttons[0].btn_state, BTNSTATE_CONNECTED);
    for (int i = 1; i < VQMBT_MAX_DEVICES; i++) {
        assert_string_equal(qm_state.buttons[i].device.name, "");
        assert_int_equal(qm_state.buttons[i].device.mac0, 0);
        assert_int_equal(qm_state.buttons[i].device.mac1, 0);
        assert_int_equal(qm_state.buttons[i].device.state, VQMBT_SCE_BT_STATE_UNKNOWN0);
        assert_int_equal(qm_state.buttons[i].btn_state, BTNSTATE_SLOT_EMPTY_DISABLED);
    }
}

/**
 * Test.
 */
static void test_bulk_add_then_readd(void** state) {
    (void)state;

    // Add device.
    const VqmbtDeviceInfo devices[VQMBT_MAX_DEVICES] = {
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

    // Re-add same device.
    changed = qm_state_update_ui(&(QmsRequest){
        .id = QMS_REQUEST_BULK_UPDATE,
        .bulk.bluetooth_on = true,
        .bulk.num_devices = 1,
        .bulk.devices = devices,
    });

    // Verify device added.
    assert_false(changed);
    assert_string_equal(qm_state.buttons[0].device.name, "Test Device");
    assert_int_equal(qm_state.buttons[0].device.mac0, 0x12345678);
    assert_int_equal(qm_state.buttons[0].device.mac1, 0x9ABCDEF0);
    assert_int_equal(qm_state.buttons[0].device.state, VQMBT_SCE_BT_STATE_DISCONNECTED);
    assert_int_equal(qm_state.buttons[0].btn_state, BTNSTATE_DISCONNECTED);
}

/**
 * Test.
 */
static void test_bulk_add_remove_only_device(void** state) {
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

    // Remove the device.
    memset(devices, 0, sizeof(devices));
    changed = qm_state_update_ui(&(QmsRequest){
        .id = QMS_REQUEST_BULK_UPDATE,
        .bulk.bluetooth_on = true,
        .bulk.num_devices = 0,
        .bulk.devices = devices,
    });

    // Verify device removed.
    assert_true(changed);
    for (int i = 0; i < VQMBT_MAX_DEVICES; i++) {
        assert_string_equal(qm_state.buttons[i].device.name, "");
        assert_int_equal(qm_state.buttons[i].device.mac0, 0);
        assert_int_equal(qm_state.buttons[i].device.mac1, 0);
        assert_int_equal(qm_state.buttons[i].device.state, VQMBT_SCE_BT_STATE_UNKNOWN0);
        assert_int_equal(qm_state.buttons[i].btn_state, BTNSTATE_SLOT_EMPTY_DISABLED);
    }
}

/**
 * Test.
 */
static void test_bulk_add_remove_second_device(void** state) {
    (void)state;

    // Add first device.
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
    assert_string_equal(qm_state.buttons[2].device.name, "");
    assert_int_equal(qm_state.buttons[2].device.mac0, 0);
    assert_int_equal(qm_state.buttons[2].device.mac1, 0);
    assert_int_equal(qm_state.buttons[2].device.state, VQMBT_SCE_BT_STATE_UNKNOWN0);
    assert_int_equal(qm_state.buttons[2].btn_state, BTNSTATE_SLOT_EMPTY_DISABLED);

    // Add second device.
    memcpy(devices,
           (VqmbtDeviceInfo[VQMBT_MAX_DEVICES]){
               [0] = {.name = "Test Device",
                      .mac0 = 0x12345678,
                      .mac1 = 0x9ABCDEF0,
                      .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
               [1] = {.name = "Test Devic2",
                      .mac0 = 0x22345672,
                      .mac1 = 0x2ABCDEF2,
                      .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
           },
           sizeof(devices));
    changed = qm_state_update_ui(&(QmsRequest){
        .id = QMS_REQUEST_BULK_UPDATE,
        .bulk.bluetooth_on = true,
        .bulk.num_devices = 2,
        .bulk.devices = devices,
    });

    // Verify device added.
    assert_true(changed);
    assert_string_equal(qm_state.buttons[0].device.name, "Test Device");
    assert_int_equal(qm_state.buttons[0].device.mac0, 0x12345678);
    assert_int_equal(qm_state.buttons[0].device.mac1, 0x9ABCDEF0);
    assert_int_equal(qm_state.buttons[0].device.state, VQMBT_SCE_BT_STATE_DISCONNECTED);
    assert_int_equal(qm_state.buttons[0].btn_state, BTNSTATE_DISCONNECTED);
    assert_string_equal(qm_state.buttons[1].device.name, "Test Devic2");
    assert_int_equal(qm_state.buttons[1].device.mac0, 0x22345672);
    assert_int_equal(qm_state.buttons[1].device.mac1, 0x2ABCDEF2);
    assert_int_equal(qm_state.buttons[1].device.state, VQMBT_SCE_BT_STATE_DISCONNECTED);
    assert_int_equal(qm_state.buttons[1].btn_state, BTNSTATE_DISCONNECTED);
    assert_string_equal(qm_state.buttons[2].device.name, "");
    assert_int_equal(qm_state.buttons[2].device.mac0, 0);
    assert_int_equal(qm_state.buttons[2].device.mac1, 0);
    assert_int_equal(qm_state.buttons[2].device.state, VQMBT_SCE_BT_STATE_UNKNOWN0);
    assert_int_equal(qm_state.buttons[2].btn_state, BTNSTATE_SLOT_EMPTY_DISABLED);

    // Remove the second device.
    memcpy(devices,
           (VqmbtDeviceInfo[VQMBT_MAX_DEVICES]){
               [0] = {.name = "Test Device",
                      .mac0 = 0x12345678,
                      .mac1 = 0x9ABCDEF0,
                      .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
           },
           sizeof(devices));
    changed = qm_state_update_ui(&(QmsRequest){
        .id = QMS_REQUEST_BULK_UPDATE,
        .bulk.bluetooth_on = true,
        .bulk.num_devices = 1,
        .bulk.devices = devices,
    });

    // Verify device removed.
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
    assert_string_equal(qm_state.buttons[2].device.name, "");
    assert_int_equal(qm_state.buttons[2].device.mac0, 0);
    assert_int_equal(qm_state.buttons[2].device.mac1, 0);
    assert_int_equal(qm_state.buttons[2].device.state, VQMBT_SCE_BT_STATE_UNKNOWN0);
    assert_int_equal(qm_state.buttons[2].btn_state, BTNSTATE_SLOT_EMPTY_DISABLED);
}

/**
 * Test bulk update from no devices to no devices.
 */
static void test_bulk_from_clean_no_devices(void** state) {
    (void)state;

    // Run.
    const VqmbtDeviceInfo devices[VQMBT_MAX_DEVICES] = {0};
    const bool changed = qm_state_update_ui(&(QmsRequest){
        .id = QMS_REQUEST_BULK_UPDATE,
        .bulk.bluetooth_on = true,
        .bulk.num_devices = 0,
        .bulk.devices = devices,
    });

    // Verify.
    assert_false(changed);
    for (int i = 0; i < VQMBT_MAX_DEVICES; i++) {
        assert_string_equal(qm_state.buttons[i].device.name, "");
        assert_int_equal(qm_state.buttons[i].device.mac0, 0);
        assert_int_equal(qm_state.buttons[i].device.mac1, 0);
        assert_int_equal(qm_state.buttons[i].device.state, VQMBT_SCE_BT_STATE_UNKNOWN0);
        assert_int_equal(qm_state.buttons[i].btn_state, BTNSTATE_SLOT_EMPTY_DISABLED);
    }
}

/**
 * Test.
 */
static void test_bulk_from_clean_max_devices(void** state) {
    (void)state;

    // Run.
    const VqmbtDeviceInfo devices[] = {
        [0] = {.name = "Test Device 0", .mac0 = 0x00000000, .mac1 = 0x9ABCDEF0, .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
        [1] = {.name = "Test Device 1", .mac0 = 0x00000001, .mac1 = 0x9ABCDEF1, .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
        [2] = {.name = "Test Device 2", .mac0 = 0x00000002, .mac1 = 0x9ABCDEF2, .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
        [3] = {.name = "Test Device 3", .mac0 = 0x00000003, .mac1 = 0x9ABCDEF3, .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
        [4] = {.name = "Test Device 4", .mac0 = 0x00000004, .mac1 = 0x9ABCDEF4, .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
        [5] = {.name = "Test Device 5", .mac0 = 0x00000005, .mac1 = 0x9ABCDEF5, .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
        [6] = {.name = "Test Device 6", .mac0 = 0x00000006, .mac1 = 0x9ABCDEF6, .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
        [7] = {.name = "Test Device 7", .mac0 = 0x00000007, .mac1 = 0x9ABCDEF7, .state = VQMBT_SCE_BT_STATE_DISCONNECTED},
    };
    const bool changed = qm_state_update_ui(&(QmsRequest){
        .id = QMS_REQUEST_BULK_UPDATE,
        .bulk.bluetooth_on = true,
        .bulk.num_devices = VQMBT_MAX_DEVICES,
        .bulk.devices = devices,
    });

    // Verify.
    assert_true(changed);
    assert_string_equal(qm_state.buttons[0].device.name, "Test Device 0");
    assert_string_equal(qm_state.buttons[1].device.name, "Test Device 1");
    assert_string_equal(qm_state.buttons[2].device.name, "Test Device 2");
    assert_string_equal(qm_state.buttons[3].device.name, "Test Device 3");
    assert_string_equal(qm_state.buttons[4].device.name, "Test Device 4");
    assert_string_equal(qm_state.buttons[5].device.name, "Test Device 5");
    assert_string_equal(qm_state.buttons[6].device.name, "Test Device 6");
    assert_string_equal(qm_state.buttons[7].device.name, "Test Device 7");
    assert_int_equal(qm_state.buttons[0].device.mac0, 0x00000000);
    assert_int_equal(qm_state.buttons[1].device.mac0, 0x00000001);
    assert_int_equal(qm_state.buttons[2].device.mac0, 0x00000002);
    assert_int_equal(qm_state.buttons[3].device.mac0, 0x00000003);
    assert_int_equal(qm_state.buttons[4].device.mac0, 0x00000004);
    assert_int_equal(qm_state.buttons[5].device.mac0, 0x00000005);
    assert_int_equal(qm_state.buttons[6].device.mac0, 0x00000006);
    assert_int_equal(qm_state.buttons[7].device.mac0, 0x00000007);
    assert_int_equal(qm_state.buttons[0].device.mac1, 0x9ABCDEF0);
    assert_int_equal(qm_state.buttons[1].device.mac1, 0x9ABCDEF1);
    assert_int_equal(qm_state.buttons[2].device.mac1, 0x9ABCDEF2);
    assert_int_equal(qm_state.buttons[3].device.mac1, 0x9ABCDEF3);
    assert_int_equal(qm_state.buttons[4].device.mac1, 0x9ABCDEF4);
    assert_int_equal(qm_state.buttons[5].device.mac1, 0x9ABCDEF5);
    assert_int_equal(qm_state.buttons[6].device.mac1, 0x9ABCDEF6);
    assert_int_equal(qm_state.buttons[7].device.mac1, 0x9ABCDEF7);
    for (int i = 0; i < VQMBT_MAX_DEVICES; i++) {
        assert_int_equal(qm_state.buttons[0].device.state, VQMBT_SCE_BT_STATE_DISCONNECTED);
        assert_int_equal(qm_state.buttons[0].btn_state, BTNSTATE_DISCONNECTED);
    }
    static_assert(sizeof(devices) / sizeof(devices[0]) == VQMBT_MAX_DEVICES, "VQMBT_MAX_DEVICES changed, test outdated");
}

/**
 * Entrypoint for all tests in this file.
 */
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup(test_bulk_from_clean_one_device, setup),
        cmocka_unit_test_setup(test_bulk_from_clean_one_device_bt_off, setup),
        cmocka_unit_test_setup(test_bulk_from_clean_one_device_already_connected, setup),
        cmocka_unit_test_setup(test_bulk_add_then_readd, setup),
        cmocka_unit_test_setup(test_bulk_add_remove_only_device, setup),
        cmocka_unit_test_setup(test_bulk_add_remove_second_device, setup),
        cmocka_unit_test_setup(test_bulk_from_clean_no_devices, setup),
        cmocka_unit_test_setup(test_bulk_from_clean_max_devices, setup),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
