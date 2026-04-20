/**
 * @file accessory.h
 * @brief Accessory subsystem base types and snapshot serialization.
 *
 * Provides the minimal common contracts for all accessories:
 *   - AccessoryType   – discriminates actuator vs. sensor
 *   - AccessorySnapshot – plain-data snapshot consumed by REST and UART surfaces
 *   - serializeAccessorySnapshots() – pure free function, host-compilable for tests
 *   - Accessory  – abstract base class
 *   - BinaryActuator – base for GPIO-driven on/off devices (e.g. laser)
 *   - SampledSensor  – base for ADC-polled analog sensors (e.g. battery, LDR)
 *
 * Design constraints:
 *   - No Arduino headers in this file; pure C++ for native testability.
 *   - Snapshot fields use const char* pointing to compile-time string literals only.
 *   - Zero-init an AccessorySnapshot with `AccessorySnapshot snap{}` before use.
 */

#ifndef ACCESSORY_H
#define ACCESSORY_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Discriminates actuator vs. sampled sensor.
 */
enum class AccessoryType : uint8_t
{
    BinaryActuator, ///< GPIO-controlled on/off device
    SampledSensor   ///< ADC-polled analog sensor
};

/**
 * @brief Plain-data snapshot of a single accessory's current state.
 *
 * Consumed by serializeAccessorySnapshots() to produce JSON, and by the
 * UART printer. Concrete accessory classes populate and return this from
 * their getSnapshot() override.
 *
 * Field conventions:
 *   name        – compile-time literal key for the JSON object, e.g. "laser"
 *   primaryKey  – compile-time literal for the first sensor value key, e.g. "voltage_mv"
 *   secondaryKey – compile-time literal for the second sensor value key, or nullptr
 */
struct AccessorySnapshot
{
    const char* name;   ///< Accessory identifier (JSON key)
    AccessoryType type; ///< Determines which fields are serialized
    bool initialized;   ///< true if init() succeeded
    bool supported;     ///< true if board config supplies a valid pin

    // --- BinaryActuator fields ---
    bool state; ///< Current on/off state

    // --- SampledSensor fields ---
    uint16_t rawAdc;          ///< Latest averaged ADC reading
    const char* primaryKey;   ///< JSON key for primaryValue (e.g. "voltage_mv", "percent")
    uint32_t primaryValue;    ///< Converted primary value
    const char* secondaryKey; ///< JSON key for secondaryValue, or nullptr if absent
    uint32_t secondaryValue;  ///< Converted secondary value (e.g. charge percent)
};

/**
 * @brief Serialize an array of AccessorySnapshot values into a JSON object string.
 *
 * Pure function – no side effects, no Arduino dependencies.
 * Serves both the REST handler and the UART printer so both surfaces
 * produce semantically identical output.
 *
 * Output format:
 * @code
 * {
 *   "laser":   { "supported": true,  "initialized": true,  "state": false },
 *   "battery": { "supported": true,  "initialized": true,  "rawAdc": 1234,
 *                "voltage_mv": 3700, "percent": 58 },
 *   "light":   { "supported": true,  "initialized": true,  "rawAdc": 2048,
 *                "percent": 25 }
 * }
 * @endcode
 *
 * @param snapshots  Array of snapshots (may be nullptr when count is 0)
 * @param count      Number of valid entries in snapshots
 * @param buf        Output buffer (null-terminated on return)
 * @param bufSize    Capacity of buf including null terminator
 * @return           Bytes written, excluding null terminator (0 on error)
 */
size_t serializeAccessorySnapshots(const AccessorySnapshot* snapshots, uint8_t count, char* buf,
                                   size_t bufSize);

// ---------------------------------------------------------------------------
// Abstract base and specialisations
// ---------------------------------------------------------------------------

/**
 * @brief Abstract base class for all accessories.
 *
 * Implementors must:
 *   - Return a stable compile-time string from getName().
 *   - Check isSupported() before calling hardware in init().
 *   - Never allocate on the heap inside getSnapshot().
 */
class Accessory
{
  public:
    virtual ~Accessory() = default;

    virtual const char* getName() const = 0;
    virtual AccessoryType getType() const = 0;
    virtual bool isSupported() const = 0;
    virtual bool init() = 0;
    virtual bool isInitialized() const = 0;
    virtual AccessorySnapshot getSnapshot() const = 0;

  protected:
    Accessory() = default;
};

/**
 * @brief Base for GPIO-driven binary output accessories (e.g. laser).
 *
 * Concrete implementations override setState() / getState() and
 * supply board-config-sourced pin via isSupported().
 */
class BinaryActuator : public Accessory
{
  public:
    AccessoryType getType() const override
    {
        return AccessoryType::BinaryActuator;
    }

    virtual void setState(bool on) = 0;
    virtual bool getState() const = 0;
};

/**
 * @brief Base for ADC-polled analog sensors (e.g. battery, ambient light).
 *
 * Concrete implementations override refresh() to read and cache the ADC,
 * and getRawAdc() to expose the last averaged sample.
 * The registry calls refresh() on a bounded cadence via refreshSensors().
 */
class SampledSensor : public Accessory
{
  public:
    AccessoryType getType() const override
    {
        return AccessoryType::SampledSensor;
    }

    /**
     * @brief Take a fresh ADC reading and update internal state.
     * Must not block for long; should average a small number of samples.
     */
    virtual void refresh() = 0;
    virtual uint16_t getRawAdc() const = 0;
};

#endif // ACCESSORY_H
