#ifndef BSC5_DATABASE_H
#define BSC5_DATABASE_H

#include "bsc5/bsc5ra.h"
#include "star_database.h"


// Specialized BSC5 database class
class BSC5Database : public StarDatabase
{
  private:
    BSC5 _bsc5;

  public:
    // Constructor
    BSC5Database() : StarDatabase(DB_BSC5)
    {
    }
    virtual ~BSC5Database()
    {
    }

    // Database management
    virtual bool loadDatabase(const char* json_data) override
    {
        return _bsc5.begin_json(String(json_data));
    }

    virtual bool isLoaded() const override
    {
        return _bsc5.isLoaded();
    }

    // Unified search interface
    virtual bool findByName(const String& name, UnifiedEntry& result) const override
    {
        StarEntry star;
        if (_bsc5.findStarByName(name, star))
        {
            return convertStarToUnified(star, result);
        }
        return false;
    }

    virtual bool findByNameFragment(const String& name_fragment,
                                    UnifiedEntry& result) const override
    {
        StarEntry star;
        if (_bsc5.findStarByNameFragment(name_fragment, star))
        {
            return convertStarToUnified(star, result);
        }
        return false;
    }

    virtual bool findByIndex(size_t index, UnifiedEntry& result) const override
    {
        StarEntry star;
        if (_bsc5.findStarById(index + 1, star)) // BSC5 IDs typically start from 1
        {
            return convertStarToUnified(star, result);
        }
        return false;
    }

    // Information methods
    virtual size_t getTotalObjectCount() const override
    {
        return _bsc5.getStarCount();
    }

    virtual void printDatabaseInfo() const override
    {
        print_out("=== BSC5 Database Info ===");
        print_out("Database Type: BSC5 (Yale Bright Star Catalog)");
        print_out("Loaded: %s", isLoaded() ? "Yes" : "No");
        if (isLoaded())
        {
            print_out("Total Stars: %zu", _bsc5.getStarCount());
        }
        print_out("=========================");
    }

    // Utility methods
    virtual String formatCoordinates(double ra_deg, double dec_deg) const override
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

    // Access to underlying BSC5 instance for specialized operations
    BSC5& getBSC5()
    {
        return _bsc5;
    }
    const BSC5& getBSC5() const
    {
        return _bsc5;
    }

  private:
    // Helper method to convert StarEntry to UnifiedEntry
    bool convertStarToUnified(const StarEntry& star, UnifiedEntry& unified) const
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
};

#endif // BSC5_DATABASE_H
