#include <Arduino.h>
#include <cmath>
#include <cstring>
#include <stdio.h>

#include "bsc5ra.h"
#include "uart.h"

BSC5::BSC5(const uint8_t* start, const uint8_t* end) : _start(start), _end(end), _star_count(0)
{
}

BSC5::~BSC5()
{
    _binary_entries.clear();
}

bool BSC5::loadDatabase(const char* data, size_t len)
{
    return begin_binary(reinterpret_cast<const uint8_t*>(data), len);
}

bool BSC5::unloadDatabase()
{
    _binary_entries.clear();
    _star_count = 0;
    return true;
}

bool BSC5::isLoaded() const
{
    return !_binary_entries.empty();
}

size_t BSC5::getTotalObjectCount() const
{
    return _star_count;
}

bool BSC5::findByName(const String& name, StarUnifiedEntry& result) const
{
    BSC5Entry star;
    if (findStarByNameInEntries(name, star))
    {
        return convertStarToUnified(star, result);
    }
    return false;
}

bool BSC5::findByNameFragment(const String& name_fragment, StarUnifiedEntry& result) const
{
    BSC5Entry star;
    if (findStarByNameFragmentInEntries(name_fragment, star))
    {
        return convertStarToUnified(star, result);
    }
    return false;
}

bool BSC5::findByIndex(size_t index, StarUnifiedEntry& result) const
{
    if (index < _binary_entries.size())
    {
        return convertStarToUnified(_binary_entries[index], result);
    }
    return false;
}

bool BSC5::begin_binary(const uint8_t* data, size_t len)
{
    size_t header_offset = 14; // count(4) + magic(6) + version(4)
    uint32_t num_stars = *reinterpret_cast<const uint32_t*>(data);
    const char* magic = reinterpret_cast<const char*>(data + 4);
    uint32_t version = *reinterpret_cast<const uint32_t*>(data + 10);
    _binary_entries.clear();

    if (len < 14)
    {
        print_out("Error: Binary data too small for header");
        return false;
    }

    bool is_full = false, is_compact = false;
    if (memcmp(magic, "BSC5RA", 6) == 0)
        is_full = true;
    else if (memcmp(magic, "BSC5RC", 6) == 0)
        is_compact = true;

    if (is_full)
    {
        print_out("BSC5 Binary Header (full): stars=%u, magic=%.6s, version=%u", num_stars, magic,
                  version);
        while (header_offset < len && _binary_entries.size() < num_stars)
        {
            // Read struct from binary
            const BinaryBSC5Entry* src =
                reinterpret_cast<const BinaryBSC5Entry*>(data + header_offset);
            BinaryBSC5Entry entry = *src;
            header_offset += sizeof(BinaryBSC5Entry);

            // Read name
            String name = "";
            if (header_offset + entry.name_len <= len && entry.name_len > 0)
            {
                name = String(reinterpret_cast<const char*>(data + header_offset), entry.name_len);
                header_offset += entry.name_len;
            }

            // Fill BSC5Entry
            BSC5Entry star;
            star.ra = entry.ra;
            star.dec = entry.dec;
            star.mag = entry.mag;
            star.spec = String(entry.spec, 2);
            star.name = name;
            star.id = _binary_entries.size() + 1;
            star.pm_ra = 0.0;
            star.pm_dec = 0.0;
            star.notes = ""; // Not in binary

            _binary_entries.push_back(star);
        }
    }
    else if (is_compact)
    {
        print_out("BSC5 Binary Header (compact): stars=%u, magic=%.6s, version=%u", num_stars,
                  magic, version);
        while (header_offset < len && _binary_entries.size() < num_stars)
        {
            const CompactBinaryBSC5Entry* src =
                reinterpret_cast<const CompactBinaryBSC5Entry*>(data + header_offset);
            CompactBinaryBSC5Entry entry = *src;
            header_offset += sizeof(CompactBinaryBSC5Entry);

            // Read name
            String name = "";
            if (header_offset + entry.name_len <= len && entry.name_len > 0)
            {
                name = String(reinterpret_cast<const char*>(data + header_offset), entry.name_len);
                header_offset += entry.name_len;
            }

            // Fill BSC5Entry
            BSC5Entry star;
            star.ra = entry.ra;
            star.dec = entry.dec;
            star.mag = entry.mag;
            star.spec = "";
            star.name = name;
            star.id = _binary_entries.size() + 1;
            star.pm_ra = 0.0;
            star.pm_dec = 0.0;
            star.notes = ""; // Not in binary

            _binary_entries.push_back(star);
        }
    }

    _star_count = _binary_entries.size();
    print_out("Loaded %zu BSC5 stars from binary (expected %u)", _star_count, num_stars);

    if (_star_count > 0)
    {
        const BSC5Entry& first = _binary_entries[0];
        print_out("First star debug:");
        print_out("  ID: %u", first.id);
        print_out("  Name: %s", first.name.c_str());
        print_out("  RA: %.6f radians", first.ra);
        print_out("  Dec: %.6f radians", first.dec);
        print_out("  Magnitude: %.1f", first.mag);
    }
    return true;
}

bool BSC5::findStarByNameInEntries(const String& name, BSC5Entry& result) const
{
    String search_term = name;
    search_term.toLowerCase();

    for (const auto& star : _binary_entries)
    {
        String star_name_lc = star.name;
        star_name_lc.toLowerCase();
        if (star_name_lc == search_term)
        {
            result = star;
            return true;
        }
    }
    return false;
}

bool BSC5::findStarByNameFragmentInEntries(const String& name_fragment, BSC5Entry& result) const
{
    String search_term = name_fragment;
    search_term.toLowerCase();

    for (const auto& star : _binary_entries)
    {
        String star_name_lc = star.name;
        star_name_lc.toLowerCase();
        if (star_name_lc.indexOf(search_term) >= 0)
        {
            result = star;
            return true;
        }
    }
    return false;
}

bool BSC5::convertStarToUnified(const BSC5Entry& star, StarUnifiedEntry& unified) const
{
    unified.name = star.name.length() > 0 ? star.name : String("HR ") + String(star.id);
    unified.type_str = "Star";                 // BSC5 is primarily stars
    unified.ra_hours = star.ra * (12.0 / PI);  // Convert radians to hours
    unified.dec_deg = star.dec * (180.0 / PI); // Convert radians to degrees
    unified.magnitude = star.mag;
    unified.constellation = ""; // BSC5 doesn't include constellation in our JSON
    unified.description = "";
    unified.source_db = DB_BSC5;
    unified.spectral_type = star.spec;
    unified.size_arcmin = 0.0;
    unified.notes = star.notes;

    return true;
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
