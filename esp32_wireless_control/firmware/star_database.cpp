#include "star_database.h"
#include "uart.h"

#include <Arduino.h>
#include <cmath>

// UnifiedEntry methods
void UnifiedEntry::print() const
{
    print_out("=== Object Information ===");
    print_out("Name: %s", name.c_str());
    print_out("Type: %s", type_str.c_str());
    print_out("Right Ascension: %.6f degrees", ra_deg);
    print_out("Declination: %.6f degrees", dec_deg);
    print_out("Source Database: %s", databaseTypeToString(source_db).c_str());

    if (constellation.length() > 0)
    {
        print_out("Constellation: %s", constellation.c_str());
    }
    if (magnitude > 0)
    {
        print_out("Magnitude: %.2f", magnitude);
    }
    if (spectral_type.length() > 0)
    {
        print_out("Spectral Type: %s", spectral_type.c_str());
    }
    if (size_arcmin > 0)
    {
        print_out("Size: %.1f arcmin", size_arcmin);
    }
    if (description.length() > 0)
    {
        print_out("Description: %s", description.c_str());
    }
    if (notes.length() > 0)
    {
        print_out("Notes: %s", notes.c_str());
    }
    print_out("==========================");
}

String UnifiedEntry::getCoordinateString() const
{
    int ra_h = (int) (ra_deg / 15.0);
    int ra_m = (int) ((ra_deg / 15.0 - ra_h) * 60.0);
    double ra_s = ((ra_deg / 15.0 - ra_h) * 60.0 - ra_m) * 60.0;

    int dec_d = (int) abs(dec_deg);
    int dec_m = (int) ((abs(dec_deg) - dec_d) * 60.0);
    double dec_s = ((abs(dec_deg) - dec_d) * 60.0 - dec_m) * 60.0;

    char coord_str[50];
    snprintf(coord_str, sizeof(coord_str), "%02dh%02dm%05.2fs %c%02dd%02dm%05.2fs", ra_h, ra_m,
             ra_s, (dec_deg >= 0) ? '+' : '-', dec_d, dec_m, dec_s);

    return String(coord_str);
}

bool UnifiedEntry::isVisible(double observer_lat_deg, double observer_lon_deg) const
{
    // Simple visibility check based on declination and observer latitude
    // Object is always above horizon if dec > (90 - lat)
    // Object is never above horizon if dec < -(90 + lat)
    double min_dec = -(90.0 + observer_lat_deg);
    double max_dec = 90.0 - observer_lat_deg;

    return (dec_deg > min_dec && dec_deg < max_dec);
}

#include "bsc5/bsc5ra.h"
#include "ngc/ngc2000.h"

// StarDatabase base class implementation
StarDatabase::StarDatabase(DatabaseType db_type) : _db_type(db_type), _backend(nullptr)
{
    switch (db_type)
    {
        case DB_BSC5:
            _backend = new BSC5();
            break;
        case DB_NGC2000:
            _backend = new NGC2000();
            break;
        default:
            _backend = nullptr;
            break;
    }
}

StarDatabase::~StarDatabase()
{
    if (_backend)
        delete _backend;
}

bool StarDatabase::loadDatabase(const char* json_data)
{
    if (_backend)
        return _backend->loadDatabase(json_data);
    return false;
}

// New overload for pointer+length
bool StarDatabase::loadDatabase(const char* json_data, size_t len)
{
    if (_backend)
        return _backend->loadDatabase(json_data, len);
    return false;
}

bool StarDatabase::isLoaded() const
{
    if (_backend)
        return _backend->isLoaded();
    return false;
}

bool StarDatabase::findByName(const String& name, UnifiedEntry& result) const
{
    if (_backend)
        return _backend->findByName(name, result);
    return false;
}

bool StarDatabase::findByNameFragment(const String& name_fragment, UnifiedEntry& result) const
{
    if (_backend)
        return _backend->findByNameFragment(name_fragment, result);
    return false;
}

bool StarDatabase::findByIndex(size_t index, UnifiedEntry& result) const
{
    if (_backend)
        return _backend->findByIndex(index, result);
    return false;
}

size_t StarDatabase::getTotalObjectCount() const
{
    if (_backend)
        return _backend->getTotalObjectCount();
    return 0;
}

void StarDatabase::printDatabaseInfo() const
{
    if (_backend)
        _backend->printDatabaseInfo();
}

String StarDatabase::formatCoordinates(double ra_deg, double dec_deg) const
{
    if (_backend)
        return _backend->formatCoordinates(ra_deg, dec_deg);
    return String("");
}

// Helper functions
String databaseTypeToString(DatabaseType type)
{
    switch (type)
    {
        case DB_BSC5:
            return "BSC5";
        case DB_NGC2000:
            return "NGC2000";
        case DB_BOTH:
            return "BSC5+NGC2000";
        default:
            return "None";
    }
}

DatabaseType stringToDatabaseType(const String& type_str)
{
    String type_upper = type_str;
    type_upper.toUpperCase();

    if (type_upper == "BSC5")
        return DB_BSC5;
    if (type_upper == "NGC2000" || type_upper == "NGC")
        return DB_NGC2000;
    if (type_upper == "BOTH" || type_upper == "ALL")
        return DB_BOTH;

    return DB_NONE;
}

bool isStarName(const String& name)
{
    String upper_name = name;
    upper_name.toUpperCase();
    return upper_name.startsWith("HR ") || upper_name.startsWith("HD ") ||
           upper_name.indexOf(" ") > 0; // Greek letter + constellation
}

bool isNGCName(const String& name)
{
    String upper_name = name;
    upper_name.toUpperCase();
    return upper_name.startsWith("NGC") || upper_name.startsWith("IC");
}

bool isMessierName(const String& name)
{
    String upper_name = name;
    upper_name.toUpperCase();
    return upper_name.startsWith("M") && name.length() <= 4;
}
