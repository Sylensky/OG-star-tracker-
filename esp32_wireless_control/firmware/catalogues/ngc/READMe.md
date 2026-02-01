# NGC 2000.0 Minimal Catalog Conversion Summary

## Catalog Overview
The NGC 2000.0 (New General Catalogue) is a comprehensive catalog of deep-sky objects, including galaxies (Gx), open clusters (OC), globular clusters (Gb), planetary nebulae (Pl), and bright stars (*). For this project, a minimal subset was generated for embedded use on the ESP32 star tracker.

## Conversion Commands Executed

### Binary Catalog Generation
```
python .\ngc2000_convert.py --preset minimal --binary
```
- **Input:** sources/ngc2000.dat
- **Object types included:** Gx, OC, Gb, Pl, *
- **Magnitude range:** 2.0 ≤ mag ≤ 8.0
- **Output:** converted/ngc2000.bin
- **Result:**
  - Objects: 233
  - File size: 19,351 bytes (83.1 bytes/object)

### JSON Catalog Generation
```
python .\ngc2000_convert.py --preset minimal
```
- **Input:** sources/ngc2000.dat
- **Object types included:** Gx, OC, Gb, Pl, *
- **Magnitude range:** 2.0 ≤ mag ≤ 8.0
- **Output:** converted/ngc2000.json
- **Result:**
  - Objects: 233
  - File size: 54,428 bytes (233.6 bytes/object)

## Binary Catalog Format (ngc2000.bin)
- **Header:**
  - 4 bytes: Number of objects (uint32_t, little-endian)
  - 4 bytes: Magic string ("NGC2")
  - 4 bytes: Version (uint32_t, little-endian)
- **Each object (83 bytes, packed, no padding):**
  - 12 bytes: ID (ASCII, null-padded)
  - 4 bytes: Type (ASCII, null-padded)
  - 4 bytes: Right Ascension (float, hours)
  - 4 bytes: Declination (float, degrees)
  - 3 bytes: Constellation (ASCII, no null terminator)
  - 4 bytes: Size (float, arcminutes)
  - 4 bytes: Magnitude (float)
  - 48 bytes: Description (ASCII, null-padded)

## JSON Catalog Format (ngc2000.json)
- Array of 233 objects, each with fields:
  - `id`, `type`, `ra`, `dec`, `constellation`, `size_arcmin`, `magnitude`, `description`

## Example Object (from binary and JSON):
```
ID: NGC55
Type: Gx
RA: 0.248333 hours
Dec: -39.183334 deg
Constellation: Scl
Size: 32.4 arcmin
Magnitude: 8.0
Description: vB, vL, vmE, triN...
```

## Notes
- The binary format is optimized for fast, memory-efficient access on embedded systems.
- The JSON format is human-readable and suitable for debugging or catalog inspection.
- Both formats contain the same 233 objects, filtered by type and magnitude as specified above.
