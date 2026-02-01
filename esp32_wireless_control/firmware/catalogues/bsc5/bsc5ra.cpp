#include <Arduino.h>
#include <cmath>
#include <cstring>
#include <stdio.h>

#include "bsc5ra.h"
#include "uart.h"

BSC5::BSC5(const uint8_t* start, const uint8_t* end)
    : _start(start), _end(end), _star_count(0), _page_size(32), _is_compact(false), _header_size(14)
{
    // Reserve space for up to 3 pages to minimize heap drop on first load
    _binary_entries.reserve(_page_size * 3);
}

BSC5::~BSC5()
{
    clearLoadedPages();
}

bool BSC5::loadDatabase(const char* data, size_t len)
{
    // Initialize pagination mode - just parse header, don't load all data
    _start = reinterpret_cast<const uint8_t*>(data);
    _end = _start + len;

    if (len < 14)
    {
        print_out("Error: Binary data too small for header");
        return false;
    }

    uint32_t num_stars = *reinterpret_cast<const uint32_t*>(_start);
    const char* magic = reinterpret_cast<const char*>(_start + 4);
    // version is unused

    if (memcmp(magic, "BSC5RA", 6) == 0)
    {
        _is_compact = false;
        print_out("BSC5 Pagination Mode (full): stars=%u, page_size=%zu", num_stars, _page_size);
    }
    else if (memcmp(magic, "BSC5RC", 6) == 0)
    {
        _is_compact = true;
        print_out("BSC5 Pagination Mode (compact): stars=%u, page_size=%zu", num_stars, _page_size);
    }
    else
    {
        print_out("Error: Invalid BSC5 magic header");
        return false;
    }

    _star_count = num_stars;
    _header_size = 14;

    print_out("BSC5 header parsed: %zu stars total, pagination enabled", _star_count);
    return true;
}

bool BSC5::unloadDatabase()
{
    clearLoadedPages();
    _star_count = 0;
    return true;
}

bool BSC5::isLoaded() const
{
    return _star_count > 0 && (_start != nullptr);
}

size_t BSC5::getTotalObjectCount() const
{
    return _star_count;
}

bool BSC5::findByName(const String& name, StarUnifiedEntry& result) const
{
    // First check already loaded entries for performance
    BSC5Entry star;
    if (findStarByNameInEntries(name, star))
    {
        return convertStarToUnified(star, result);
    }

    // If not found in loaded pages, search through all pages
    return findByNameInAllPages(name, result);
}

bool BSC5::findByNameFragment(const String& name_fragment, StarUnifiedEntry& result) const
{
    // First check already loaded entries for performance
    BSC5Entry star;
    if (findStarByNameFragmentInEntries(name_fragment, star))
    {
        return convertStarToUnified(star, result);
    }

    // If not found in loaded pages, search through all pages
    return findByNameFragmentInAllPages(name_fragment, result);
}

bool BSC5::findByIndex(size_t index, StarUnifiedEntry& result) const
{
    if (index >= _star_count)
    {
        return false;
    }

    // Check if the page containing this index is loaded
    size_t page_index = getPageForIndex(index);
    if (!isPageLoaded(page_index))
    {
        // Dynamically load the required page
        const_cast<BSC5*>(this)->loadPage(page_index, _page_size);
    }

    // Search through loaded entries for the one with matching global index
    for (const auto& star : _binary_entries)
    {
        if (star.id == index + 1)
        { // IDs are 1-based
            return convertStarToUnified(star, result);
        }
    }

    return false;
}

// Pagination implementation
bool BSC5::loadPage(size_t page_index, size_t page_size)
{
    if (isPageLoaded(page_index))
    {
        return true; // Already loaded
    }

    size_t start_index = page_index * page_size;
    size_t end_index =
        (start_index + page_size < _star_count) ? start_index + page_size : _star_count;

    if (start_index >= _star_count)
    {
        return false;
    }

    // Parse the page by scanning through binary data
    size_t header_offset = _header_size;
    size_t current_star = 0;

    while (header_offset < (_end - _start) && current_star < end_index)
    {
        if (current_star >= start_index)
        {
            // This star is in our target page
            BSC5Entry star;
            if (loadStarAtIndex(current_star, star))
            {
                _binary_entries.push_back(star);
            }
        }

        // Skip to next star (calculate offset increment)
        if (_is_compact)
        {
            header_offset += sizeof(CompactBinaryBSC5Entry);
            if (header_offset < (_end - _start))
            {
                uint8_t name_len =
                    *(_start + header_offset - 1); // name_len is last field in struct
                header_offset += name_len;
            }
        }
        else
        {
            header_offset += sizeof(BinaryBSC5Entry);
            if (header_offset < (_end - _start))
            {
                uint8_t name_len =
                    *(_start + header_offset - 1); // name_len is last field in struct
                header_offset += name_len;
            }
        }
        current_star++;
    }

    _loaded_pages.push_back(page_index);

    return true;
}

bool BSC5::isPageLoaded(size_t page_index) const
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

size_t BSC5::getCurrentPageCount() const
{
    return _loaded_pages.size();
}

size_t BSC5::getPageSize() const
{
    return _page_size;
}

void BSC5::clearLoadedPages()
{
    _binary_entries.clear();
    _loaded_pages.clear();
}

size_t BSC5::getPageForIndex(size_t global_index) const
{
    return global_index / _page_size;
}

size_t BSC5::getLocalIndexInPage(size_t global_index, size_t page_index) const
{
    return global_index - (page_index * _page_size);
}

bool BSC5::loadStarAtIndex(size_t global_index, BSC5Entry& star) const
{
    // This is a simplified implementation that scans from the beginning
    // A more optimized version would cache star offsets
    size_t header_offset = _header_size;
    size_t current_star = 0;

    while (header_offset < (_end - _start) && current_star <= global_index)
    {
        if (current_star == global_index)
        {
            // Found the target star
            if (_is_compact)
            {
                const CompactBinaryBSC5Entry* src =
                    reinterpret_cast<const CompactBinaryBSC5Entry*>(_start + header_offset);
                header_offset += sizeof(CompactBinaryBSC5Entry);

                String name = "";
                if (header_offset + src->name_len <= (_end - _start) && src->name_len > 0)
                {
                    name = String(reinterpret_cast<const char*>(_start + header_offset),
                                  src->name_len);
                }

                star.ra = src->ra;
                star.dec = src->dec;
                star.mag = src->mag;
                star.spec = "";
                star.name = name;
                star.id = global_index + 1;
                star.pm_ra = 0.0;
                star.pm_dec = 0.0;
                star.notes = "";
                return true;
            }
            else
            {
                const BinaryBSC5Entry* src =
                    reinterpret_cast<const BinaryBSC5Entry*>(_start + header_offset);
                header_offset += sizeof(BinaryBSC5Entry);

                String name = "";
                if (header_offset + src->name_len <= (_end - _start) && src->name_len > 0)
                {
                    name = String(reinterpret_cast<const char*>(_start + header_offset),
                                  src->name_len);
                }

                star.ra = src->ra;
                star.dec = src->dec;
                star.mag = src->mag;
                star.spec = String(src->spec, 2);
                star.name = name;
                star.id = global_index + 1;
                star.pm_ra = 0.0;
                star.pm_dec = 0.0;
                star.notes = "";
                return true;
            }
        }

        // Skip to next star
        if (_is_compact)
        {
            const CompactBinaryBSC5Entry* src =
                reinterpret_cast<const CompactBinaryBSC5Entry*>(_start + header_offset);
            header_offset += sizeof(CompactBinaryBSC5Entry);
            if (header_offset < (_end - _start))
            {
                header_offset += src->name_len;
            }
        }
        else
        {
            const BinaryBSC5Entry* src =
                reinterpret_cast<const BinaryBSC5Entry*>(_start + header_offset);
            header_offset += sizeof(BinaryBSC5Entry);
            if (header_offset < (_end - _start))
            {
                header_offset += src->name_len;
            }
        }
        current_star++;
    }

    return false;
}

bool BSC5::findByNameInAllPages(const String& name, StarUnifiedEntry& result) const
{
    String search_term = name;
    search_term.toLowerCase();

    print_out("BSC5: Searching all pages for '%s'", name.c_str());

    // Search through all stars page by page
    size_t total_pages = (_star_count + _page_size - 1) / _page_size;

    for (size_t page = 0; page < total_pages; page++)
    {
        if (!isPageLoaded(page))
            const_cast<BSC5*>(this)->loadPage(page, _page_size);

        for (const auto& star : _binary_entries)
        {
            size_t star_page = getPageForIndex(star.id - 1);
            if (star_page == page)
            {
                String star_name_lc = star.name;
                star_name_lc.toLowerCase();
                if (star_name_lc == search_term)
                {
                    print_out("BSC5: Found '%s' in page %zu", name.c_str(), page);
                    // Clear all loaded pages before returning
                    const_cast<BSC5*>(this)->clearLoadedPages();
                    return convertStarToUnified(star, result);
                }
            }
        }
        // Clear this page after iterating
        const_cast<BSC5*>(this)->clearLoadedPages();
    }

    print_out("BSC5: Star '%s' not found in catalog", name.c_str());
    return false;
}

bool BSC5::findByNameFragmentInAllPages(const String& name_fragment, StarUnifiedEntry& result) const
{
    String search_term = name_fragment;
    search_term.toLowerCase();

    print_out("BSC5: Searching all pages for fragment '%s'", name_fragment.c_str());

    // Search through all stars page by page
    size_t total_pages = (_star_count + _page_size - 1) / _page_size;

    for (size_t page = 0; page < total_pages; page++)
    {
        if (!isPageLoaded(page))
            const_cast<BSC5*>(this)->loadPage(page, _page_size);

        for (const auto& star : _binary_entries)
        {
            size_t star_page = getPageForIndex(star.id - 1);
            if (star_page == page)
            {
                String star_name_lc = star.name;
                star_name_lc.toLowerCase();
                if (star_name_lc.indexOf(search_term) >= 0)
                {
                    print_out("BSC5: Found fragment '%s' in '%s' (page %zu)", name_fragment.c_str(),
                              star.name.c_str(), page);
                    // Clear all loaded pages before returning
                    const_cast<BSC5*>(this)->clearLoadedPages();
                    return convertStarToUnified(star, result);
                }
            }
        }
        // Clear this page after iterating
        const_cast<BSC5*>(this)->clearLoadedPages();
    }

    print_out("BSC5: Fragment '%s' not found in catalog", name_fragment.c_str());
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
