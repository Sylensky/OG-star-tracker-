#include "ngc2000.h"
#include "uart.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <cmath>

// Global instance
NGC2000 ngc2000;

// NGCEntry methods
void NGCEntry::print() const
{
    print_out("=== NGC Object Information ===");
    print_out("Name: %s", name.c_str());
    print_out("Type: %s", getTypeString().c_str());
    print_out("Right Ascension: %.6f degrees", ra_deg);
    print_out("Declination: %.6f degrees", dec_deg);
    print_out("Constellation: %s", constellation.c_str());
    if (magnitude > 0)
    {
        print_out("Magnitude: %.2f", magnitude);
    }
    else
    {
        print_out("Magnitude: Unknown");
    }

    if (size_arcmin > 0)
    {
        print_out("Size: %.1f arcminutes", size_arcmin);
    }

    if (!notes.isEmpty())
    {
        print_out("Notes: %s", notes.c_str());
    }
    print_out("==============================");
}

String NGCEntry::getTypeString() const
{
    switch (type)
    {
        case NGC_GALAXY:
            return "Galaxy";
        case NGC_OPEN_CLUSTER:
            return "Open Cluster";
        case NGC_GLOBULAR_CLUSTER:
            return "Globular Cluster";
        case NGC_PLANETARY_NEBULA:
            return "Planetary Nebula";
        case NGC_NEBULA:
            return "Nebula";
        case NGC_SINGLE_STAR:
            return "Star";
        case NGC_DOUBLE_STAR:
            return "Double Star";
        case NGC_ASTERISM:
            return "Asterism";
        case NGC_UNKNOWN:
            return "Other";
        default:
            return "Unknown";
    }
}

// NGC2000 methods
NGC2000::NGC2000() : StarDatabase(DB_NGC2000), _doc(nullptr), _using_json(false), _object_count(0)
{
}

NGC2000::NGC2000(const uint8_t* start, const uint8_t* end)
    : StarDatabase(DB_NGC2000), _start(start), _end(end), _doc(nullptr), _using_json(false),
      _object_count(0)
{
}

NGC2000::~NGC2000()
{
    if (_doc)
    {
        delete _doc;
        _doc = nullptr;
    }
}

bool NGC2000::begin_json(const String& json_data)
{
    if (_doc)
    {
        delete _doc;
    }

    _doc = new JsonDocument(); // Dynamic allocation for NGC catalog
    _using_json = true;

    DeserializationError error = deserializeJson(*_doc, json_data);
    if (error)
    {
        print_out("NGC JSON parsing failed: %s", error.c_str());
        delete _doc;
        _doc = nullptr;
        return false;
    }

    // Check if we have the new metadata structure or legacy array format
    JsonVariant objects_var = (*_doc)["objects"];
    if (!objects_var.isNull())
    {
        // New format with metadata
        JsonVariant total_objects_var = (*_doc)["total_objects"];
        if (!total_objects_var.isNull())
        {
            _object_count = total_objects_var.as<size_t>();
        }

        if (!objects_var.is<JsonArray>())
        {
            print_out("No objects array found in NGC JSON");
            return false;
        }
        JsonArray objects_array = objects_var.as<JsonArray>();
        _object_count = objects_array.size();

        // Print catalog info if available
        JsonVariant catalog_var = (*_doc)["catalog"];
        if (!catalog_var.isNull())
        {
            print_out("Catalog: %s", catalog_var.as<String>().c_str());
        }

        JsonVariant version_var = (*_doc)["version"];
        if (!version_var.isNull())
        {
            print_out("Version: %s", version_var.as<String>().c_str());
        }

        JsonVariant coord_var = (*_doc)["coordinate_system"];
        if (!coord_var.isNull())
        {
            print_out("Coordinate System: %s", coord_var.as<String>().c_str());
        }
    }
    else if (_doc->is<JsonArray>())
    {
        // Legacy format - direct array
        _object_count = _doc->as<JsonArray>().size();
    }
    else
    {
        print_out("Invalid NGC JSON format - expected array or object with objects");
        return false;
    }

    print_out("Loaded %zu NGC objects from JSON", _object_count);
    return _object_count > 0;
}

bool NGC2000::findNGCByName(const String& name, NGCEntry& result) const
{
    if (!_using_json || !_doc)
    {
        return false;
    }

    JsonArray objects;

    // Handle both new metadata format and legacy array format
    JsonVariant objects_var = (*_doc)["objects"];
    if (!objects_var.isNull())
    {
        objects = objects_var.as<JsonArray>();
    }
    else if (_doc->is<JsonArray>())
    {
        objects = _doc->as<JsonArray>();
    }
    else
    {
        return false;
    }

    String search_name = name;
    search_name.toUpperCase();

    for (JsonObject obj : objects)
    {
        String obj_name = obj["name"].as<String>();
        obj_name.toUpperCase();

        if (obj_name == search_name)
        {
            return parseObjectFromJson(obj, result);
        }
    }

    return false;
}

bool NGC2000::findNGCByNameFragment(const String& name_fragment, NGCEntry& result) const
{
    if (!_using_json || !_doc)
    {
        return false;
    }

    JsonArray objects;

    // Handle both new metadata format and legacy array format
    JsonVariant objects_var = (*_doc)["objects"];
    if (!objects_var.isNull())
    {
        objects = objects_var.as<JsonArray>();
    }
    else if (_doc->is<JsonArray>())
    {
        objects = _doc->as<JsonArray>();
    }
    else
    {
        return false;
    }

    String search_term = name_fragment;
    search_term.toUpperCase();

    for (JsonObject obj : objects)
    {
        String obj_name = obj["name"].as<String>();
        obj_name.toUpperCase();

        if (obj_name.indexOf(search_term) >= 0)
        {
            return parseObjectFromJson(obj, result);
        }
    }

    return false;
}

bool NGC2000::findByRADec(double ra_deg, double dec_deg, double radius_deg, NGCEntry& result) const
{
    if (!_using_json || !_doc)
    {
        return false;
    }

    JsonArray objects;

    // Handle both new metadata format and legacy array format
    JsonVariant objects_var = (*_doc)["objects"];
    if (!objects_var.isNull())
    {
        objects = objects_var.as<JsonArray>();
    }
    else if (_doc->is<JsonArray>())
    {
        objects = _doc->as<JsonArray>();
    }
    else
    {
        return false;
    }

    double best_distance = radius_deg + 1.0; // Start beyond search radius
    JsonObject best_obj;
    bool found = false;

    for (JsonObject obj : objects)
    {
        double obj_ra = obj["ra_deg"].as<double>();
        double obj_dec = obj["dec_deg"].as<double>();

        double distance = angularDistance(ra_deg, dec_deg, obj_ra, obj_dec);

        if (distance <= radius_deg && distance < best_distance)
        {
            best_distance = distance;
            best_obj = obj;
            found = true;
        }
    }

    if (found)
    {
        return parseObjectFromJson(best_obj, result);
    }

    return false;
}

bool NGC2000::findNearestToRADec(double ra_deg, double dec_deg, NGCEntry& result) const
{
    if (!_using_json || !_doc)
    {
        return false;
    }

    JsonArray objects;

    // Handle both new metadata format and legacy array format
    JsonVariant objects_var = (*_doc)["objects"];
    if (!objects_var.isNull())
    {
        objects = objects_var.as<JsonArray>();
    }
    else if (_doc->is<JsonArray>())
    {
        objects = _doc->as<JsonArray>();
    }
    else
    {
        return false;
    }

    double best_distance = 999.0; // Start with a large distance
    JsonObject best_obj;
    bool found = false;

    for (JsonObject obj : objects)
    {
        double obj_ra = obj["ra_deg"].as<double>();
        double obj_dec = obj["dec_deg"].as<double>();

        double distance = angularDistance(ra_deg, dec_deg, obj_ra, obj_dec);

        if (distance < best_distance)
        {
            best_distance = distance;
            best_obj = obj;
            found = true;
        }
    }

    if (found)
    {
        return parseObjectFromJson(best_obj, result);
    }

    return false;
}

bool NGC2000::findByType(NGC_ObjectType type, NGCEntry& result) const
{
    if (!_using_json || !_doc)
    {
        return false;
    }

    JsonArray objects;

    // Handle both new metadata format and legacy array format
    JsonVariant objects_var = (*_doc)["objects"];
    if (!objects_var.isNull())
    {
        objects = objects_var.as<JsonArray>();
    }
    else if (_doc->is<JsonArray>())
    {
        objects = _doc->as<JsonArray>();
    }
    else
    {
        return false;
    }

    for (JsonObject obj : objects)
    {
        int obj_type = obj["type"].as<int>();

        if (obj_type == static_cast<int>(type))
        {
            return parseObjectFromJson(obj, result);
        }
    }

    return false;
}

bool NGC2000::parseObjectFromJson(JsonObject obj, NGCEntry& result) const
{
    if (obj.isNull())
    {
        return false;
    }

    // Clear the result object
    result = NGCEntry();

    // Parse basic fields
    result.name = obj["name"].as<String>();
    result.ra_deg = obj["ra_deg"].as<double>();
    result.dec_deg = obj["dec_deg"].as<double>();
    result.type = static_cast<NGC_ObjectType>(obj["type"].as<int>());

    // Parse optional fields with defaults
    result.magnitude = obj["magnitude"] | 0.0;
    result.size_arcmin = obj["size_arcmin"] | 0.0;
    result.constellation = obj["constellation"] | String("");
    result.notes = obj["notes"] | String("");

    return true;
}

double NGC2000::angularDistance(double ra1_deg, double dec1_deg, double ra2_deg,
                                double dec2_deg) const
{
    // Convert to radians
    double ra1 = ra1_deg * M_PI / 180.0;
    double dec1 = dec1_deg * M_PI / 180.0;
    double ra2 = ra2_deg * M_PI / 180.0;
    double dec2 = dec2_deg * M_PI / 180.0;

    // Use the haversine formula for better numerical stability
    double dra = ra2 - ra1;
    double ddec = dec2 - dec1;

    double a =
        sin(ddec / 2.0) * sin(ddec / 2.0) + cos(dec1) * cos(dec2) * sin(dra / 2.0) * sin(dra / 2.0);
    double c = 2.0 * asin(sqrt(a));

    // Convert back to degrees
    return c * 180.0 / M_PI;
}

bool NGC2000::isLoaded() const
{
    return _using_json && _doc && _object_count > 0;
}

void NGC2000::printCatalogInfo() const
{
    if (!_using_json || !_doc)
    {
        print_out("NGC2000 catalog not loaded");
        return;
    }

    print_out("=== NGC 2000.0 Catalog Info ===");

    JsonVariant catalog_var = (*_doc)["catalog"];
    if (!catalog_var.isNull())
    {
        print_out("Catalog: %s", catalog_var.as<String>().c_str());
    }

    JsonVariant version_var = (*_doc)["version"];
    if (!version_var.isNull())
    {
        print_out("Version: %s", version_var.as<String>().c_str());
    }

    JsonVariant coord_var = (*_doc)["coordinate_system"];
    if (!coord_var.isNull())
    {
        print_out("Coordinate System: %s", coord_var.as<String>().c_str());
    }

    print_out("Total Objects: %zu", _object_count);
    print_out("================================");
}

// Static utility functions
String NGC2000::typeToString(NGC_ObjectType type)
{
    switch (type)
    {
        case NGC_GALAXY:
            return "Galaxy";
        case NGC_OPEN_CLUSTER:
            return "Open Cluster";
        case NGC_GLOBULAR_CLUSTER:
            return "Globular Cluster";
        case NGC_PLANETARY_NEBULA:
            return "Planetary Nebula";
        case NGC_NEBULA:
            return "Nebula";
        case NGC_SINGLE_STAR:
            return "Star";
        case NGC_DOUBLE_STAR:
            return "Double Star";
        case NGC_ASTERISM:
            return "Asterism";
        case NGC_UNKNOWN:
            return "Other";
        default:
            return "Unknown";
    }
}

NGC_ObjectType NGC2000::stringToType(const String& type_str)
{
    String type_upper = type_str;
    type_upper.toUpperCase();

    if (type_upper == "GALAXY")
        return NGC_GALAXY;
    if (type_upper == "OPEN CLUSTER" || type_upper == "OPEN_CLUSTER")
        return NGC_OPEN_CLUSTER;
    if (type_upper == "GLOBULAR CLUSTER" || type_upper == "GLOBULAR_CLUSTER")
        return NGC_GLOBULAR_CLUSTER;
    if (type_upper == "PLANETARY NEBULA" || type_upper == "PLANETARY_NEBULA")
        return NGC_PLANETARY_NEBULA;
    if (type_upper == "NEBULA")
        return NGC_NEBULA;
    if (type_upper == "STAR")
        return NGC_SINGLE_STAR;
    if (type_upper == "DOUBLE STAR" || type_upper == "DOUBLE_STAR")
        return NGC_DOUBLE_STAR;
    if (type_upper == "ASTERISM")
        return NGC_ASTERISM;

    return NGC_UNKNOWN;
}

// StarDatabase virtual function implementations
bool NGC2000::loadDatabase(const char* json_data)
{
    return begin_json(String(json_data));
}

// New overload for pointer+length
bool NGC2000::loadDatabase(const char* json_data, size_t len)
{
    // Avoid extra copy if possible
    String json_str(json_data, len);
    return begin_json(json_str);
}

bool NGC2000::findByName(const String& name, UnifiedEntry& result) const
{
    NGCEntry ngc_obj;
    if (findNGCByName(name, ngc_obj))
    {
        return convertNGCToUnified(ngc_obj, result);
    }
    return false;
}

bool NGC2000::findByNameFragment(const String& name_fragment, UnifiedEntry& result) const
{
    NGCEntry ngc_obj;
    if (findNGCByNameFragment(name_fragment, ngc_obj))
    {
        return convertNGCToUnified(ngc_obj, result);
    }
    return false;
}

bool NGC2000::findByIndex(size_t index, UnifiedEntry& result) const
{
    // NGC doesn't have a direct findByIndex method, so we can't implement this
    // without iterating through the JSON, which would be inefficient
    return false;
}

size_t NGC2000::getTotalObjectCount() const
{
    return _object_count;
}

void NGC2000::printDatabaseInfo() const
{
    print_out("=== NGC2000 Database Info ===");
    print_out("Database Type: NGC2000 (New General Catalogue)");
    print_out("Loaded: %s", isLoaded() ? "Yes" : "No");
    if (isLoaded())
    {
        print_out("Total Objects: %zu", _object_count);
        printCatalogInfo();
    }
    print_out("============================");
}

String NGC2000::formatCoordinates(double ra_deg, double dec_deg) const
{
    // NGC style formatting (slightly different precision)
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

bool NGC2000::convertNGCToUnified(const NGCEntry& ngc, UnifiedEntry& unified) const
{
    unified.name = ngc.name;
    unified.type_str = ngc.getTypeString();
    unified.ra_deg = ngc.ra_deg;
    unified.dec_deg = ngc.dec_deg;
    unified.magnitude = ngc.magnitude;
    unified.constellation = ngc.constellation;
    unified.description = ngc.notes; // Use notes as description
    unified.source_db = DB_NGC2000;
    unified.spectral_type = "";
    unified.size_arcmin = ngc.size_arcmin;
    unified.notes = ngc.notes;

    return true;
}
