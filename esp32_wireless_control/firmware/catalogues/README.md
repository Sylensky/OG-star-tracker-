# Star Catalogues Integration Overview

## Purpose
This folder contains the star catalogues and conversion tools for the ESP32 star tracker firmware. It supports multiple catalogues (NGC2000, BSC5) and provides a unified search and access interface via the `star_database` backend.

## Backend Structure
- **star_database_interface.h / star_database.cpp**
  - Defines the unified interface (`StarDatabaseInterface`) for all catalogue backends.
  - Provides the `StarUnifiedEntry` struct for consistent search results.
  - Handles catalogue selection, loading, and search dispatching.

- **ngc/ngc2000.cpp / ngc2000.h** ([NGC2000 Backend Documentation](ngc/README.md))
  - Implements the NGC2000 backend, supporting binary and JSON formats.
  - Parses deep-sky object data and exposes unified search methods.

- **bsc5/bsc5ra.cpp / bsc5ra.h** ([BSC5 Backend Documentation](bsc5/README.md))
  - Implements the BSC5 backend, supporting binary and JSON formats.
  - Parses bright star data and exposes unified search methods.

## Mechanism
1. **Catalogue Conversion**
   - Python scripts (`ngc2000_convert.py`, `bsc5ra_convert.py`) convert raw catalogue data to binary and JSON formats optimized for embedded use.
2. **Catalogue Loading**
   - At runtime, the firmware loads the selected catalogue (NGC2000 or BSC5) using the backend's `loadDatabase()` method.
3. **Unified Search**
   - All catalogue backends implement the same interface, allowing the main firmware to search by name, index, or fragment without knowing the catalogue details.
   - Results are returned as `StarUnifiedEntry` objects, containing all relevant fields (name, coordinates, magnitude, etc.).
4. **Backend Selection**
   - The main firmware can switch between catalogues (NGC2000, BSC5) as needed, using the unified interface for all operations.

## Example
- To search for a star or object, the firmware calls `findByName()` on the active backend.
- The backend parses its internal data and returns a unified result, regardless of catalogue format.

## Notes
- This modular approach allows easy addition of new catalogues or formats in the future.
- All catalogue-specific details are encapsulated in their respective backend classes.
