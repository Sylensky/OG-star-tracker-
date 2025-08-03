#!/usr/bin/env python3
"""
Convert NGC 2000.0 catalog data to JSON format

Based on the format specification from the README:
   Bytes Format  Units   Label    Explanations
   1-  5  A5     ---     Name     NGC or IC designation (preceded by I)
   7-  9  A3     ---     Type     Object classification
  11- 12  I2     h       RAh      Right Ascension B2000 (hours)
  14- 17  F4.1   min     RAm      Right Ascension B2000 (minutes)
      20  A1     ---     DE-      Declination B2000 (sign)
  21- 22  I2     deg     DEd      Declination B2000 (degrees)
  24- 25  I2     arcmin  DEm      Declination B2000 (minutes)
      27  A1     ---     Source   Source of entry
  30- 32  A3     ---     Const    Constellation
      33  A1     ---   l_size     [<] Limit on Size
  34- 38  F5.1   arcmin  size     ? Largest dimension
  41- 44  F4.1   mag     mag      ? Integrated magnitude
      45  A1     ---   n_mag      [p] 'p' if mag is photographic
  47- 99  A53    ---     Desc     Description of the object

Object Types (from README):
     Gx  = Galaxy
     OC  = Open star cluster
     Gb  = Globular star cluster, usually in the Milky Way Galaxy
     Nb  = Bright emission or reflection nebula
     Pl  = Planetary nebula
     C+N = Cluster associated with nebulosity
     Ast = Asterism or group of a few stars
     Kt  = Knot or nebulous region in an external galaxy
     *** = Triple star
     D*  = Double star
     *   = Single star
     ?   = Uncertain type or may not exist
         = (blank) Unidentified at the place given, or type unknown
     -   = Object called nonexistent in the RNGC
     PD  = Photographic plate defect
"""

import json
import argparse

def parse_ngc_line(line):
    """Parse a single line from the NGC 2000.0 data file"""
    if len(line) < 99:
        line = line.ljust(99)  # Pad line if it's too short
    
    # Extract fields according to byte positions (adjust for 0-based indexing)
    name = line[0:5].strip()
    obj_type = line[6:9].strip()
    ra_h = line[10:12].strip()
    ra_m = line[13:17].strip()
    de_sign = line[19:20].strip()
    de_d = line[20:22].strip()
    de_m = line[23:25].strip()
    source = line[26:27].strip()
    constellation = line[29:32].strip()
    l_size = line[32:33].strip()
    size = line[33:38].strip()
    magnitude = line[40:44].strip()
    n_mag = line[44:45].strip()
    description = line[46:99].strip()
    
    # Convert numeric fields
    try:
        ra_hours = int(ra_h) if ra_h else None
    except ValueError:
        ra_hours = None
        
    try:
        ra_minutes = float(ra_m) if ra_m else None
    except ValueError:
        ra_minutes = None
        
    try:
        dec_degrees = int(de_d) if de_d else None
    except ValueError:
        dec_degrees = None
        
    try:
        dec_minutes = int(de_m) if de_m else None
    except ValueError:
        dec_minutes = None
        
    try:
        size_val = float(size) if size else None
    except ValueError:
        size_val = None
        
    try:
        mag_val = float(magnitude) if magnitude else None
    except ValueError:
        mag_val = None
    
    # Calculate decimal coordinates
    ra_decimal = None
    dec_decimal = None
    
    if ra_hours is not None and ra_minutes is not None:
        ra_decimal = ra_hours + (ra_minutes / 60.0)
        ra_decimal = ra_decimal * 15.0  # Convert hours to degrees
    
    if dec_degrees is not None and dec_minutes is not None:
        dec_decimal = dec_degrees + (dec_minutes / 60.0)
        if de_sign == '-':
            dec_decimal = -dec_decimal
    
    # Create object entry with proper field names
    obj = {
        "name": name,
        "type": obj_type,
        "ra_deg": ra_decimal,
        "dec_deg": dec_decimal,
        "constellation": constellation,
        "size_arcmin": size_val,
        "magnitude": mag_val
    }
    
    # Only include fields that have values to reduce size
    filtered_obj = {}
    for key, value in obj.items():
        if value is not None and value != "":
            filtered_obj[key] = value
    
    return filtered_obj

def should_include_object(obj, include_types, max_magnitude):
    """Determine if an object should be included based on filters"""
    obj_type = obj.get("type", "")
    magnitude = obj.get("magnitude")
    
    # Check type filter
    if include_types and obj_type not in include_types:
        return False
    
    # Check magnitude filter (default: include only objects with magnitude >= max_magnitude)
    if max_magnitude is not None:
        # Ignore entry if magnitude is missing
        if magnitude is None:
            return False
        if magnitude < max_magnitude:
            return False
    
    return True

def convert_ngc_to_json(input_file, output_file, include_types=None, max_magnitude=None, pretty=True):
    """Convert NGC 2000.0 data file to JSON format"""
    objects = []
    
    with open(input_file, 'r', encoding='utf-8', errors='ignore') as f:
        for line_num, line in enumerate(f, 1):
            try:
                obj = parse_ngc_line(line.rstrip('\n\r'))
                if obj and "name" in obj:  # Only add if name exists
                    if should_include_object(obj, include_types, max_magnitude):
                        objects.append(obj)
            except Exception as e:
                print(f"Error parsing line {line_num}: {e}")
                print(f"Line content: {repr(line)}")
                continue
    
    # Create final JSON structure with enhanced metadata like BSC5
    catalog = {
        "catalog": "NGC 2000.0 - New General Catalogue and Index Catalogue",
        "version": "2000.0",
        "coordinate_system": "J2000",
        "total_objects": len(objects),
        "data_info": {
            "source": "NASA/IPAC Extragalactic Database",
            "description": "New General Catalogue (NGC) and Index Catalogue (IC) of Nebulae and Star Clusters",
            "included_types": include_types if include_types else "all",
            "max_magnitude": max_magnitude,
            "coordinate_epoch": "J2000.0"
        },
        "objects": objects
    }
    
    with open(output_file, 'w', encoding='utf-8') as f:
        # Default to pretty unless compact is explicitly requested
        if args.compact and not args.pretty:
            json.dump(catalog, f, separators=(',', ':'), ensure_ascii=False)
        else:
            json.dump(catalog, f, indent=2, ensure_ascii=False)
    
    print(f"Converted {len(objects)} objects to {output_file}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Convert NGC 2000.0 catalog to JSON format',
        epilog="""
Usage examples:
  python ngc2000_to_json.py                           # Default: Galaxies, Stars, Planetary Nebulae (mag ≤ 15)
  python ngc2000_to_json.py --preset deep-sky         # Deep-sky objects (mag ≤ 15)
  python ngc2000_to_json.py --types Gx OC Gb --max-magnitude 12  # Custom filter
  python ngc2000_to_json.py --compact --preset minimal           # Minimal set, compact format
        """,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument('input_file', nargs='?', default='ngc2000.dat', 
                       help='Input NGC 2000.0 data file (default: ngc2000.dat)')
    parser.add_argument('output_file', nargs='?', default='ngc2000.json',
                       help='Output JSON file (default: ngc2000.json)')
    parser.add_argument('--types', nargs='+', 
                       choices=['Gx', 'OC', 'Gb', 'Nb', 'Pl', 'C+N', 'Ast', 'Kt', '***', 'D*', '*', '?', '-', 'PD'],
                       help='Object types to include (default: all)')
    parser.add_argument('--max-magnitude', type=float,
                       help='Maximum magnitude to include (brighter objects only)')
    parser.add_argument('--compact', action='store_true',
                       help='Generate compact JSON without indentation')
    parser.add_argument('--pretty', action='store_true',
                       help='Generate pretty JSON with indentation (default if neither specified)')
    parser.add_argument('--preset', choices=['all', 'deep-sky', 'bright', 'minimal'],
                       help='Predefined filter presets')
    
    args = parser.parse_args()
    
    # Apply presets and set defaults
    include_types = args.types
    max_magnitude = args.max_magnitude
    
    if args.preset == 'deep-sky':
        include_types = ['Gx', 'OC', 'Gb', 'Nb', 'Pl', 'C+N']
        max_magnitude = 15.0
    elif args.preset == 'bright':
        include_types = ['Gx', 'OC', 'Gb', 'Nb', 'Pl', 'C+N']
        max_magnitude = 12.0
    elif args.preset == 'minimal':
        include_types = ['OC', 'Gb', 'Gx', 'Nb', 'Pl']
        max_magnitude = 10.0
    elif not args.preset and not args.types:
        # Default: include galaxies, single stars, and planetary nebulae
        include_types = ['Gx', '*', 'Pl']
        max_magnitude = 15.0
    
    convert_ngc_to_json(args.input_file, args.output_file, include_types, max_magnitude, 
                       pretty=(args.pretty or not args.compact))
    
    # Print some statistics
    import os
    file_size = os.path.getsize(args.output_file)
    print(f"Output file size: {file_size / 1024:.1f} KB")
    
    if include_types:
        print(f"Included object types: {', '.join(include_types)}")
    else:
        print("Included object types: All types")
    if max_magnitude:
        print(f"Maximum magnitude: {max_magnitude}")
