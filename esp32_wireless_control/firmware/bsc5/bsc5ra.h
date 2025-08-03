#ifndef _BSC5RA_H_
#define _BSC5RA_H_ 1

#include "../star_database.h"
#include <ArduinoJson.h>
#include <WString.h>
#include <stdint.h>

// JSON-based star entry structure (optimized for ArduinoJson)
struct StarEntry
{
    uint32_t id;   // Star catalog ID
    double ra;     // Right Ascension in radians
    double dec;    // Declination in radians
    String spec;   // Spectral type (shortened)
    float mag;     // Magnitude
    String name;   // Star name
    double pm_ra;  // RA proper motion (for compatibility)
    double pm_dec; // Dec proper motion (for compatibility)
    double ra_pm;  // RA proper motion (alternative name)
    double dec_pm; // Dec proper motion (alternative name)
    String notes;  // Concatenated notes

    StarEntry() : id(0), ra(0), dec(0), mag(0), pm_ra(0), pm_dec(0), ra_pm(0), dec_pm(0)
    {
    }

    void print() const;
};

// Legacy Entry class for compatibility
class Entry
{
  public:
    Entry(float xno, double sra0, double sdec0, const char* is, uint16_t mag, float xrpm,
          float xdpm)
        : xno(xno), sra0(sra0), sdec0(sdec0), is(is), mag(mag), xrpm(xrpm), xdpm(xdpm)
    {
    }
    float xno;
    double sra0;
    double sdec0;
    String is;
    uint16_t mag;
    float xrpm;
    float xdpm;
    void print();
};

// Legacy binary data structures (for compatibility)
typedef struct bsc5_header
{
    int32_t star0;
    int32_t star1;
    int32_t starn;
    int32_t stnum;
    int32_t mprop;
    int32_t nmag;
    uint32_t nbent;
} __attribute__((__packed__)) bsc5_header_t;

typedef struct bsc5_entry
{
    union
    {
        uint32_t _xno;
        float xno;
    };
    union
    {
        uint64_t _sra0;
        double sra0;
    };
    union
    {
        uint64_t _sdec0;
        double sdec0;
    };
    char is[2];
    uint16_t mag;
    union
    {
        uint32_t _xrpm;
        float xrpm;
    };
    union
    {
        uint32_t _xdpm;
        float xdpm;
    };
} __attribute__((__packed__)) bsc5_entry_t;

class BSC5 : public StarDatabase
{
  public:
    BSC5(const uint8_t* start, const uint8_t* end);
    BSC5(); // Default constructor for StarDatabase inheritance

    // StarDatabase virtual function implementations
    virtual bool loadDatabase(const char* json_data) override;
    virtual bool loadDatabase(const char* json_data, size_t len) override;
    virtual bool isLoaded() const override;
    virtual bool findByName(const String& name, UnifiedEntry& result) const override;
    virtual bool findByNameFragment(const String& name_fragment,
                                    UnifiedEntry& result) const override;
    virtual bool findByIndex(size_t index, UnifiedEntry& result) const override;
    virtual size_t getTotalObjectCount() const override;
    virtual void printDatabaseInfo() const override;
    virtual String formatCoordinates(double ra_deg, double dec_deg) const override;

    // BSC5-specific methods (keeping existing interface)
    bool begin_json(const String& json_data);
    bool findStarById(uint32_t id, StarEntry& result) const;
    bool findStarByName(const String& name_fragment, StarEntry& result) const;
    bool findStarByNameFragment(const String& name_fragment, StarEntry& result) const;
    void printStar(const StarEntry& star) const;
    size_t getStarCount() const
    {
        return _star_count;
    }

  private:
    const uint8_t* _start;
    const uint8_t* _end;
    JsonDocument* _doc; // ArduinoJson document
    bool _using_json;
    size_t _star_count;

    // Helper methods for ArduinoJson parsing
    bool parseStarFromJson(JsonObject star_obj, StarEntry& star) const;

    // Helper method to convert StarEntry to UnifiedEntry
    bool convertStarToUnified(const StarEntry& star, UnifiedEntry& unified) const;
};

extern BSC5 bsc5;

#endif /* _BSC5RA_H_ */
