#!/usr/bin/env python3
import json
import struct
import os
import re

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
    entry['is'] = entry_data[offset:offset+2].decode('ascii', errors='replace')
    offset += 2
    
    # Parse magnitudes - Integer*2 each, count from NMAG
    magnitudes = []
    for i in range(header['mag_count']):
        if offset + 2 <= len(entry_data):
            mag = struct.unpack('>h', entry_data[offset:offset+2])[0]  # signed 16-bit
            magnitudes.append(mag)  # magnitude * 100
            offset += 2
    
    # Use first magnitude as primary magnitude for compatibility
    entry['mag'] = magnitudes[0] if magnitudes else 0
    
    # Parse proper motion - Real*4 each, if MPROP >= 1
    if header['mprop'] >= 1:
        if offset + 4 <= len(entry_data):
            entry['xrpm'] = struct.unpack('>f', entry_data[offset:offset+4])[0]  # RA proper motion
            offset += 4
        else:
            entry['xrpm'] = 0.0
        if offset + 4 <= len(entry_data):
            entry['xdpm'] = struct.unpack('>f', entry_data[offset:offset+4])[0]  # Dec proper motion
            offset += 4
        else:
            entry['xdpm'] = 0.0
    else:
        entry['xrpm'] = 0.0
        entry['xdpm'] = 0.0
    
    return entry

def parse_notes(notes_path):
    notes = {}
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
    names = {}
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
    # Parse notes and names
    import os
    # Use PlatformIO PROJECT_DIR if available, else fallback to relative
    base = os.environ.get('PROJECT_DIR')
    if base is None:
        base = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'bsc5'))
    else:
        base = os.path.join(base, 'bsc5')
    notes = parse_notes(os.path.join(base, "ybsc5.notes"))
    names = parse_names(os.path.join(base, "ybsc5.names"))

    # Parse binary catalog using BSC5ra format
    bsc5_path = os.path.join(base, "BSC5ra.bsc5")
    if not os.path.exists(bsc5_path):
        print(f"Error: {bsc5_path} not found")
        return

    with open(bsc5_path, "rb") as f:
        h = parse_bsc5_header(f)
        
        print(f"BSC5 Header Info:")
        print(f"  Stars: {h['star_count']}")
        print(f"  Coordinate system: {'J2000' if h['j2000'] else 'B1950'}")
        print(f"  Bytes per entry: {h['nbent']}")
        print(f"  Magnitudes per star: {h['mag_count']}")
        
        # Parse all entries
        entries = []
        for i in range(h['star_count']):
            entry = parse_bsc5_entry(f, h)
            if entry:
                entries.append(entry)

    # Build a lookup for all catalog entries by int(xno) and by close float match
    catalog_lookup = {}
    for i, e in enumerate(entries):
        # Use catalog number if available, otherwise calculate from sequence
        if 'xno' in e:
            int_xno = int(round(e['xno']))
        else:
            int_xno = h['star1'] + i
            e['xno'] = float(int_xno)
        
        if int_xno not in catalog_lookup:
            catalog_lookup[int_xno] = []
        catalog_lookup[int_xno].append(e)

    stars = []
    # Use all indices from notes and names
    all_indices = set(notes.keys()) | set(names.keys())
    for idx in sorted(all_indices):
        # Find the closest matching entry in the catalog
        star_entry = None
        if idx in catalog_lookup:
            # Prefer exact int match
            star_entry = catalog_lookup[idx][0]
        else:
            # Try to find a close float match (within 0.5)
            for e in entries:
                if 'xno' in e and abs(e['xno'] - idx) < 0.5:
                    star_entry = e
                    break
        if not star_entry:
            continue  # skip if no matching entry in catalog
        is_str = star_entry["is"] if isinstance(star_entry["is"], str) else star_entry["is"].decode("ascii", errors="replace")
        name_lines = names.get(idx, [])
        # Find the 1N entry (star name)
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
            continue  # skip stars without a valid 1N entry (not just 'See HR')
        # Compose the note: combine ybsc5.notes and other name lines (if any)
        note = notes.get(idx)
        combined_notes = []
        if note:
            combined_notes.append(note)
        if other_notes:
            combined_notes.extend(other_notes)
        star = {
            "xno": idx,  # use the strict index from notes/names
            "sra0": star_entry['sra0'],
            "sdec0": star_entry['sdec0'],
            "is": is_str,
            "mag": star_entry['mag'],
            "xrpm": star_entry['xrpm'],
            "xdpm": star_entry['xdpm'],
            "name": name_entry,
            "notes": combined_notes if combined_notes else None
        }
        stars.append(star)

    out_path = os.path.join(base, "bsc5_stars_with_notes.json")
    with open(out_path, "w", encoding="utf-8") as out:
        json.dump(stars, out, ensure_ascii=False, indent=2)
    print(f"Inserted {len(stars)} stars into {out_path}")

if __name__ == "__main__":
    main()
