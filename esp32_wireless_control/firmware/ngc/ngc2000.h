#ifndef NGC2000_H
#define NGC2000_H

#include "../star_database.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstdint>

// NGC2000 Object Types
enum NGC_ObjectType
{
    NGC_GALAXY = 0,       // Gx - Galaxy
    NGC_OPEN_CLUSTER,     // OC - Open star cluster
    NGC_GLOBULAR_CLUSTER, // Gb - Globular star cluster
    NGC_NEBULA,           // Nb - Bright emission or reflection nebula
    NGC_PLANETARY_NEBULA, // Pl - Planetary nebula
    NGC_CLUSTER_NEBULA,   // C+N - Cluster associated with nebulosity
    NGC_ASTERISM,         // Ast - Asterism or group of a few stars
    NGC_KNOT,             // Kt - Knot or nebulous region in external galaxy
    NGC_TRIPLE_STAR,      // *** - Triple star
    NGC_DOUBLE_STAR,      // D* - Double star
    NGC_SINGLE_STAR,      // * - Single star
    NGC_UNCERTAIN,        // ? - Uncertain type or may not exist
    NGC_NONEXISTENT,      // - - Object called nonexistent
    NGC_PLATE_DEFECT,     // PD - Photographic plate defect
    NGC_UNKNOWN           // (blank) - Unidentified or type unknown
};

// Structure to hold NGC object data
struct NGCEntry
{
    String name;          // NGC or IC designation (e.g., "NGC1234", "IC456")
    NGC_ObjectType type;  // Object type classification
    double ra_deg;        // Right Ascension in degrees (J2000)
    double dec_deg;       // Declination in degrees (J2000)
    String constellation; // Constellation abbreviation
    float size_arcmin;    // Largest dimension in arcminutes
    float magnitude;      // Integrated magnitude
    String notes;         // Object notes/description

    // Utility methods
    void print() const;
    String getTypeString() const;
};

// Main NGC2000 catalog class
class NGC2000 : public StarDatabase
{
  public:
    NGC2000(const uint8_t* start, const uint8_t* end);
    NGC2000(); // Default constructor for StarDatabase inheritance
    ~NGC2000();

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

    // NGC2000-specific methods (keeping existing interface)
    bool begin_json(const String& json_data);
    bool findNGCByName(const String& name, NGCEntry& result) const;
    bool findNGCByNameFragment(const String& name_fragment, NGCEntry& result) const;
    bool findByType(NGC_ObjectType type, NGCEntry& result) const;
    bool findByRADec(double ra_deg, double dec_deg, double radius_deg, NGCEntry& result) const;
    bool findNearestToRADec(double ra_deg, double dec_deg, NGCEntry& result) const;
    size_t getObjectCount() const;
    void printCatalogInfo() const;

  private:
    const uint8_t* _start; // Start of raw data (if used)
    const uint8_t* _end;   // End of raw data (if used)
    JsonDocument* _doc;
    bool _using_json;
    size_t _object_count;

    // Helper methods
    bool parseObjectFromJson(JsonObject obj_json, NGCEntry& obj) const;
    double angularDistance(double ra1_deg, double dec1_deg, double ra2_deg, double dec2_deg) const;

    // Helper method to convert NGCEntry to UnifiedEntry
    bool convertNGCToUnified(const NGCEntry& ngc, UnifiedEntry& unified) const;

    // Static utility methods
    static String typeToString(NGC_ObjectType type);
    static NGC_ObjectType stringToType(const String& type_str);
};

extern NGC2000 ngc2000;

#endif // NGC2000_H
