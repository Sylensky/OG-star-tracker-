# BSC5 Data and Conversion

## Source
- The Yale Bright Star Catalog (BSC5) is available at following link:
  - http://tdc-www.harvard.edu/catalogs/bsc5.html
- Download the main data file (e.g., `bsc5.dat`) from the FTP archive.

## JSON Format Support
The BSC5 class now supports JSON format data for easier integration with modern applications:

### Conversion Scripts
- Python scripts are provided in this folder to convert the BSC5 data to JSON:
  ```sh
  python bsc5_to_json.py
  python bsc5ra_to_json.py
  ```

### Using JSON Data
1. Add the JSON file to your PlatformIO configuration:
   ```ini
   [env:esp32]
   board_build.embed_txtfiles = bsc5/bsc5ra_stars_with_notes.json
   ```

2. Initialize the BSC5 class with JSON data:
   ```cpp
   #include "bsc5ra.h"
   
   // Load JSON data
   bsc5.loadFromJson(bsc5ra_stars_with_notes_json_start);
   
   // Find star by catalog number
   StarEntry star;
   if (bsc5.findStarByXno(15, star)) {
       star.print();
   }
   
   // Find star by name (partial, case-insensitive)
   if (bsc5.findStarByName("deneb", star)) {
       star.print();
   }
   ```

### Available Methods
- `findStarByXno(float xno, StarEntry& result)` - Find star by catalog number
- `findStarByName(const String& name_fragment, StarEntry& result)` - Find star by name (supports partial matching)
- `printStar(const StarEntry& star)` - Print star information
- `getStarCount()` - Get total number of stars loaded

## Notes
- The JSON format includes star names, notes, and proper motion data
- Name search is case-insensitive and supports partial matching (e.g., "deneb" will find "DENEB")
- Original binary format is still supported for backward compatibility
