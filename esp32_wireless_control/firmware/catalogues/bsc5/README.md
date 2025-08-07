# BSC5RA Catalog Conversion Instructions

## Overview
This folder contains the Yale Bright Star Catalog (BSC5) converter script (`bsc5ra_convert.py`). It generates binary and JSON catalog files for use on embedded systems (e.g., ESP32 star tracker).

## Conversion Commands

### Full Binary Catalog
```
python .\bsc5ra_convert.py --binary
```
- **Input:** sources/BSC5ra.bsc5, sources/ybsc5.names, sources/ybsc5.notes
- **Output:** converted/bsc5ra.bin
- **Result:**
  - All bright stars with valid names
  - Notes are NOT included in the binary format

### Compact Binary Catalog
```
python .\bsc5ra_convert.py --binary --compact
```
- **Input:** sources/BSC5ra.bsc5, sources/ybsc5.names
- **Output:** converted/bsc5ra_compact.bin
- **Result:**
  - Only essential fields (no spectral type, no notes)
  - Name limited to 32 characters

### Full JSON Catalog
```
python .\bsc5ra_convert.py
```
- **Input:** sources/BSC5ra.bsc5, sources/ybsc5.names, sources/ybsc5.notes
- **Output:** converted/bsc5ra.json
- **Result:**
  - All fields, including notes

### Compact JSON Catalog
```
python .\bsc5ra_convert.py --compact
```
- **Input:** sources/BSC5ra.bsc5, sources/ybsc5.names
- **Output:** converted/bsc5ra_compact.json
- **Result:**
  - Only essential fields

## Notes
- Names are clipped at the first semicolon, period, or comma, and truncated to fit format limits.
- Stars with names exceeding the allowed length after clipping are omitted from the binary output.
- The binary format is optimized for embedded use; the JSON format is for inspection and debugging.
