#include <Arduino.h>
#include <EEPROM.h>

#include "configs/config.h"
#include "eeprom_manager.h"

#if DEBUG == 1
#include "uart.h"
#endif

bool EepromManager::initialized = false;
size_t EepromManager::eepromSize = 512;

/**
 * @brief Initialize EEPROM with specified size
 * @param size Size in bytes to allocate for EEPROM
 */
void EepromManager::begin(size_t size)
{
    eepromSize = size;
    EEPROM.begin(size);
    initialized = true;
#if DEBUG == 1
    print_out("EEPROM initialized with size: %d bytes", size);
#endif
}
