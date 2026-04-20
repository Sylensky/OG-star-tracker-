// TDD Gate test for Epic 0.5.2 - Laser Pointer GPIO Control
// Verifies the laser accessory JSON contract via serializeAccessorySnapshots().
// Importing laser_accessory.h ensures this fails to compile until Milestone 1
// is implemented.
// Run with: pio test -e native_test

#include <string.h>
#include <unity.h>

#include "functions/accessories/accessory.h"
// Importing the laser header causes a compile failure until the file is created.
#include "functions/accessories/laser_accessory.h"

void setUp(void)
{
}
void tearDown(void)
{
}

void test_laser_snapshot_is_binary_actuator_type()
{
    AccessorySnapshot snap{};
    snap.name = "laser";
    snap.type = AccessoryType::BinaryActuator;
    snap.initialized = true;
    snap.supported = true;
    snap.state = false;

    char buf[256] = {};
    serializeAccessorySnapshots(&snap, 1, buf, sizeof(buf));

    TEST_ASSERT_NOT_NULL(strstr(buf, "\"laser\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state\""));
    // Actuator must NOT carry sensor-only fields
    TEST_ASSERT_NULL(strstr(buf, "rawAdc"));
}

void test_laser_snapshot_state_off_serializes_false()
{
    AccessorySnapshot snap{};
    snap.name = "laser";
    snap.type = AccessoryType::BinaryActuator;
    snap.initialized = true;
    snap.supported = true;
    snap.state = false;

    char buf[256] = {};
    serializeAccessorySnapshots(&snap, 1, buf, sizeof(buf));

    TEST_ASSERT_NOT_NULL(strstr(buf, "false"));
}

void test_laser_snapshot_state_on_serializes_true()
{
    AccessorySnapshot snap{};
    snap.name = "laser";
    snap.type = AccessoryType::BinaryActuator;
    snap.initialized = true;
    snap.supported = true;
    snap.state = true;

    char buf[256] = {};
    serializeAccessorySnapshots(&snap, 1, buf, sizeof(buf));

    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state\":true"));
}

void test_laser_snapshot_unsupported_board_shows_supported_false()
{
    AccessorySnapshot snap{};
    snap.name = "laser";
    snap.type = AccessoryType::BinaryActuator;
    snap.initialized = false;
    snap.supported = false;
    snap.state = false;

    char buf[256] = {};
    serializeAccessorySnapshots(&snap, 1, buf, sizeof(buf));

    TEST_ASSERT_NOT_NULL(strstr(buf, "\"supported\":false"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"initialized\":false"));
}

int main(int argc, char** argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_laser_snapshot_is_binary_actuator_type);
    RUN_TEST(test_laser_snapshot_state_off_serializes_false);
    RUN_TEST(test_laser_snapshot_state_on_serializes_true);
    RUN_TEST(test_laser_snapshot_unsupported_board_shows_supported_false);
    return UNITY_END();
}
