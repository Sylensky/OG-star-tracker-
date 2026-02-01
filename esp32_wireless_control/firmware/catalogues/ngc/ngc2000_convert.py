#!/usr/bin/env python3
"""
Enhanced NGC 2000.0 to JSON/Binary converter with compact format support

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
import struct
import os
import argparse

def parse_ngc_line(line):
    """Parse a single line from the NGC 2000.0 data file"""
    if len(line) < 99:
        line = line.ljust(99)  # Pad line if it's too short

    # Extract fields according to byte positions (adjust for 0-based indexing)
    # Parse ID and preserve NGC/IC prefix if present
    raw_id = line[0:5].strip()
    # Remove all spaces from the ID for consistency (e.g., 'NGC 104' -> 'NGC104')
    raw_id_nospaces = raw_id.replace(' ', '')
    if raw_id_nospaces.startswith('I'):
        # IC object, remove spaces only
        id = 'IC' + raw_id_nospaces[1:]
    elif raw_id_nospaces.isdigit():
        id = 'NGC' + raw_id_nospaces
    elif raw_id_nospaces.startswith('NGC'):
        id = raw_id_nospaces
    else:
        id = raw_id_nospaces
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
    ra = None
    dec = None

    if ra_hours is not None and ra_minutes is not None:
        ra = ra_hours + (ra_minutes / 60.0)

    if dec_degrees is not None and dec_minutes is not None:
        dec = dec_degrees + (dec_minutes / 60.0)
        if de_sign == '-':
            dec = -dec

    return {
        "id": id,
        "type": obj_type,
        "ra": ra,
        "dec": dec,
        "constellation": constellation,
        "size_arcmin": size_val,
        "magnitude": mag_val,
        "description": description
    }

def write_binary_format(objects, output_path, compact_format):
    """Write NGC objects in custom binary format"""
    with open(output_path, 'wb') as f:
        # Write header
        f.write(struct.pack('<I', len(objects)))  # Number of objects
        f.write(b'NGC2' if not compact_format else b'NC2C')  # Magic number
        f.write(struct.pack('<I', 1))  # Version

        # Write object entries
        for obj in objects:
            if compact_format:
                # Compact binary format: id (12s), type (4s), ra_deg (f), dec_deg (f), magnitude (f)
                id_bytes = obj['id'][:11].ljust(11, '\0').encode('ascii')
                id_bytes += b'\0'  # Ensure 12 bytes
                f.write(id_bytes)
                type_bytes = obj['type'][:3].ljust(3, '\0').encode('ascii')
                type_bytes += b'\0'  # Ensure 4 bytes
                f.write(type_bytes)
                f.write(struct.pack('<f', float(obj['ra'] or 0)))
                f.write(struct.pack('<f', float(obj['dec'] or 0)))
                f.write(struct.pack('<f', float(obj.get('magnitude', 0) or 0)))
                # Omit description, constellation, size_arcmin
            else:
                # Full binary format: id (12s), type (4s), ra_deg (f), dec_deg (f), constellation (3s), size_arcmin (f), magnitude (f), description (48s)
                id_bytes = obj['id'][:11].ljust(11, '\0').encode('ascii')
                id_bytes += b'\0'  # Ensure 12 bytes
                f.write(id_bytes)
                type_bytes = obj['type'][:3].ljust(3, '\0').encode('ascii')
                type_bytes += b'\0'  # Ensure 4 bytes
                f.write(type_bytes)
                f.write(struct.pack('<f', float(obj['ra'] or 0)))
                f.write(struct.pack('<f', float(obj['dec'] or 0)))
                const = obj.get('constellation', '')[:3].ljust(3, '\0').encode('ascii')
                f.write(const)
                f.write(struct.pack('<f', float(obj.get('size_arcmin', 0) or 0)))
                f.write(struct.pack('<f', float(obj.get('magnitude', 0) or 0)))
                desc = obj.get('description', '')
                desc_bytes = desc.encode('utf-8')[:47]
                desc_bytes += b'\0' * (48 - len(desc_bytes))
                f.write(desc_bytes)

def write_json_format(objects, output_path, compact_format):
    """Write NGC objects in JSON format with only the fields present in the binary format."""
    filtered = []
    for obj in objects:
        if compact_format:
            filtered.append({
                "id": obj.get('id', ''),
                "type": obj.get('type', ''),
                "ra": obj.get('ra', 0),
                "dec": obj.get('dec', 0),
                "magnitude": obj.get('magnitude', 0),
            })
        else:
            filtered.append({
                "id": obj.get('id', ''),
                "type": obj.get('type', ''),
                "ra": obj.get('ra', 0),
                "dec": obj.get('dec', 0),
                "constellation": obj.get('constellation', ''),
                "size_arcmin": obj.get('size_arcmin', 0),
                "magnitude": obj.get('magnitude', 0),
                "description": obj.get('description', ''),
            })
    with open(output_path, "w", encoding="utf-8") as out:
        json.dump(filtered, out, ensure_ascii=False, indent=2)

def should_include_object(obj, include_types, max_magnitude, min_magnitude):
    """Determine if an object should be included based on filters"""
    obj_type = obj.get("type", "")
    magnitude = obj.get("magnitude")

    # Check type filter
    if include_types and obj_type not in include_types:
        return False

    # Always include nebulae even if magnitude is missing
    nebula_keywords = ["NB", "PL", "C+N"]
    type_norm = obj_type.strip().upper()
    if magnitude is None:
        if any(keyword in type_norm for keyword in nebula_keywords):
            return True
        else:
            return False

    if max_magnitude is not None and magnitude > max_magnitude:
        return False
    if min_magnitude is not None and magnitude < min_magnitude:
        return False

    return True

def main():
    parser = argparse.ArgumentParser(description='Convert NGC 2000.0 catalog to JSON/Binary format')
    parser.add_argument('--input', default='sources/ngc2000.dat', 
                       help='Input NGC 2000.0 data file (default: sources/ngc2000.dat)')
    parser.add_argument('--compact', action='store_true', 
                       help='Generate compact format (reduced field names and quantized data)')
    parser.add_argument('--binary', action='store_true',
                       help='Generate binary format instead of JSON')
    parser.add_argument('--types', nargs='*', 
                       help='Object types to include (e.g., Gx OC Gb Pl)')
    parser.add_argument('--max-magnitude', type=float,
                       help='Maximum magnitude to include (brighter objects)')
    parser.add_argument('--min-magnitude', type=float,
                       help='Minimum magnitude to include (fainter objects)')
    parser.add_argument('--magnitude-bins', type=int, default=10,
                       help='Number of magnitude bins for compact format (default: 10)')
    parser.add_argument('--max-desc-length', type=int, default=40,
                       help='Maximum description length in compact mode (default: 40)')

    # Preset configurations
    parser.add_argument('--preset', choices=['minimal', 'deep-sky', 'bright'], 
                       help='Use preset configuration')

    args = parser.parse_args()

    if args.preset == 'minimal':
        args.types = ['Gx', 'OC', 'Gb', 'Pl', 'Nb', '*']
        args.max_magnitude = 8.0
        args.min_magnitude = 2.0
        if not args.compact:
            args.compact = False
    elif args.preset == 'deep-sky':
        args.types = ['Gx', 'Nb', 'Pl', 'Gb']
        args.max_magnitude = 15.0
        args.min_magnitude = 3.0
        if not args.compact:
            args.compact = False
    elif args.preset == 'bright':
        args.max_magnitude = 6.0
        args.min_magnitude = 0.1
        if not args.compact:
            args.compact = False

    # If user provides min_magnitude > max_magnitude, swap them and warn
    if args.max_magnitude is not None and args.min_magnitude is not None:
        if args.min_magnitude > args.max_magnitude:
            print(f"Warning: min_magnitude ({args.min_magnitude}) > max_magnitude ({args.max_magnitude}), swapping values.")
            args.min_magnitude, args.max_magnitude = args.max_magnitude, args.min_magnitude

    base = os.path.dirname(os.path.abspath(__file__))
    input_path = os.path.join(base, args.input)

    if not os.path.exists(input_path):
        print(f"Error: Input file not found: {input_path}")
        return

    print(f"Converting NGC 2000.0 catalog from {input_path}")
    if args.types:
        print(f"Including object types: {', '.join(args.types)}")
    if args.max_magnitude:
        print(f"Maximum magnitude: {args.max_magnitude}")
    if args.min_magnitude:
        print(f"Minimum magnitude: {args.min_magnitude}")

    objects = []

    with open(input_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line_num, line in enumerate(f, 1):
            try:
                obj = parse_ngc_line(line)
                if obj is None:
                    continue
                # Apply filters
                if not should_include_object(obj, args.types, args.max_magnitude, args.min_magnitude):
                    continue
                # Skip objects with missing essential data
                if not obj['id'] or obj.get('ra') is None or obj.get('dec') is None:
                    continue
                # Remove None values to reduce size
                obj = {k: v for k, v in obj.items() if v is not None}
                objects.append(obj)

            except Exception as e:
                print(f"Warning: Error parsing line {line_num}: {e}")
                continue

    if args.binary:
        suffix = "_compact" if args.compact else ""
        out_path = os.path.join(base, f"converted/ngc2000{suffix}.bin")
        write_binary_format(objects, out_path, args.compact)
    else:
        suffix = "_compact" if args.compact else ""
        out_path = os.path.join(base, f"converted/ngc2000{suffix}.json")
        write_json_format(objects, out_path, args.compact)
    
    print(f"Generated {len(objects)} objects in {out_path}")

    if os.path.exists(out_path):
        size = os.path.getsize(out_path)
        if len(objects) > 0:
            print(f"File size: {size} bytes ({size/len(objects):.1f} bytes per object)")
        else:
            print(f"File size: {size} bytes (no objects parsed)")

if __name__ == "__main__":
    main()
