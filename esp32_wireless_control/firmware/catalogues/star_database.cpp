#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "bsc5/bsc5ra.h"
#include "ngc/ngc2000.h"

#include "star_database.h"
#include "uart.h"

StarDatabase::StarDatabase(StarDatabaseType db_type, const uint8_t* start, const uint8_t* end)
    : _db_type(db_type), _backend(nullptr)
{
    switch (db_type)
    {
        case DB_NGC2000:
        case DB_NGC2000_COMPACT:
            _backend = new NGC2000(start, end);
            break;
        case DB_BSC5:
        case DB_BSC5_COMPACT:
            _backend = new BSC5(start, end);
            break;
        default:
            print_out("Error: Unsupported database type %d", db_type);
            break;
    }
}

// Pagination interface
bool StarDatabase::loadPage(size_t page_index, size_t page_size)
{
    if (_backend)
        return _backend->loadPage(page_index, page_size);
    return false;
}

bool StarDatabase::isPageLoaded(size_t page_index) const
{
    if (_backend)
        return _backend->isPageLoaded(page_index);
    return false;
}

size_t StarDatabase::getCurrentPageCount() const
{
    if (_backend)
        return _backend->getCurrentPageCount();
    return 0;
}

size_t StarDatabase::getPageSize() const
{
    if (_backend)
        return _backend->getPageSize();
    return 0;
}

void StarDatabase::clearLoadedPages()
{
    if (_backend)
        _backend->clearLoadedPages();
}
StarDatabase::~StarDatabase()
{
    delete _backend;
}

bool StarDatabase::loadDatabase(const char* data, size_t len)
{
    if (_backend)
        return _backend->loadDatabase(data, len);
    return false;
}

bool StarDatabase::unloadDatabase()
{
    if (_backend)
    {
        _backend->unloadDatabase();
        return true;
    }
    return false;
}

bool StarDatabase::isLoaded() const
{
    if (_backend)
        return _backend->isLoaded();
    return false;
}

bool StarDatabase::findByName(const String& name, StarUnifiedEntry& result) const
{
    if (_backend)
        return _backend->findByName(name, result);
    return false;
}

bool StarDatabase::findByNameFragment(const String& name_fragment, StarUnifiedEntry& result) const
{
    if (_backend)
        return _backend->findByNameFragment(name_fragment, result);
    return false;
}

bool StarDatabase::findByIndex(size_t index, StarUnifiedEntry& result) const
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

void StarUnifiedEntry::print() const
{
    print_out("=== Object Information ===");
    print_out("Name: %s", name.c_str());
    print_out("Type: %s", type_str.c_str());
    print_out("Right Ascension: %.6f hours", ra_hours);
    print_out("Declination: %.6f degrees", dec_deg);

    if (constellation.length() > 0)
        print_out("Constellation: %s", constellation.c_str());
    if (magnitude > 0)
        print_out("Magnitude: %.2f", magnitude);
    if (spectral_type.length() > 0)
        print_out("Spectral Type: %s", spectral_type.c_str());
    if (size_arcmin > 0)
        print_out("Size: %.1f arcmin", size_arcmin);
    if (description.length() > 0)
        print_out("Description: %s", description.c_str());
    if (notes.length() > 0)
        print_out("Notes: %s", notes.c_str());
    print_out("==========================");
}
