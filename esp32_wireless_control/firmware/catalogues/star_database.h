/**
 * @file star_database.h
 * @version 0.1.0
 *
 * @section License
 * Copyright (C) 2025, Sylensky
 */

#ifndef STAR_DATABASE_H
#define STAR_DATABASE_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include "star_database_interface.h"

// Forward declarations
struct NGCEntry;
struct BSC5Entry;

// Forward declare NGC2000
class NGC2000;
// Forward declare BSC5
class BSC5;

class StarDatabase
{
  protected:
    StarDatabaseType _db_type;
    StarDatabaseInterface* _backend;

  public:
    StarDatabase(StarDatabaseType db_type, const uint8_t* start, const uint8_t* end);
    virtual ~StarDatabase();

    // Database management
    virtual bool loadDatabase(const char* data, size_t len);
    virtual bool unloadDatabase();
    StarDatabaseType getDatabaseType() const
    {
        return _db_type;
    }
    virtual bool isLoaded() const;

    // Unified search interface
    virtual bool findByName(const String& name, StarUnifiedEntry& result) const;
    virtual bool findByNameFragment(const String& name_fragment, StarUnifiedEntry& result) const;
    virtual bool findByIndex(size_t index, StarUnifiedEntry& result) const;

    // Information methods
    virtual size_t getTotalObjectCount() const;
    virtual void printDatabaseInfo() const;
};

#endif // STAR_DATABASE_H
