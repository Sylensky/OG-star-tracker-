# StarDatabase - Unified Catalog System

The StarDatabase system provides a unified interface for accessing both stellar and deep-sky object catalogs in the ESP32 star tracker firmware.

## Overview

The StarDatabase acts as a parent class that manages multiple astronomical catalogs:
- **BSC5 (Yale Bright Star Catalog)**: Contains ~9,000 bright stars with proper names
- **NGC 2000.0**: Contains nebulae, galaxies, star clusters, and other deep-sky objects

## Architecture

```
StarDatabase (Parent Class)
├── BSC5 Integration (Stars)
│   ├── Star search by name/fragment
│   ├── Stellar coordinates and magnitudes
│   └── Spectral types and proper motion
└── NGC2000 Integration (Deep-Sky Objects)
    ├── Object search by name/type/constellation
    ├── Deep-sky coordinates and magnitudes
    └── Object sizes and descriptions
```

## Key Features

### 1. Unified Search Interface
```cpp
UnifiedEntry obj;

// Search across all loaded databases
starDB.findByName("Sirius", obj);
starDB.findByNameFragment("NGC", obj);
starDB.findBrighterThan(3.0, obj);
```

### 2. Database Switching
```cpp
// Use only stars
starDB.setActiveDatabase(DB_BSC5);

// Use only deep-sky objects  
starDB.setActiveDatabase(DB_NGC2000);

// Search both databases
starDB.setActiveDatabase(DB_BOTH);
```

### 3. Type-Specific Searches
```cpp
// Find stellar objects only
starDB.findStar("Deneb", obj);

// Find deep-sky objects only
starDB.findDeepSkyObject("NGC1234", obj);
```

### 4. Coordinate Utilities
```cpp
// Format coordinates for display
String coords = starDB.formatCoordinates(ra_deg, dec_deg);

// Calculate angular separation
double separation = starDB.angularSeparation(ra1, dec1, ra2, dec2);
```

## Data Structures

### UnifiedEntry
Common structure for all astronomical objects:
```cpp
struct UnifiedEntry {
    String name;           // Object name
    String type_str;       // Object type
    double ra_deg;         // Right Ascension (degrees)
    double dec_deg;        // Declination (degrees)
    float magnitude;       // Visual magnitude
    String constellation;  // Constellation (if available)
    DatabaseType source_db; // Source database
    bool is_star;          // Stellar object flag
    bool is_deep_sky;      // Deep-sky object flag
    
    // Extended data
    String spectral_type;  // For stars (BSC5)
    float size_arcmin;     // For deep-sky objects (NGC)
    String notes;          // Additional information
};
```

## Usage Examples

### Basic Setup
```cpp
#include "star_database.h"

void setup() {
    // Load catalogs
    starDB.loadBSC5(bsc5_json_data);
    starDB.loadNGC2000(ngc_json_data);
    
    // Set active database
    starDB.setActiveDatabase(DB_BOTH);
    
    // Print database info
    starDB.printDatabaseInfo();
}
```

### Object Search
```cpp
UnifiedEntry obj;

// Find by exact name
if (starDB.findByName("Vega", obj)) {
    starDB.printObject(obj);
}

// Find by name fragment
if (starDB.findByNameFragment("Alpha", obj)) {
    print_out("Found: %s", obj.name.c_str());
}

// Find bright objects
if (starDB.findBrighterThan(2.0, obj)) {
    print_out("Bright object: %s (mag %.2f)", 
              obj.name.c_str(), obj.magnitude);
}
```

### Telescope Integration
```cpp
// Goto command example
UnifiedEntry target;
if (starDB.findByName("M42", target)) {
    telescope.slewTo(target.ra_deg, target.dec_deg);
    print_out("Slewing to %s at %s", 
              target.name.c_str(),
              target.getCoordinateString().c_str());
}
```

## Database Backends

### BSC5 Integration
- **Data Source**: Yale Bright Star Catalog (5th Edition)
- **Object Count**: ~9,000 stars
- **Coordinate System**: J2000
- **Key Fields**: Name, spectral type, magnitude, proper motion
- **Search Methods**: By name fragment, stellar ID

### NGC2000 Integration  
- **Data Source**: New General Catalogue and Index Catalogue
- **Object Count**: Variable (filtered by magnitude/type)
- **Coordinate System**: J2000
- **Key Fields**: Name, object type, size, constellation
- **Search Methods**: By name, type, constellation, magnitude

## Performance Considerations

### Memory Usage
- Uses ArduinoJson for efficient JSON parsing
- Unified data structures minimize memory overhead
- Lazy loading of catalog data

### Search Performance
- Direct JSON traversal for name-based searches
- Type-specific searches route to appropriate backend
- Caching of frequently accessed objects (future enhancement)

## File Structure

```
firmware/
├── star_database.h              # Main header
├── star_database.cpp            # Implementation
├── star_database_example.cpp    # Usage examples
├── bsc5/
│   ├── bsc5ra.h                # BSC5 star catalog
│   ├── bsc5ra.cpp              # BSC5 implementation
│   └── bsc5ra_stars_with_notes.json
└── ngc/
    ├── ngc2000.h               # NGC catalog
    ├── ngc2000.cpp             # NGC implementation
    └── ngc2000.json
```

## Integration with Existing Code

### UART Output
All output uses the project's `print_out()` function from `uart.h`:
```cpp
print_out("Found object: %s", obj.name.c_str());
```

### JSON Data Loading
Compatible with PlatformIO embedded file system:
```cpp
extern const char bsc5_json_data[];
extern const char ngc_json_data[];
```

### Arduino Framework
Full compatibility with ESP32 Arduino framework and existing project structure.

## Future Enhancements

### Planned Features
1. **Proximity Search**: Find objects near telescope position
2. **Visibility Calculations**: Filter by altitude/azimuth
3. **Observing Lists**: Create custom target lists
4. **Messier Integration**: Cross-reference NGC objects with Messier numbers
5. **Search Caching**: Improve performance for repeated searches

### API Extensions
1. **Multiple Result Search**: Return arrays of matching objects
2. **Advanced Filtering**: By magnitude range, object size, etc.
3. **Coordinate Transformations**: Alt/Az calculations
4. **Time-based Visibility**: Rise/set times and optimal viewing

## Error Handling

The system includes comprehensive error handling:
- JSON parsing validation
- Database loading verification
- Search result validation
- Memory management for large catalogs

## Debugging

Enable detailed output for troubleshooting:
```cpp
starDB.printDatabaseInfo();  // Database status
obj.print();                 // Object details
```

Use the project's existing UART debugging system for consistent logging across all modules.
