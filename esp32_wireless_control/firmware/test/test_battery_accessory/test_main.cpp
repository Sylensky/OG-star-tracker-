// TDD Gate test for Epic 0.5.3 - Battery Telemetry
// Tests the pure calculation helpers and snapshot serialization contract.
// Importing battery_accessory.h causes a compile failure until it is created.
// Run with: pio test -e native_test

#include <stdint.h>
#include <string.h>
#include <unity.h>

#include "functions/accessories/accessory.h"
#include "functions/accessories/battery_accessory.h"

void setUp(void)
{
}
void tearDown(void)
{
}

// ---------------------------------------------------------------------------
// Pure calculation helpers
// ---------------------------------------------------------------------------

void test_battery_raw_adc_to_voltage_mv()
{
    // 100k/100k divider: V_batt = 2 × V_adc, V_adc_mv = rawAdc × 3300 / 4095
    // rawAdc=1861 → 1861 × 6600 / 4095 = 12282600 / 4095 = 2999 mV
    TEST_ASSERT_EQUAL_UINT32(2999u, BatteryAccessory::rawAdcToVoltageMv(1861));
}

void test_battery_raw_adc_zero_gives_zero_mv()
{
    TEST_ASSERT_EQUAL_UINT32(0u, BatteryAccessory::rawAdcToVoltageMv(0));
}

void test_battery_voltage_mv_to_percent_midpoint()
{
    // 3600 mV → (3600-3000)*100/1200 = 50 %
    TEST_ASSERT_EQUAL_UINT8(50u, BatteryAccessory::voltageMvToPercent(3600u));
}

void test_battery_voltage_mv_to_percent_clamped_low()
{
    TEST_ASSERT_EQUAL_UINT8(0u, BatteryAccessory::voltageMvToPercent(2000u));
    TEST_ASSERT_EQUAL_UINT8(0u, BatteryAccessory::voltageMvToPercent(3000u));
}

void test_battery_voltage_mv_to_percent_clamped_high()
{
    TEST_ASSERT_EQUAL_UINT8(100u, BatteryAccessory::voltageMvToPercent(4200u));
    TEST_ASSERT_EQUAL_UINT8(100u, BatteryAccessory::voltageMvToPercent(5000u));
}

// ---------------------------------------------------------------------------
// Snapshot serialization contract (SampledSensor type)
// ---------------------------------------------------------------------------

void test_battery_snapshot_serializes_all_required_fields()
{
    AccessorySnapshot snap{};
    snap.name = "battery";
    snap.type = AccessoryType::SampledSensor;
    snap.initialized = true;
    snap.supported = true;
    snap.rawAdc = 1861;
    snap.primaryKey = "voltage_mv";
    snap.primaryValue = BatteryAccessory::rawAdcToVoltageMv(1861);
    snap.secondaryKey = "percent";
    snap.secondaryValue = BatteryAccessory::voltageMvToPercent(snap.primaryValue);

    char buf[256] = {};
    serializeAccessorySnapshots(&snap, 1, buf, sizeof(buf));

    TEST_ASSERT_NOT_NULL(strstr(buf, "\"battery\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"rawAdc\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"voltage_mv\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"percent\""));
    // SampledSensor must NOT carry actuator-only field
    TEST_ASSERT_NULL(strstr(buf, "\"state\""));
}

void test_battery_snapshot_unsupported_board()
{
    AccessorySnapshot snap{};
    snap.name = "battery";
    snap.type = AccessoryType::SampledSensor;
    snap.initialized = false;
    snap.supported = false;
    snap.rawAdc = 0;
    snap.primaryKey = "voltage_mv";
    snap.primaryValue = 0;
    snap.secondaryKey = "percent";
    snap.secondaryValue = 0;

    char buf[256] = {};
    serializeAccessorySnapshots(&snap, 1, buf, sizeof(buf));

    TEST_ASSERT_NOT_NULL(strstr(buf, "\"supported\":false"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"initialized\":false"));
}

int main(int argc, char** argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_battery_raw_adc_to_voltage_mv);
    RUN_TEST(test_battery_raw_adc_zero_gives_zero_mv);
    RUN_TEST(test_battery_voltage_mv_to_percent_midpoint);
    RUN_TEST(test_battery_voltage_mv_to_percent_clamped_low);
    RUN_TEST(test_battery_voltage_mv_to_percent_clamped_high);
    RUN_TEST(test_battery_snapshot_serializes_all_required_fields);
    RUN_TEST(test_battery_snapshot_unsupported_board);
    return UNITY_END();
}
