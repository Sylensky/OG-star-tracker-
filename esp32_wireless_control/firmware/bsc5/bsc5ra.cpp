#include "bsc5ra.h"
#include "uart.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <cmath>
#include <cstring>

// Global instance - simplified initialization
BSC5 bsc5(nullptr, nullptr);

// StarEntry methods
void StarEntry::print() const
{
    print_out("=== Star Information ===");
    print_out("Catalog ID: %u", id);
    print_out("Name: %s", name.c_str());
    print_out("Right Ascension (radians): %.10f", ra);
    print_out("Declination (radians): %.10f", dec);
    print_out("Spectral type: %s", spec.c_str());
    print_out("Magnitude: %.2f", mag);
    print_out("RA proper motion: %.15f", ra_pm);
    print_out("Dec proper motion: %.15f", dec_pm);
    if (notes.length() > 0)
    {
        print_out("Notes: %s", notes.c_str());
    }
}

// BSC5 class implementation - simplified
BSC5::BSC5(const uint8_t* start, const uint8_t* end)
    : StarDatabase(DB_BSC5), _start(start), _end(end), _doc(nullptr), _using_json(false),
      _star_count(0)
{
}

BSC5::BSC5()
    : StarDatabase(DB_BSC5), _start(nullptr), _end(nullptr), _doc(nullptr), _using_json(false),
      _star_count(0)
{
}

bool BSC5::begin_json(const String& json_data)
{
    _using_json = true;
    _star_count = 0;

    // Allocate JsonDocument
    if (_doc)
    {
        delete _doc;
    }
    _doc = new JsonDocument;

    DeserializationError error = deserializeJson(*_doc, json_data);
    if (error)
    {
        print_out("JSON parsing failed: %s", error.c_str());
        delete _doc;
        _doc = nullptr;
        return false;
    }

    // Simple approach - check for stars array or use direct array
    JsonVariant starsVar = (*_doc)["stars"];
    if (starsVar.is<JsonArray>())
    {
        // New format with metadata
        _star_count = starsVar.as<JsonArray>().size();

        JsonVariant catalogVar = (*_doc)["catalog"];
        if (catalogVar.is<const char*>())
        {
            print_out("Catalog: %s", catalogVar.as<const char*>());
        }
    }
    else if (_doc->is<JsonArray>())
    {
        // Legacy format - direct array
        _star_count = _doc->as<JsonArray>().size();
    }
    else
    {
        print_out("Invalid JSON format");
        delete _doc;
        _doc = nullptr;
        return false;
    }

    print_out("Loaded %zu stars from JSON", _star_count);
    return _star_count > 0;
}

bool BSC5::findStarById(uint32_t id, StarEntry& result) const
{
    if (!_using_json || !_doc)
        return false;

    JsonArray stars;
    JsonVariant starsVar = (*_doc)["stars"];

    if (starsVar.is<JsonArray>())
    {
        stars = starsVar.as<JsonArray>();
    }
    else if (_doc->is<JsonArray>())
    {
        stars = _doc->as<JsonArray>();
    }
    else
    {
        return false;
    }

    for (JsonVariant starVar : stars)
    {
        JsonObject star = starVar.as<JsonObject>();

        // Check for both 'id' and 'xno' fields
        uint32_t star_id = 0;
        if (star["id"].is<uint32_t>())
        {
            star_id = star["id"].as<uint32_t>();
        }
        else if (star["xno"].is<uint32_t>())
        {
            star_id = star["xno"].as<uint32_t>();
        }

        if (star_id == id)
        {
            return parseStarFromJson(star, result);
        }
    }

    return false;
}

bool BSC5::findStarByName(const String& name_fragment, StarEntry& result) const
{
    if (!_using_json || !_doc)
        return false;

    JsonArray stars;
    JsonVariant starsVar = (*_doc)["stars"];

    if (starsVar.is<JsonArray>())
    {
        stars = starsVar.as<JsonArray>();
    }
    else if (_doc->is<JsonArray>())
    {
        stars = _doc->as<JsonArray>();
    }
    else
    {
        return false;
    }

    String search_term = name_fragment;
    search_term.toLowerCase();

    for (JsonVariant starVar : stars)
    {
        JsonObject star = starVar.as<JsonObject>();
        String star_name = star["name"].as<String>();
        star_name.toLowerCase();

        if (star_name.indexOf(search_term) >= 0)
        {
            return parseStarFromJson(star, result);
        }
    }

    return false;
}

bool BSC5::parseStarFromJson(JsonObject star_obj, StarEntry& star) const
{
    // Handle ID field
    if (star_obj["id"].is<uint32_t>())
    {
        star.id = star_obj["id"].as<uint32_t>();
    }
    else if (star_obj["xno"].is<uint32_t>())
    {
        star.id = star_obj["xno"].as<uint32_t>();
    }

    // Handle coordinate fields
    if (star_obj["ra"].is<double>())
    {
        star.ra = star_obj["ra"].as<double>();
        star.dec = star_obj["dec"].as<double>();
    }
    else
    {
        star.ra = star_obj["sra0"].as<double>();
        star.dec = star_obj["sdec0"].as<double>();
    }

    // Handle spectral type
    if (star_obj["spec"].is<const char*>())
    {
        star.spec = star_obj["spec"].as<String>();
    }
    else if (star_obj["spectral_type"].is<const char*>())
    {
        star.spec = star_obj["spectral_type"].as<String>();
    }

    star.mag = star_obj["mag"].as<float>();
    star.name = star_obj["name"].as<String>();

    // Handle proper motion (set both field names for compatibility)
    star.ra_pm = star_obj["ra_pm"].as<double>();
    star.dec_pm = star_obj["dec_pm"].as<double>();
    star.pm_ra = star.ra_pm;
    star.pm_dec = star.dec_pm;

    // Handle notes
    star.notes = "";
    JsonVariant notesVar = star_obj["notes"];
    if (notesVar.is<JsonArray>())
    {
        JsonArray notes_array = notesVar.as<JsonArray>();
        for (JsonVariant note : notes_array)
        {
            if (star.notes.length() > 0)
            {
                star.notes += "; ";
            }
            star.notes += note.as<String>();
        }
    }

    return true;
}

void BSC5::printStar(const StarEntry& star) const
{
    star.print();
}

// Legacy Entry print method
void Entry::print()
{
    print_out("Legacy Entry:");
    print_out("XNO: %.0f", xno);
    print_out("RA: %.10f", sra0);
    print_out("Dec: %.10f", sdec0);
    print_out("Spectral: %s", is.c_str());
    print_out("Magnitude: %.2f", mag / 100.0);
}

// StarDatabase virtual function implementations
bool BSC5::loadDatabase(const char* json_data)
{
    return begin_json(String(json_data));
}

// New overload for pointer+length
bool BSC5::loadDatabase(const char* json_data, size_t len)
{
    // Avoid extra copy if possible
    String json_str(json_data, len);
    return begin_json(json_str);
}

bool BSC5::isLoaded() const
{
    return _using_json && _doc && _star_count > 0;
}

bool BSC5::findByName(const String& name, UnifiedEntry& result) const
{
    StarEntry star;
    if (findStarByName(name, star))
    {
        return convertStarToUnified(star, result);
    }
    return false;
}

bool BSC5::findByNameFragment(const String& name_fragment, UnifiedEntry& result) const
{
    if (!_using_json || !_doc)
        return false;

    JsonArray stars;
    JsonVariant starsVar = (*_doc)["stars"];

    if (starsVar.is<JsonArray>())
    {
        stars = starsVar.as<JsonArray>();
    }
    else if (_doc->is<JsonArray>())
    {
        stars = _doc->as<JsonArray>();
    }
    else
    {
        return false;
    }

    String search_term = name_fragment;
    search_term.toLowerCase();

    for (JsonVariant starVar : stars)
    {
        JsonObject star = starVar.as<JsonObject>();
        String star_name = star["name"].as<String>();
        star_name.toLowerCase();

        if (star_name.indexOf(search_term) >= 0)
        {
            StarEntry star_entry;
            if (parseStarFromJson(star, star_entry))
            {
                return convertStarToUnified(star_entry, result);
            }
        }
    }

    return false;
}

bool BSC5::findByIndex(size_t index, UnifiedEntry& result) const
{
    StarEntry star;
    if (findStarById(index + 1, star)) // BSC5 IDs typically start from 1
    {
        return convertStarToUnified(star, result);
    }
    return false;
}

size_t BSC5::getTotalObjectCount() const
{
    return _star_count;
}

void BSC5::printDatabaseInfo() const
{
    print_out("=== BSC5 Database Info ===");
    print_out("Database Type: BSC5 (Yale Bright Star Catalog)");
    print_out("Loaded: %s", isLoaded() ? "Yes" : "No");
    if (isLoaded())
    {
        print_out("Total Stars: %zu", _star_count);
    }
    print_out("=========================");
}

String BSC5::formatCoordinates(double ra_deg, double dec_deg) const
{
    // BSC5 style formatting
    int ra_h = (int) (ra_deg / 15.0);
    int ra_m = (int) ((ra_deg / 15.0 - ra_h) * 60.0);
    double ra_s = ((ra_deg / 15.0 - ra_h) * 60.0 - ra_m) * 60.0;

    int dec_d = (int) abs(dec_deg);
    int dec_m = (int) ((abs(dec_deg) - dec_d) * 60.0);
    double dec_s = ((abs(dec_deg) - dec_d) * 60.0 - dec_m) * 60.0;

    char coord_str[50];
    snprintf(coord_str, sizeof(coord_str), "%02dh%02dm%06.3fs %c%02dd%02dm%06.3fs", ra_h, ra_m,
             ra_s, (dec_deg >= 0) ? '+' : '-', dec_d, dec_m, dec_s);

    return String(coord_str);
}

bool BSC5::convertStarToUnified(const StarEntry& star, UnifiedEntry& unified) const
{
    unified.name = star.name.length() > 0 ? star.name : String("HR ") + String(star.id);
    unified.type_str = "Star";
    unified.ra_deg = star.ra * 180.0 / M_PI; // Convert from radians to degrees
    unified.dec_deg = star.dec * 180.0 / M_PI;
    unified.magnitude = star.mag;
    unified.constellation = ""; // BSC5 doesn't include constellation in our JSON
    unified.description = "";
    unified.source_db = DB_BSC5;
    unified.spectral_type = star.spec;
    unified.size_arcmin = 0.0;
    unified.notes = star.notes;

    return true;
}
