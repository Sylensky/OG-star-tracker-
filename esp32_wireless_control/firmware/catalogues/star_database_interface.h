/**
 * @file star_database_interface.h
 * @version 0.1.0
 *
 * @section License
 * Copyright (C) 2025, Sylensky
 */

#ifndef STAR_DATABASE_INTERFACE_H
#define STAR_DATABASE_INTERFACE_H

#include <WString.h>

// Database types
enum StarDatabaseType
{
    DB_NONE = 0,
    DB_NGC2000,         // New General Catalogue
    DB_NGC2000_COMPACT, // Compact NGC2000 format
    DB_BSC5,            // Bright Star Catalog 5th edition
    DB_BSC5_COMPACT,    // Compact BSC5 format
};

// Unified object entry for search results
struct StarUnifiedEntry
{
    String name;
    String type_str;
    double ra_hours;
    double dec_deg;
    float magnitude;
    String constellation;
    String description;

    // Source information
    StarDatabaseType source_db;

    // Extended data (may be empty depending on source)
    String spectral_type;
    float size_arcmin;
    String notes;

    // Utility methods
    void print() const;
};

class StarDatabaseInterface
{
  public:
    virtual ~StarDatabaseInterface()
    {
    }
    virtual bool loadDatabase(const char* data, size_t len) = 0;
    virtual bool unloadDatabase() = 0;
    virtual bool isLoaded() const = 0;
    virtual bool findByName(const String& name, StarUnifiedEntry& result) const = 0;
    virtual bool findByNameFragment(const String& name_fragment,
                                    StarUnifiedEntry& result) const = 0;
    virtual bool findByIndex(size_t index, StarUnifiedEntry& result) const = 0;
    virtual size_t getTotalObjectCount() const = 0;
    virtual void printDatabaseInfo() const = 0;

    // Pagination support for memory optimization
    virtual bool loadPage(size_t page_index, size_t page_size = 32) = 0;
    virtual bool isPageLoaded(size_t page_index) const = 0;
    virtual size_t getCurrentPageCount() const = 0;
    virtual size_t getPageSize() const = 0;
    virtual void clearLoadedPages() = 0;
};

#endif // STAR_DATABASE_INTERFACE_H
