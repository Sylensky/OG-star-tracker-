#!/usr/bin/env python3
import json
import struct
import os
import re
import argparse

def parse_bsc5_header(f):
    """Parse the 28-byte BSC5 header according to specification"""
    header_data = f.read(28)
    if len(header_data) < 28:
        raise ValueError(f"Invalid header size: {len(header_data)} bytes")

    # Parse header: 7 big-endian 32-bit integers
    star0, star1, starn, stnum, mprop, nmag, nbent = struct.unpack('>7i', header_data)

    return {
        'star0': star0,      # Subtract from star number to get sequence number
        'star1': star1,      # First star number in file
        'starn': starn,      # Number of stars (negative = J2000, positive = B1950)
        'stnum': stnum,      # Star ID number format
        'mprop': mprop,      # Proper motion inclusion flag
        'nmag': nmag,        # Number of magnitudes (negative = J2000)
        'nbent': nbent,      # Number of bytes per star entry
        'j2000': starn < 0 or nmag < 0,  # Coordinate system
        'star_count': abs(starn),
        'mag_count': abs(nmag)
    }

def parse_bsc5_entry(f, header):
    """Parse a single star entry based on header information"""
    entry_data = f.read(header['nbent'])
    if len(entry_data) < header['nbent']:
        return None

    offset = 0
    entry = {}

    # Parse catalog number (XNO) - optional based on STNUM
    if header['stnum'] >= 0:  # Has catalog numbers
        if header['stnum'] == 4:  # Integer*4
            entry['xno'] = struct.unpack('>i', entry_data[offset:offset+4])[0]
            offset += 4
        else:  # Real*4
            entry['xno'] = struct.unpack('>f', entry_data[offset:offset+4])[0]
            offset += 4

    # Parse coordinates - always Real*8 (8 bytes each)
    entry['sra0'] = struct.unpack('>d', entry_data[offset:offset+8])[0]  # RA in radians
    offset += 8
    entry['sdec0'] = struct.unpack('>d', entry_data[offset:offset+8])[0]  # Dec in radians
    offset += 8

    # Parse spectral type - Character*2
    entry['spectral_type'] = entry_data[offset:offset+2].decode('ascii', errors='replace')
    offset += 2

    # Parse primary magnitude - Integer*2 (first magnitude only)
    if header['mag_count'] > 0 and offset + 2 <= len(entry_data):
        mag = struct.unpack('>h', entry_data[offset:offset+2])[0]  # signed 16-bit
        entry['mag'] = mag / 100.0
        offset += 2
    else:
        entry['mag'] = None

    # Parse proper motion - Real*4 each, if MPROP >= 1
    if header['mprop'] >= 1:
        if offset + 4 <= len(entry_data):
            entry['ra_pm'] = struct.unpack('>f', entry_data[offset:offset+4])[0]  # RA proper motion
            offset += 4
        if offset + 4 <= len(entry_data):
            entry['dec_pm'] = struct.unpack('>f', entry_data[offset:offset+4])[0]  # Dec proper motion
            offset += 4

    # Parse radial velocity - Real*8, if MPROP == 2
    if header['mprop'] == 2:
        if offset + 8 <= len(entry_data):
            entry['radial_velocity'] = struct.unpack('>d', entry_data[offset:offset+8])[0]
            offset += 8

    # Parse object name - Character*(-STNUM), if STNUM < 0
    if header['stnum'] < 0:
        name_length = -header['stnum']
        if offset + name_length <= len(entry_data):
            entry['object_name'] = entry_data[offset:offset+name_length].decode('ascii', errors='replace').strip()
            offset += name_length

    return entry

def parse_notes(notes_path):
    """Parse the ybsc5.notes file"""
    notes = {}
    if not os.path.exists(notes_path):
        return notes
  
    note_re = re.compile(r"^\s*(\d+) 1N: (.+)$")
    with open(notes_path, encoding="utf-8") as f:
        for line in f:
            m = note_re.match(line)
            if m:
                star_id = int(m.group(1))
                desc = m.group(2).strip()
                notes[star_id] = desc
    return notes

def parse_names(names_path):
    """Parse the ybsc5.names file"""
    names = {}
    if not os.path.exists(names_path):
        return names

    name_re = re.compile(r"^\s*(\d+) ")
    with open(names_path, encoding="utf-8") as f:
        for line in f:
            m = name_re.match(line)
            if m:
                star_id = int(m.group(1))
                if star_id not in names:
                    names[star_id] = []
                names[star_id].append(line.strip())
    return names

def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Convert BSC5 catalog to JSON or binary format')
    parser.add_argument('--binary', action='store_true',
                       help='Write output as binary file (default: JSON)')
    parser.add_argument('--compact', action='store_true',
                       help='Generate compact format (reduced fields)')
    args = parser.parse_args()

    # Determine base directory
    base = os.environ.get('PROJECT_DIR')
    if base is None:
        base = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'bsc5'))
    else:
        base = os.path.join(base, 'bsc5')

    # Parse notes and names
    notes = parse_notes(os.path.join(base, "sources/ybsc5.notes"))
    names = parse_names(os.path.join(base, "sources/ybsc5.names"))

    # Parse binary catalog
    bsc5_path = os.path.join(base, "sources/BSC5ra.bsc5")
    if not os.path.exists(bsc5_path):
        print(f"Error: {bsc5_path} not found")
        return

    with open(bsc5_path, "rb") as f:
        header = parse_bsc5_header(f)

        print(f"BSC5 Header Info:")
        print(f"  Stars: {header['star_count']}")
        print(f"  Coordinate system: {'J2000' if header['j2000'] else 'B1950'}")
        print(f"  Bytes per entry: {header['nbent']}")
        print(f"  Magnitudes per star: {header['mag_count']}")
        print(f"  Proper motion: {header['mprop']}")
        print(f"  Star ID format: {header['stnum']}")

        # Parse all entries
        entries = []
        for i in range(header['star_count']):
            entry = parse_bsc5_entry(f, header)
            if entry:
                entries.append(entry)
            else:
                print(f"Warning: Could not parse entry {i+1}")
                break

    print(f"Parsed {len(entries)} star entries")

    # Build catalog lookup
    catalog_lookup = {}
    for i, entry in enumerate(entries):
        # Use catalog number if available, otherwise use sequence number
        if 'xno' in entry:
            star_num = int(round(entry['xno']))
        else:
            star_num = header['star1'] + i  # Calculate from sequence

        catalog_lookup[star_num] = entry
        entry['computed_star_id'] = star_num

    # Create stars with notes and names
    stars = []
    all_indices = set(notes.keys()) | set(names.keys())

    for idx in sorted(all_indices):
        if idx not in catalog_lookup:
            # Try to find closest match
            closest_entry = None
            min_diff = float('inf')
            for star_id, entry in catalog_lookup.items():
                diff = abs(star_id - idx)
                if diff < min_diff and diff < 2:  # Allow small differences
                    min_diff = diff
                    closest_entry = entry

            if not closest_entry:
                continue
            star_entry = closest_entry
        else:
            star_entry = catalog_lookup[idx]

        # Extract name from names data
        name_lines = names.get(idx, [])
        name_entry = None
        other_notes = []

        for line in name_lines:
            if '1N:' in line:
                name_candidate = line.split('1N:', 1)[1].strip()
                if not name_candidate.startswith('See HR'):
                    name_entry = name_candidate
            else:
                other_notes.append(line)

        if not name_entry:
            continue  # Skip stars without valid names

        # Clip name at first semicolon, period, or comma and remove trailing dots (for all formats)
        # Find first occurrence of ; or . or ,
        clip_pos = len(name_entry)
        semicolon_pos = name_entry.find(';')
        period_pos = name_entry.find('.')
        comma_pos = name_entry.find(',')
        
        if semicolon_pos != -1:
            clip_pos = min(clip_pos, semicolon_pos)
        if period_pos != -1:
            clip_pos = min(clip_pos, period_pos)
        if comma_pos != -1:
            clip_pos = min(clip_pos, comma_pos)

        # Clip and remove trailing whitespace and dots
        original_name = name_entry
        name_entry = name_entry[:clip_pos].rstrip(' .')

        # Debug: print when name is clipped significantly (for both formats)
        if len(original_name) > len(name_entry) + 10:
            format_type = "compact" if args.compact else "full"

        # Apply format-specific length limits
        if args.compact:
            # Drop objects with names that still exceed 32 characters after clipping
            if len(name_entry) > 32:
                print(f"Dropping star {idx}: name too long after clipping: '{name_entry}' ({len(name_entry)} chars)")
                continue

        # For full format, also check name length limit (255 characters)
        elif len(name_entry) > 255:
            print(f"Dropping star {idx}: name too long for full format: '{name_entry[:50]}...' ({len(name_entry)} chars)")
            continue

        # Combine notes
        note = notes.get(idx)
        combined_notes = []
        if note:
            combined_notes.append(note)
        if other_notes:
            combined_notes.extend(other_notes)

        # Create star record
        star = {
            "xno": idx,
            "sra0": star_entry['sra0'],
            "sdec0": star_entry['sdec0'],
            "mag": star_entry.get('mag', 0),
            "name": name_entry,
        }

        # Add additional fields for full format only
        if not args.compact:
            star["spectral_type"] = star_entry.get('spectral_type', '')
            star["coordinate_system"] = "J2000" if header['j2000'] else "B1950"
            
            # Add proper motion if available
            if 'ra_pm' in star_entry:
                star['ra_pm'] = star_entry['ra_pm']
            if 'dec_pm' in star_entry:
                star['dec_pm'] = star_entry['dec_pm']
            if 'radial_velocity' in star_entry:
                star['radial_velocity'] = star_entry['radial_velocity']
            
            # Add notes if available
            if combined_notes:
                star['notes'] = combined_notes

        stars.append(star)

    # Output files based on format

    if args.binary:
        suffix = "_compact" if args.compact else ""
        out_path = os.path.join(base, f"converted/bsc5ra{suffix}.bin")
        def write_binary_format(stars, output_path, compact_format):
            with open(output_path, 'wb') as f:
                # Write header: number of stars (uint32), magic, version
                f.write(struct.pack('<I', len(stars)))
                if compact_format:
                    f.write(b'BSC5RC')  # BSC5 Revised Compact
                else:
                    f.write(b'BSC5RA')  # BSC5 Revised All
                f.write(struct.pack('<I', 1))
                
                for star in stars:
                    # RA, Dec (float64)
                    f.write(struct.pack('<d', star['sra0']))
                    f.write(struct.pack('<d', star['sdec0']))
                    # Magnitude (float32)
                    f.write(struct.pack('<f', float(star.get('mag', 0.0))))
                    
                    if compact_format:
                        # Compact: only name (length-prefixed string, max 32)
                        name_bytes = star.get('name', '').encode('utf-8')
                        f.write(struct.pack('<B', min(len(name_bytes), 32)))
                        f.write(name_bytes[:32])
                    else:
                        # Full format: spectral type, name (no notes in binary)
                        # Spectral type (2 bytes, padded)
                        spec = star.get('spectral_type', '')[:2].ljust(2, '\0')
                        f.write(spec.encode('ascii'))
                        # Name (length-prefixed string, max 255)
                        name_bytes = star.get('name', '').encode('utf-8')
                        f.write(struct.pack('<B', min(len(name_bytes), 255)))
                        f.write(name_bytes[:255])
        write_binary_format(stars, out_path, args.compact)
        file_size = os.path.getsize(out_path)
        print(f"Successfully wrote {len(stars)} stars to {out_path}")
        print(f"Output format: binary, file size: {file_size:,} bytes")
    else:
        suffix = "_compact" if args.compact else ""
        out_path = os.path.join(base, f"converted/bsc5ra{suffix}.json")
        
        # Both compact and full formats output just the stars array (no metadata wrapper)
        output_data = stars
        
        with open(out_path, "w", encoding="utf-8") as out:
            # Always use pretty formatting (no --pretty flag needed)
            json.dump(output_data, out, ensure_ascii=False, indent=2)
        
        file_size = os.path.getsize(out_path)
        format_type = "compact" if args.compact else "full"
        print(f"Successfully wrote {len(stars)} stars to {out_path}")
        print(f"Output format: {format_type}, file size: {file_size:,} bytes")

if __name__ == "__main__":
    main()
