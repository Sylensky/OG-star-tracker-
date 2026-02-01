
#include <Arduino.h>
#include <cstring> // For memcmp, memcpy
#include <vector>

#include "ngc2000.h"
#include "uart.h"

NGC2000::NGC2000(const uint8_t* start, const uint8_t* end)
    : _start(start), _end(end), _object_count(0), _page_size(32), _is_compact(false),
      _header_size(12)
{
    // Reserve space for up to 3 pages to minimize heap drop on first load
    _binary_entries.reserve(_page_size * 3);
}

NGC2000::~NGC2000()
{
    clearLoadedPages();
}

bool NGC2000::loadDatabase(const char* binary_data, size_t len)
{
    // Initialize pagination mode - just parse header, don't load all data
    _start = reinterpret_cast<const uint8_t*>(binary_data);
    _end = _start + len;

    if (len < 8)
    {
        print_out("Error: Binary data too small for header");
        return false;
    }

    uint32_t num_objects = *reinterpret_cast<const uint32_t*>(_start);
    const char* magic = reinterpret_cast<const char*>(_start + 4);
    // version is unused

    if (memcmp(magic, "NGC2", 4) == 0)
    {
        _is_compact = false;
        _header_size = 12;
        print_out("NGC2000 Pagination Mode (full): objects=%u, page_size=%zu", num_objects,
                  _page_size);
    }
    else if (memcmp(magic, "NC2C", 4) == 0)
    {
        _is_compact = true;
        _header_size = 12;
        print_out("NGC2000 Pagination Mode (compact): objects=%u, page_size=%zu", num_objects,
                  _page_size);
    }
    else
    {
        print_out("Error: Invalid NGC2000 magic header");
        return false;
    }

    _object_count = num_objects;

    print_out("NGC2000 header parsed: %zu objects total, pagination enabled", _object_count);
    return true;
}

bool NGC2000::unloadDatabase()
{
    clearLoadedPages();
    _object_count = 0;
    return true;
}

bool NGC2000::isLoaded() const
{
    return _object_count > 0 && (_start != nullptr);
}

size_t NGC2000::getTotalObjectCount() const
{
    return _object_count;
}

bool NGC2000::findByName(const String& name, StarUnifiedEntry& result) const
{
    // First check already loaded entries for performance
    NGCEntry ngc_obj;
    if (findNGCByName(name, ngc_obj))
    {
        return convertNGCToUnified(ngc_obj, result);
    }

    // If not found in loaded pages, search through all pages
    return findByNameInAllPages(name, result);
}

bool NGC2000::findByNameFragment(const String& name_fragment, StarUnifiedEntry& result) const
{
    // First check already loaded entries for performance
    NGCEntry ngc_obj;
    if (findNGCByNameFragment(name_fragment, ngc_obj))
    {
        return convertNGCToUnified(ngc_obj, result);
    }

    // If not found in loaded pages, search through all pages
    return findByNameFragmentInAllPages(name_fragment, result);
}

bool NGC2000::findByIndex(size_t index, StarUnifiedEntry& result) const
{
    if (index >= _object_count)
    {
        return false;
    }

    // Check if the page containing this index is loaded
    size_t page_index = getPageForIndex(index);
    if (!isPageLoaded(page_index))
    {
        // Dynamically load the required page
        const_cast<NGC2000*>(this)->loadPage(page_index, _page_size);
    }

    // Directly load and convert the object
    BinaryNGCEntry target_obj;
    if (loadObjectAtIndex(index, target_obj))
    {
        NGCEntry ngc_obj;
        if (parseObjectFromBinary(target_obj, ngc_obj))
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

// Pagination implementation
bool NGC2000::loadPage(size_t page_index, size_t page_size)
{
    if (isPageLoaded(page_index))
        return true; // Already loaded

    size_t start_index = page_index * page_size;
    size_t end_index =
        (start_index + page_size < _object_count) ? start_index + page_size : _object_count;

    if (start_index >= _object_count)
    {
        return false;
    }

    // Calculate byte offset for fixed-size records
    size_t record_size = _is_compact ? sizeof(CompactBinaryNGCEntry) : sizeof(BinaryNGCEntry);

    for (size_t i = start_index; i < end_index; i++)
    {
        size_t offset = _header_size + i * record_size;
        if (offset + record_size <= (_end - _start))
        {
            BinaryNGCEntry entry = {};

            if (_is_compact)
            {
                const CompactBinaryNGCEntry* src =
                    reinterpret_cast<const CompactBinaryNGCEntry*>(_start + offset);
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
            }
            else
            {
                const BinaryNGCEntry* src =
                    reinterpret_cast<const BinaryNGCEntry*>(_start + offset);
                entry = *src;
            }

            _binary_entries.push_back(entry);
        }
    }

    _loaded_pages.push_back(page_index);

    return true;
}

bool NGC2000::isPageLoaded(size_t page_index) const
{
    for (size_t loaded : _loaded_pages)
    {
        if (loaded == page_index)
        {
            return true;
        }
    }
    return false;
}

size_t NGC2000::getCurrentPageCount() const
{
    return _loaded_pages.size();
}

size_t NGC2000::getPageSize() const
{
    return _page_size;
}

void NGC2000::clearLoadedPages()
{
    _binary_entries.clear();
    _loaded_pages.clear();
}

size_t NGC2000::getPageForIndex(size_t global_index) const
{
    return global_index / _page_size;
}

size_t NGC2000::getLocalIndexInPage(size_t global_index, size_t page_index) const
{
    return global_index - (page_index * _page_size);
}

bool NGC2000::loadObjectAtIndex(size_t global_index, BinaryNGCEntry& obj) const
{
    if (global_index >= _object_count)
    {
        return false;
    }

    size_t record_size = _is_compact ? sizeof(CompactBinaryNGCEntry) : sizeof(BinaryNGCEntry);
    size_t offset = _header_size + global_index * record_size;

    if (offset + record_size > (_end - _start))
    {
        return false;
    }

    if (_is_compact)
    {
        const CompactBinaryNGCEntry* src =
            reinterpret_cast<const CompactBinaryNGCEntry*>(_start + offset);
        memcpy(obj.id, src->id, 12);
        memcpy(obj.type, src->type, 4);
        obj.ra = src->ra;
        obj.dec = src->dec;
        obj.constellation[0] = '\0';
        obj.constellation[1] = '\0';
        obj.constellation[2] = '\0';
        obj.size_arcmin = 0.0f;
        obj.magnitude = src->magnitude;
        obj.description[0] = '\0';
    }
    else
    {
        const BinaryNGCEntry* src = reinterpret_cast<const BinaryNGCEntry*>(_start + offset);
        obj = *src;
    }

    return true;
}

bool NGC2000::findByNameInAllPages(const String& name, StarUnifiedEntry& result) const
{
    String search_term = name;
    search_term.toUpperCase();

    print_out("NGC2000: Searching all pages for '%s'", name.c_str());

    // Search through all objects page by page
    size_t total_pages = (_object_count + _page_size - 1) / _page_size;

    for (size_t page = 0; page < total_pages; page++)
    {
        if (!isPageLoaded(page))
            const_cast<NGC2000*>(this)->loadPage(page, _page_size);

        size_t start_index = page * _page_size;
        size_t end_index =
            (start_index + _page_size < _object_count) ? start_index + _page_size : _object_count;

        for (size_t i = start_index; i < end_index; i++)
        {
            BinaryNGCEntry obj;
            if (loadObjectAtIndex(i, obj))
            {
                String obj_id(obj.id);
                obj_id.toUpperCase();
                if (obj_id == search_term)
                {
                    print_out("NGC2000: Found '%s' in page %zu", name.c_str(), page);
                    NGCEntry ngc_obj;
                    if (parseObjectFromBinary(obj, ngc_obj))
                    {
                        // Clear all loaded pages before returning
                        const_cast<NGC2000*>(this)->clearLoadedPages();
                        return convertNGCToUnified(ngc_obj, result);
                    }
                }
            }
        }
        // Clear this page after iterating
        const_cast<NGC2000*>(this)->clearLoadedPages();
    }

    print_out("NGC2000: Object '%s' not found in catalog", name.c_str());
    return false;
}

bool NGC2000::findByNameFragmentInAllPages(const String& name_fragment,
                                           StarUnifiedEntry& result) const
{
    String search_term = name_fragment;
    search_term.toUpperCase();

    print_out("NGC2000: Searching all pages for fragment '%s'", name_fragment.c_str());

    // Search through all objects page by page
    size_t total_pages = (_object_count + _page_size - 1) / _page_size;

    for (size_t page = 0; page < total_pages; page++)
    {
        if (!isPageLoaded(page))
            const_cast<NGC2000*>(this)->loadPage(page, _page_size);

        size_t start_index = page * _page_size;
        size_t end_index =
            (start_index + _page_size < _object_count) ? start_index + _page_size : _object_count;

        for (size_t i = start_index; i < end_index; i++)
        {
            BinaryNGCEntry obj;
            if (loadObjectAtIndex(i, obj))
            {
                String obj_id(obj.id);
                obj_id.toUpperCase();
                if (obj_id.indexOf(search_term) >= 0)
                {
                    print_out("NGC2000: Found fragment '%s' in '%s' (page %zu)",
                              name_fragment.c_str(), obj.id, page);
                    NGCEntry ngc_obj;
                    if (parseObjectFromBinary(obj, ngc_obj))
                    {
                        // Clear all loaded pages before returning
                        const_cast<NGC2000*>(this)->clearLoadedPages();
                        return convertNGCToUnified(ngc_obj, result);
                    }
                }
            }
        }
        // Clear this page after iterating
        const_cast<NGC2000*>(this)->clearLoadedPages();
    }

    print_out("NGC2000: Fragment '%s' not found in catalog", name_fragment.c_str());
    return false;
}
