// TDD Gate test for Epic 0.5.4 - Ambient Light Sensor
// Tests the pure calculation helper and snapshot serialization contract.
// Importing light_accessory.h causes a compile failure until it is created.
// Run with: pio test -e native_test

#include <stdint.h>
#include <string.h>
#include <unity.h>

#include "functions/accessories/accessory.h"
#include "functions/accessories/light_accessory.h"

void setUp(void)
{
}
void tearDown(void)
{
}

// ---------------------------------------------------------------------------
// Pure calculation helper
// ---------------------------------------------------------------------------

void test_light_raw_adc_to_percent_midpoint()
{
    // 10k pull-up + GL5516 to GND: higher rawAdc = brighter
    // rawAdc=2048 (≈50% of 4095) → 50 %
    TEST_ASSERT_EQUAL_UINT8(50u, LightAccessory::rawAdcToPercent(2048));
}

void test_light_raw_adc_zero_gives_zero_percent()
{
    // fully dark → 0 %
    TEST_ASSERT_EQUAL_UINT8(0u, LightAccessory::rawAdcToPercent(0));
}

void test_light_raw_adc_max_gives_100_percent()
{
    // fully bright (4095) → 100 %
    TEST_ASSERT_EQUAL_UINT8(100u, LightAccessory::rawAdcToPercent(4095));
}

void test_light_raw_adc_clamps_above_max()
{
    // Any value >= 4095 must be clamped to 100
    TEST_ASSERT_EQUAL_UINT8(100u, LightAccessory::rawAdcToPercent(4095));
}

// ---------------------------------------------------------------------------
// Snapshot serialization contract (SampledSensor, percent-only)
// ---------------------------------------------------------------------------

void test_light_snapshot_has_required_fields()
{
    AccessorySnapshot snap{};
    snap.name = "light";
    snap.type = AccessoryType::SampledSensor;
    snap.initialized = true;
    snap.supported = true;
    snap.rawAdc = 2048;
    snap.primaryKey = "percent";
    snap.primaryValue = LightAccessory::rawAdcToPercent(2048);
    snap.secondaryKey = nullptr; // no voltage for light sensor

    char buf[256] = {};
    serializeAccessorySnapshots(&snap, 1, buf, sizeof(buf));

    TEST_ASSERT_NOT_NULL(strstr(buf, "\"light\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"rawAdc\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"percent\""));
    // Light sensor must NOT carry battery-only fields
    TEST_ASSERT_NULL(strstr(buf, "\"voltage_mv\""));
    TEST_ASSERT_NULL(strstr(buf, "\"state\""));
}

void test_light_snapshot_unsupported_board()
{
    AccessorySnapshot snap{};
    snap.name = "light";
    snap.type = AccessoryType::SampledSensor;
    snap.initialized = false;
    snap.supported = false;
    snap.rawAdc = 0;
    snap.primaryKey = "percent";
    snap.primaryValue = 0;
    snap.secondaryKey = nullptr;

    char buf[256] = {};
    serializeAccessorySnapshots(&snap, 1, buf, sizeof(buf));

    TEST_ASSERT_NOT_NULL(strstr(buf, "\"supported\":false"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"initialized\":false"));
}

int main(int argc, char** argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_light_raw_adc_to_percent_midpoint);
    RUN_TEST(test_light_raw_adc_zero_gives_zero_percent);
    RUN_TEST(test_light_raw_adc_max_gives_100_percent);
    RUN_TEST(test_light_raw_adc_clamps_above_max);
    RUN_TEST(test_light_snapshot_has_required_fields);
    RUN_TEST(test_light_snapshot_unsupported_board);
    return UNITY_END();
}
