/**
 * @file accessory_registry.cpp
 * @brief Implementation of AccessoryRegistry.
 */

#include "functions/accessories/accessory_registry.h"
#include "uart.h"

#include <Arduino.h> // millis()

AccessoryRegistry::AccessoryRegistry() : _count(0), _lastRefreshMs(0)
{
    for (uint8_t i = 0; i < MAX_ACCESSORIES; i++)
        _accessories[i] = nullptr;
}

AccessoryRegistry& AccessoryRegistry::getInstance()
{
    static AccessoryRegistry instance;
    return instance;
}

void AccessoryRegistry::registerAccessory(Accessory* accessory)
{
    if (accessory == nullptr)
        return;
    if (_count >= MAX_ACCESSORIES)
    {
        print_out("[Accessory] Registry full – cannot register %s", accessory->getName());
        return;
    }
    _accessories[_count++] = accessory;
}

void AccessoryRegistry::initAll()
{
    if (_count == 0)
    {
        print_out("[Accessory] Registry: no accessories registered");
        return;
    }

    for (uint8_t i = 0; i < _count; i++)
    {
        Accessory* acc = _accessories[i];

        if (!acc->isSupported())
        {
            print_out("[Accessory] %s: not supported on this board – skipping init",
                      acc->getName());
            continue;
        }

        bool ok = acc->init();
        print_out("[Accessory] %s: %s", acc->getName(), ok ? "initialized OK" : "init FAILED");
    }
}

void AccessoryRegistry::refreshSensors()
{
    uint32_t now = millis();
    if (now - _lastRefreshMs < REFRESH_INTERVAL_MS)
        return;
    _lastRefreshMs = now;

    for (uint8_t i = 0; i < _count; i++)
    {
        Accessory* acc = _accessories[i];
        if (acc->getType() == AccessoryType::SampledSensor && acc->isInitialized())
        {
            static_cast<SampledSensor*>(acc)->refresh();
        }
    }
}

size_t AccessoryRegistry::buildSnapshotJson(char* buf, size_t bufSize) const
{
    AccessorySnapshot snapshots[MAX_ACCESSORIES];
    for (uint8_t i = 0; i < _count; i++)
        snapshots[i] = _accessories[i]->getSnapshot();
    return serializeAccessorySnapshots(snapshots, _count, buf, bufSize);
}

void AccessoryRegistry::printSnapshot() const
{
    char buf[SNAPSHOT_BUFFER_SIZE];
    buildSnapshotJson(buf, sizeof(buf));
    print_out("Accessories: %s", buf);
}
