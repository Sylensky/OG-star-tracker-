#include "tmc_motor_driver.h"
#include "uart.h"

TmcMotorDriver::TmcMotorDriver(HardwareSerial* serialPort, uint8_t addr, float rSense,
                               uint8_t rxPin, uint8_t txPin)
    : serialPort(serialPort), addr(addr), tmc_driver(serialPort, rSense, addr)
{
    if (serialPort != nullptr)
    {
        serialPort->begin(115200, SERIAL_8N1, rxPin, txPin);
        while (!serialPort)
            ;
    }

    tmc_driver.begin();
    tmc_driver.pdn_disable(true);      // enable pdn_uart
    tmc_driver.mstep_reg_select(true); // Use MRES register for microstepping
    tmc_driver.VACTUAL(0);             // Enable stepping by step pin

    // Basic initialization only - advanced settings moved to setMicrosteps()
    tmc_driver.rms_current(1000, 0.6); // Higher initial current: 1000mA run, 600mA hold
    tmc_driver.push();                 // Push basic settings to driver
}

void TmcMotorDriver::setMicrosteps(uint16_t microsteps)
{
    tmc_driver.microsteps(microsteps);

    // IRUN: 1400mA (87% of 1.6A max for NEMA17) for strong torque during movement
    // IHOLD: 1120mA (80% of IRUN) for reliable position holding under load
    tmc_driver.rms_current(1400, 0.8); // 1400mA run, 1120mA hold
    tmc_driver.iholddelay(12);         // Hold delay for load stability
    tmc_driver.TPOWERDOWN(25);         // Extended power down delay for heavy loads

    // Apply StealthChop settings
    tmc_driver.en_spreadCycle(false); // Use StealthChop for quiet operation
    tmc_driver.pwm_autoscale(true);   // Enable automatic PWM scaling

    // Push all settings at once
    tmc_driver.push();

    uint16_t actual_microsteps = tmc_driver.microsteps();
    uint16_t actual_rms_current = tmc_driver.rms_current();

#if DEBUG == 1
    print_out("Microsteps set:");
    print_out("  Requested: %d, Actual: %d", microsteps, actual_microsteps);
    print_out("  RMS Current: %d mA", actual_rms_current);
    print_out("  Hold Current: %d mA", (uint16_t) (actual_rms_current * 0.8));
#endif

    if (microsteps != actual_microsteps)
    {
        print_out("WARNING: Microstep setting mismatch!");
    }
}

void TmcMotorDriver::setDirection(bool direction)
{
    tmc_driver.shaft(direction);
    tmc_driver.push();
}

void TmcMotorDriver::print_status()
{
#if DEBUG == 1
    uint8_t ihold_raw = tmc_driver.ihold();
    uint16_t rms_current = tmc_driver.rms_current();

    // Hold current = RMS_current * (IHOLD/32) where IHOLD is 0-31
    uint16_t hold_current = (uint16_t) (rms_current * ihold_raw / 32.0);

    print_out("TMC Driver Status:");
    print_out("  RMS Current: %d mA", tmc_driver.rms_current());
    print_out("  Hold Current: %d mA (IHOLD=%d/32 of IRUN)", hold_current, ihold_raw);
    print_out("  Microsteps: %d", tmc_driver.microsteps());
    print_out("  irun: %d", tmc_driver.irun());
    print_out("  ihold: %d", tmc_driver.ihold());
    print_out("  iholddelay: %d", tmc_driver.iholddelay());
    print_out("  TPOWERDOWN: %d", tmc_driver.TPOWERDOWN());

    // StealthChop settings
    print_out("StealthChop Settings:");
    print_out("  SpreadCycle enabled: %s", tmc_driver.en_spreadCycle() ? "true" : "false");
    print_out("  PWM autoscale: %s", tmc_driver.pwm_autoscale() ? "true" : "false");
    print_out("  PWM autograd: %s", tmc_driver.pwm_autograd() ? "true" : "false");
    print_out("  TPWMTHRS: %d", tmc_driver.TPWMTHRS());

    // Read back GCONF register values
    print_out("GCONF Settings:");
    print_out("  pdn_disable: %s", tmc_driver.pdn_disable() ? "true" : "false");
    print_out("  mstep_reg_select: %s", tmc_driver.mstep_reg_select() ? "true" : "false");

    // Read driver status flags
    print_out("Driver Status:");
    print_out("  Driver enabled: %s", tmc_driver.isEnabled() ? "true" : "false");
    print_out("  Communication OK: %s", tmc_driver.test_connection() == 0 ? "true" : "false");

    // Read GSTAT error registers
    bool drv_error = tmc_driver.drv_err();
    bool uv_cp_error = tmc_driver.uv_cp();
    bool reset_flag = tmc_driver.reset();

    print_out("TMC Error Status (GSTAT):");
    print_out("  Driver Error (drv_err): %s", drv_error ? "ERROR" : "OK");
    print_out("  Undervoltage/Charge Pump (uv_cp): %s", uv_cp_error ? "ERROR" : "OK");
    print_out("  Reset Flag: %s", reset_flag ? "RESET OCCURRED" : "OK");

    // Clear GSTAT flags by writing back the read values
    // This clears the latched error flags
    if (drv_error || uv_cp_error || reset_flag)
    {
        print_out("*** TMC DRIVER HAS ERRORS - CLEARING GSTAT FLAGS ***");

        uint8_t gstat_clear = 0;
        if (reset_flag)
            gstat_clear |= 0x01; // Clear reset flag
        if (drv_error)
            gstat_clear |= 0x02; // Clear drv_err flag
        if (uv_cp_error)
            gstat_clear |= 0x04; // Clear uv_cp flag

        tmc_driver.GSTAT(gstat_clear);

        print_out("GSTAT error flags cleared (0x%02X)", gstat_clear);
        print_out("*** CHECK CONNECTIONS AND POWER - ERRORS WERE PRESENT ***");
    }
    else
    {
        print_out("TMC Driver: All GSTAT error flags clear");
    }
#endif
}
