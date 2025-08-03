#ifndef STAR_DATABASE_H
#define STAR_DATABASE_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Forward declarations
struct StarEntry;
struct NGCEntry;

// Database backend types
enum DatabaseType
{
    DB_NONE = 0,
    DB_BSC5,    // Yale Bright Star Catalog
    DB_NGC2000, // New General Catalogue
    DB_BOTH     // Search both databases
};

// Unified object entry for search results
struct UnifiedEntry
{
    // Common fields
    String name;
    String type_str;
    double ra_deg;  // Right Ascension in degrees
    double dec_deg; // Declination in degrees
    float magnitude;
    String constellation;
    String description;

    // Source information
    DatabaseType source_db;

    // Extended data (may be empty depending on source)
    String spectral_type; // For stars (BSC5)
    float size_arcmin;    // For deep-sky objects (NGC)
    String notes;         // Additional information

    // Utility methods
    void print() const;
    String getCoordinateString() const;
    bool isVisible(double observer_lat_deg, double observer_lon_deg) const;
};

// Abstract base class for star/object databases

class StarDatabase
{
  protected:
    DatabaseType _db_type;
    StarDatabase* _backend;

  public:
    StarDatabase(DatabaseType db_type);
    virtual ~StarDatabase();

    // Database management
    virtual bool loadDatabase(const char* json_data);
    // New overload for pointer+length
    virtual bool loadDatabase(const char* json_data, size_t len);
    DatabaseType getDatabaseType() const
    {
        return _db_type;
    }
    virtual bool isLoaded() const;

    // Unified search interface
    virtual bool findByName(const String& name, UnifiedEntry& result) const;
    virtual bool findByNameFragment(const String& name_fragment, UnifiedEntry& result) const;
    virtual bool findByIndex(size_t index, UnifiedEntry& result) const;

    // Information methods
    virtual size_t getTotalObjectCount() const;
    virtual void printDatabaseInfo() const;

    // Utility methods
    virtual String formatCoordinates(double ra_deg, double dec_deg) const;
};

// Specialized database classes will be defined in separate headers

// Helper functions
String databaseTypeToString(DatabaseType type);
DatabaseType stringToDatabaseType(const String& type_str);
bool isStarName(const String& name);
bool isNGCName(const String& name);
bool isMessierName(const String& name);
bool isStarName(const String& name);
bool isNGCName(const String& name);
bool isMessierName(const String& name);

#endif // STAR_DATABASE_H
