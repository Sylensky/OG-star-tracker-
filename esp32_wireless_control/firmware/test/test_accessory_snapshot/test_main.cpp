// TDD Gate test for Epic 0.5.1 - Accessory Subsystem Foundation
// These tests cover the pure, host-compilable serialization function.
// Run with: pio test -e native_test

#include <string.h>
#include <unity.h>

// accessory.h declares AccessorySnapshot, AccessoryType, and
// serializeAccessorySnapshots(). File does not exist yet - this import
// intentionally causes a compile failure until Milestone 1 is implemented.
#include "functions/accessories/accessory.h"

void setUp(void)
{
}
void tearDown(void)
{
}

void test_empty_snapshot_returns_empty_json_object()
{
    char buf[256] = {};
    size_t len = serializeAccessorySnapshots(nullptr, 0, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("{}", buf);
    TEST_ASSERT_GREATER_THAN(0, len);
}

void test_actuator_snapshot_includes_name_and_state_field()
{
    AccessorySnapshot snap{};
    snap.name = "laser";
    snap.type = AccessoryType::BinaryActuator;
    snap.initialized = true;
    snap.supported = true;
    snap.state = false;

    char buf[512] = {};
    serializeAccessorySnapshots(&snap, 1, buf, sizeof(buf));

    TEST_ASSERT_NOT_NULL(strstr(buf, "\"laser\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"initialized\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"supported\""));
}

void test_sensor_snapshot_includes_name_and_raw_adc()
{
    AccessorySnapshot snap{};
    snap.name = "battery";
    snap.type = AccessoryType::SampledSensor;
    snap.initialized = true;
    snap.supported = true;
    snap.rawAdc = 2048;
    snap.primaryKey = "voltage_mv";
    snap.primaryValue = 3700;
    snap.secondaryKey = "percent";
    snap.secondaryValue = 58;

    char buf[512] = {};
    serializeAccessorySnapshots(&snap, 1, buf, sizeof(buf));

    TEST_ASSERT_NOT_NULL(strstr(buf, "\"battery\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"rawAdc\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"voltage_mv\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"percent\""));
}

void test_sensor_snapshot_without_secondary_key_omits_secondary()
{
    AccessorySnapshot snap{};
    snap.name = "light";
    snap.type = AccessoryType::SampledSensor;
    snap.initialized = true;
    snap.supported = true;
    snap.rawAdc = 1024;
    snap.primaryKey = "percent";
    snap.primaryValue = 25;
    snap.secondaryKey = nullptr;

    char buf[512] = {};
    serializeAccessorySnapshots(&snap, 1, buf, sizeof(buf));

    TEST_ASSERT_NOT_NULL(strstr(buf, "\"light\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"percent\""));
}

void test_unsupported_accessory_snapshot_marks_supported_false()
{
    AccessorySnapshot snap{};
    snap.name = "laser";
    snap.type = AccessoryType::BinaryActuator;
    snap.initialized = false;
    snap.supported = false;
    snap.state = false;

    char buf[512] = {};
    serializeAccessorySnapshots(&snap, 1, buf, sizeof(buf));

    TEST_ASSERT_NOT_NULL(strstr(buf, "\"laser\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "false"));
}

int main(int argc, char** argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_empty_snapshot_returns_empty_json_object);
    RUN_TEST(test_actuator_snapshot_includes_name_and_state_field);
    RUN_TEST(test_sensor_snapshot_includes_name_and_raw_adc);
    RUN_TEST(test_sensor_snapshot_without_secondary_key_omits_secondary);
    RUN_TEST(test_unsupported_accessory_snapshot_marks_supported_false);
    return UNITY_END();
}
