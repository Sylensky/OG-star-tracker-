/**
 * @file accessory_snapshot.cpp
 * @brief Pure serialization function for AccessorySnapshot arrays.
 *
 * This translation unit has no Arduino dependencies: only ArduinoJson and the
 * accessory type header. It is included in the native test build so that
 * serializeAccessorySnapshots() can be exercised without embedded hardware.
 */

#include "functions/accessories/accessory.h"

#include <ArduinoJson.h>

size_t serializeAccessorySnapshots(const AccessorySnapshot* snapshots, uint8_t count, char* buf,
                                   size_t bufSize)
{
    if (buf == nullptr || bufSize == 0)
        return 0;

    ArduinoJson::JsonDocument doc;
    doc.to<ArduinoJson::JsonObject>(); // ensure object type even when count == 0

    for (uint8_t i = 0; i < count; i++)
    {
        const AccessorySnapshot& s = snapshots[i];
        if (s.name == nullptr)
            continue;

        ArduinoJson::JsonObject obj = doc[s.name].to<ArduinoJson::JsonObject>();
        obj["supported"] = s.supported;
        obj["initialized"] = s.initialized;

        if (s.type == AccessoryType::BinaryActuator)
        {
            obj["state"] = s.state;
        }
        else // SampledSensor
        {
            obj["rawAdc"] = s.rawAdc;
            if (s.primaryKey != nullptr)
                obj[s.primaryKey] = s.primaryValue;
            if (s.secondaryKey != nullptr)
                obj[s.secondaryKey] = s.secondaryValue;
        }
    }

    return serializeJson(doc, buf, bufSize);
}
