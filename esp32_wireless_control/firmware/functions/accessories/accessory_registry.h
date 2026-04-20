/**
 * @file accessory_registry.h
 * @brief Central registry for all accessories.
 *
 * Provides one place to initialize, refresh, and snapshot every registered
 * accessory. Concrete accessories (laser, battery, light) are registered by
 * their respective feature modules in later epics (0.5.2-0.5.4).
 *
 * Usage:
 *   // At startup (after board config):
 *   AccessoryRegistry::getInstance().initAll();
 *
 *   // In main loop (rate-limited internally):
 *   AccessoryRegistry::getInstance().refreshSensors();
 *
 *   // In REST handler / UART command:
 *   char buf[512];
 *   AccessoryRegistry::getInstance().buildSnapshotJson(buf, sizeof(buf));
 */

#ifndef ACCESSORY_REGISTRY_H
#define ACCESSORY_REGISTRY_H

#include "functions/accessories/accessory.h"

#include <stddef.h>
#include <stdint.h>

class AccessoryRegistry
{
  public:
    /** Maximum number of accessories the registry can hold. */
    static constexpr uint8_t MAX_ACCESSORIES = 8;

    /**
     * @brief Minimum milliseconds between sensor refresh passes.
     * Prevents excessive ADC reads when refreshSensors() is called
     * from the main loop at high cadence.
     */
    static constexpr uint32_t REFRESH_INTERVAL_MS = 2000;

    /**
     * @brief Size of the snapshot JSON output buffer.
     * Sized for MAX_ACCESSORIES entries at ~60 bytes each with headroom.
     */
    static constexpr size_t SNAPSHOT_BUFFER_SIZE = 512;

    static AccessoryRegistry& getInstance();

    /**
     * @brief Register an accessory.
     * Must be called before initAll(). Does nothing if registry is full
     * or accessory pointer is null.
     */
    void registerAccessory(Accessory* accessory);

    /**
     * @brief Initialize all registered accessories against active board config.
     * Logs per-accessory init outcome to UART.
     * Safe to call with zero accessories registered.
     */
    void initAll();

    /**
     * @brief Refresh all initialized sampled sensors.
     * Rate-limited to REFRESH_INTERVAL_MS; safe to call every loop iteration.
     */
    void refreshSensors();

    /**
     * @brief Serialize all accessory snapshots into buf as a JSON object.
     * Delegates to serializeAccessorySnapshots(); produces "{}" when empty.
     * @return Bytes written, excluding null terminator.
     */
    size_t buildSnapshotJson(char* buf, size_t bufSize) const;

    /**
     * @brief Print accessory snapshot to UART console.
     */
    void printSnapshot() const;

    uint8_t getCount() const
    {
        return _count;
    }

  private:
    AccessoryRegistry();
    AccessoryRegistry(const AccessoryRegistry&) = delete;
    AccessoryRegistry& operator=(const AccessoryRegistry&) = delete;

    Accessory* _accessories[MAX_ACCESSORIES];
    uint8_t _count;
    uint32_t _lastRefreshMs;
};

#endif // ACCESSORY_REGISTRY_H
