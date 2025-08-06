#include <Arduino.h>
#include <cstring>
#include <vector>

#include "ngc2000.h"
#include "uart.h"

NGC2000::NGC2000(const uint8_t* start, const uint8_t* end)
    : _start(start), _end(end), _object_count(0)
{
}

NGC2000::~NGC2000()
{
    _binary_entries.clear();
}

bool NGC2000::loadDatabase(const char* binary_data, size_t len)
{
    return begin_binary(reinterpret_cast<const uint8_t*>(binary_data), len);
}

bool NGC2000::unloadDatabase()
{
    _binary_entries.clear();
    _object_count = 0;
    return true;
}

bool NGC2000::isLoaded() const
{
    return !_binary_entries.empty();
}

size_t NGC2000::getTotalObjectCount() const
{
    return _object_count;
}

bool NGC2000::findByName(const String& name, StarUnifiedEntry& result) const
{
    NGCEntry ngc_obj;
    if (findNGCByName(name, ngc_obj))
    {
        return convertNGCToUnified(ngc_obj, result);
    }
    return false;
}

bool NGC2000::findByNameFragment(const String& name_fragment, StarUnifiedEntry& result) const
{
    NGCEntry ngc_obj;
    if (findNGCByNameFragment(name_fragment, ngc_obj))
    {
        return convertNGCToUnified(ngc_obj, result);
    }
    return false;
}

bool NGC2000::findByIndex(size_t index, StarUnifiedEntry& result) const
{
    if (index < _binary_entries.size())
    {
        NGCEntry ngc_obj;
        if (parseObjectFromBinary(_binary_entries[index], ngc_obj))
        {
            return convertNGCToUnified(ngc_obj, result);
        }
    }
    return false;
}

bool NGC2000::begin_binary(const uint8_t* data, size_t len)
{
    size_t header_offset = 12;
    uint32_t num_objects = *reinterpret_cast<const uint32_t*>(data);
    const char* magic = reinterpret_cast<const char*>(data + 4);
    uint32_t version = *reinterpret_cast<const uint32_t*>(data + 8);
    _binary_entries.clear();

    // Check if we have enough data for the header (8 bytes minimum)
    if (len < 8)
    {
        print_out("Error: Binary data too small for header");
        return false;
    }

    bool is_full = false, is_compact = false;
    if (memcmp(magic, "NGC2", 4) == 0)
        is_full = true;
    else if (memcmp(magic, "NC2C", 4) == 0)
        is_compact = true;

    if (is_full)
    {
        print_out("NGC Binary Header (full): objects=%u, magic=%.4s, version=%u", num_objects,
                  magic, version);
        print_out("BinaryNGCEntry size: %zu bytes", sizeof(BinaryNGCEntry));
        while (header_offset + sizeof(BinaryNGCEntry) <= len &&
               _binary_entries.size() < num_objects)
        {
            BinaryNGCEntry entry;
            const BinaryNGCEntry* src =
                reinterpret_cast<const BinaryNGCEntry*>(data + header_offset);
            entry = *src;
            _binary_entries.push_back(entry);
            header_offset += sizeof(BinaryNGCEntry);
        }
    }
    else if (is_compact)
    {
        print_out("NGC Binary Header (compact): objects=%u, magic=%.4s", num_objects, magic);
        print_out("CompactBinaryNGCEntry size: %zu bytes", sizeof(CompactBinaryNGCEntry));
        for (size_t i = 0; i < num_objects && header_offset + sizeof(CompactBinaryNGCEntry) <= len;
             ++i)
        {
            const CompactBinaryNGCEntry* src =
                reinterpret_cast<const CompactBinaryNGCEntry*>(data + header_offset);
            BinaryNGCEntry entry = {};
            memcpy(entry.id, src->id, 12);
            memcpy(entry.type, src->type, 4);
            entry.ra = src->ra;
            entry.dec = src->dec;
            entry.constellation[0] = '\0';
            entry.constellation[1] = '\0';
            entry.constellation[2] = '\0';
            entry.size_arcmin = 0.0f;
            entry.magnitude = src->magnitude;
            entry.description[0] = '\0';
            _binary_entries.push_back(entry);
            header_offset += sizeof(CompactBinaryNGCEntry);
        }
    }
    _object_count = _binary_entries.size();
    print_out("Loaded %zu NGC objects from binary (expected %u)", _object_count, num_objects);

    // Debug: show first object if loaded
    if (_object_count > 0)
    {
        const BinaryNGCEntry& first = _binary_entries[0];
        print_out("First object debug:");
        print_out("  ID: %.12s", first.id);
        print_out("  Type: %.4s", first.type);
        print_out("  RA: %.6f hours", first.ra);
        print_out("  Dec: %.6f deg", first.dec);
        char const_debug[4] = {first.constellation[0], first.constellation[1],
                               first.constellation[2], '\0'};
        print_out("  Constellation: %s", const_debug);
        print_out("  Magnitude: %.1f", first.magnitude);
    }
    return true;
}

bool NGC2000::findNGCByName(const String& name, NGCEntry& result) const
{
    String search_id = name;
    search_id.toUpperCase();
    for (const auto& entry : _binary_entries)
    {
        String entry_id(entry.id);
        entry_id.toUpperCase();
        if (entry_id == search_id)
        {
            return parseObjectFromBinary(entry, result);
        }
    }
    return false;
}

bool NGC2000::findNGCByNameFragment(const String& name_fragment, NGCEntry& result) const
{
    String search_term = name_fragment;
    search_term.toUpperCase();
    for (const auto& entry : _binary_entries)
    {
        String entry_id(entry.id);
        entry_id.toUpperCase();
        if (entry_id.indexOf(search_term) >= 0)
        {
            return parseObjectFromBinary(entry, result);
        }
    }
    return false;
}

void NGC2000::printDatabaseInfo() const
{
    print_out("=== NGC 2000.0 Catalog Info ===");
    print_out("Total Objects: %zu", _object_count);
    print_out("================================");
}

bool NGC2000::parseObjectFromBinary(const BinaryNGCEntry& bin, NGCEntry& result) const
{
    result.name = String(bin.id);   // id as string
    result.type = String(bin.type); // type as string
    result.ra_hours = bin.ra;       // store as hours
    result.dec_deg = bin.dec;
    // constellation is 3 bytes, create null-terminated string manually
    char const_str[4];
    const_str[0] = bin.constellation[0];
    const_str[1] = bin.constellation[1];
    const_str[2] = bin.constellation[2];
    const_str[3] = '\0';
    result.constellation = String(const_str);
    result.size_arcmin = bin.size_arcmin;
    result.magnitude = bin.magnitude;
    result.notes = String(bin.description);
    result.description = String(bin.description);
    return true;
}

bool NGC2000::convertNGCToUnified(const NGCEntry& ngc, StarUnifiedEntry& unified) const
{
    unified.name = ngc.name;
    unified.type_str = ngc.type;
    unified.ra_hours = ngc.ra_hours;
    unified.dec_deg = ngc.dec_deg;
    unified.magnitude = ngc.magnitude;
    unified.constellation = ngc.constellation;
    unified.description = ngc.notes;
    unified.source_db = DB_NGC2000;
    unified.spectral_type = "";
    unified.size_arcmin = ngc.size_arcmin;
    unified.notes = ngc.notes;
    return true;
}
