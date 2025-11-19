#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

#if DEBUG == 1
// Forward declaration for debug printing
void print_out(const char* format, ...);
#endif

/**
 * @brief EEPROM Manager class for handling persistent storage
 *
 * This class provides a centralized interface for reading and writing
 * data to EEPROM memory with proper error handling and debugging.
 */
class EepromManager
{
  public:
    enum PresetType
    {
        INTERVALOMETER_PRESETS,
        TRACKING_RATE_PRESETS
    };

    static void begin(size_t size = 512);

    // Basic EEPROM operations
    template <class T> static int writeObject(int address, const T& object);
    template <class T> static int readObject(int address, T& object);

    // Preset management methods using templates to avoid circular dependency
    template <class T> static int writePresets(int address, const T& presets);
    template <class T> static int readPresets(int address, T& presets);

  private:
    static bool initialized;
    static size_t eepromSize;
};

// Template implementations must be in header file
template <class T> inline int EepromManager::writeObject(int address, const T& object)
{
    const byte* p = (const byte*) (const void*) &object;
    unsigned int i = 0;
    for (i = 0; i < sizeof(object); i++)
    {
#if DEBUG == 1
        print_out("EEPROM Write - Address: %d, Data: 0x%02X", address, *p);
#endif
        EEPROM.write(address++, *p++);
    }
    EEPROM.commit();

#if DEBUG == 1
    print_out("EEPROM Write completed - %d bytes written", i);
#endif

    return i;
}

template <class T> inline int EepromManager::readObject(int address, T& object)
{
    byte* p = (byte*) (void*) &object;
    unsigned int i = 0;
    for (i = 0; i < sizeof(object); i++)
    {
        *p++ = EEPROM.read(address++);
#if DEBUG == 1
        print_out("EEPROM Read - Address: %d, Data: 0x%02X", address, *(p - 1));
#endif
    }

#if DEBUG == 1
    print_out("EEPROM Read completed - %d bytes read", i);
#endif

    return i;
}

template <class T> inline int EepromManager::writePresets(int address, const T& presets)
{
    const byte* p = (const byte*) (const void*) &presets;
    unsigned int i = 0;
    for (i = 0; i < sizeof(presets); i++)
    {
#if DEBUG == 1
        print_out("EEPROM Write - Address: %d, Data: 0x%02X", address, *p);
#endif
        EEPROM.write(address++, *p++);
    }
    EEPROM.commit();

#if DEBUG == 1
    print_out("EEPROM Write completed - %d bytes written", i);
#endif

    return i;
}

template <class T> inline int EepromManager::readPresets(int address, T& presets)
{
    byte* p = (byte*) (void*) &presets;
    unsigned int i = 0;
    for (i = 0; i < sizeof(presets); i++)
    {
        byte value = EEPROM.read(address++);
        *p++ = value;
#if DEBUG == 1
        print_out("EEPROM Read - Address: %d, Data: 0x%02X", address - 1, value);
#endif
    }

#if DEBUG == 1
    print_out("EEPROM Read completed - %d bytes read", i);
#endif

    return i;
}

#endif // EEPROM_MANAGER_H
