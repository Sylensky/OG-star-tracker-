/**
 * @file bsc5ra.h
 * @version 0.1.0
 *
 * @section License
 * Copyright (C) 2025, Sylensky
 */

#ifndef _BSC5RA_H_
#define _BSC5RA_H_

#include <WString.h>
#include <stdint.h>
#include <vector>

#include "../star_database.h"

// Structure to hold BSC5 star data
struct BSC5Entry
{
    uint32_t id;   // Star catalog ID
    double ra;     // Right Ascension in radians
    double dec;    // Declination in radians
    String spec;   // Spectral type (shortened)
    float mag;     // Magnitude
    String name;   // Star name
    double pm_ra;  // RA proper motion
    double pm_dec; // Dec proper motion
    String notes;  // Concatenated notes

    // Utility methods
    void print() const;
};

// Helper struct for binary star objects (full format)
#pragma pack(push, 1)
struct BinaryBSC5Entry
{
    double ra;        // Right Ascension in radians
    double dec;       // Declination in radians
    float mag;        // Magnitude
    char spec[2];     // Spectral type (2 chars, no null terminator)
    uint8_t name_len; // Name length
    // Variable length name follows (no notes in binary)
};
#pragma pack(pop)

// Helper struct for binary star objects (compact format)
#pragma pack(push, 1)
struct CompactBinaryBSC5Entry
{
    double ra;        // Right Ascension in radians
    double dec;       // Declination in radians
    float mag;        // Magnitude
    uint8_t name_len; // Name length
    // Variable length name follows
};
#pragma pack(pop)

// Main BSC5 catalog class
// Implements StarDatabaseInterface
class BSC5 : public StarDatabaseInterface
{
  public:
    BSC5(const uint8_t* start, const uint8_t* end);
    BSC5();
    virtual ~BSC5();

    // StarDatabaseInterface implementations (binary only)
    bool loadDatabase(const char* binary_data, size_t len) override;
    bool unloadDatabase() override;
    bool isLoaded() const override;

    bool findByName(const String& name, StarUnifiedEntry& result) const override;
    bool findByNameFragment(const String& name_fragment, StarUnifiedEntry& result) const override;
    bool findByIndex(size_t index, StarUnifiedEntry& result) const override;

    void printDatabaseInfo() const override;
    size_t getTotalObjectCount() const override;

  private:
    const uint8_t* _start; // Start of raw data
    const uint8_t* _end;   // End of raw data
    size_t _star_count;
    std::vector<BSC5Entry> _binary_entries;

    // Helper methods
    bool begin_binary(const uint8_t* data, size_t len);
    bool findStarByNameInEntries(const String& name, BSC5Entry& result) const;
    bool findStarByNameFragmentInEntries(const String& name_fragment, BSC5Entry& result) const;

    bool parseStarFromBinary(size_t index, BSC5Entry& star) const;
    bool convertStarToUnified(const BSC5Entry& star, StarUnifiedEntry& unified) const;
};

extern BSC5 bsc5;

#endif /* _BSC5RA_H_ */
