#include "bsc5ra.h"
#include "uart.h"

#define ntohll(x) ((((uint64_t) ntohl(x)) << 32) + (ntohl((x) >> 32)))

BSC5 bsc5(bsc5_BSC5ra_bsc5_start, bsc5_BSC5ra_bsc5_end);

uint64_t ntoh64(const uint64_t* input)
{
    uint64_t rval;
    uint8_t* data = (uint8_t*) &rval;

    data[0] = *input >> 56;
    data[1] = *input >> 48;
    data[2] = *input >> 40;
    data[3] = *input >> 32;
    data[4] = *input >> 24;
    data[5] = *input >> 16;
    data[6] = *input >> 8;
    data[7] = *input >> 0;

    return rval;
}

// Entry Entry::newFromIndex(int32_t index)
//{
//
// }

BSC5::BSC5(const uint8_t* start, const uint8_t* end) : _start(start), _end(end)
{
}

std::optional<Entry> BSC5::findByXno(float xno)
{
    bsc5_entry_t _entry;
    bsc5_entry_t* entry = &_entry;
    const uint8_t* i = 0;
    char is[3] = {0};

    print_out("Catalog number of star to search: %.0f", xno);

    for (i = _start + sizeof(bsc5_header_t); i < _end; i += sizeof(bsc5_entry_t))
    {
        //		bcopy(_start+sizeof(bsc5_header_t)+i*sizeof(bsc5_entry_t), &_entry,
        // sizeof(bsc5_entry_t));

        bcopy(i, &_entry, sizeof(bsc5_entry_t));

        entry->_xno = ntohl(entry->_xno);

        //		print_out("Search for: %.0f; current: %.0f", xno, entry->xno);

        if (entry->xno == xno)
        {
            {
                uint64_t tmp = entry->_sra0;
                entry->_sra0 = ntoh64(&tmp);
            }
            {
                uint64_t tmp = entry->_sdec0;
                entry->_sdec0 = ntoh64(&tmp);
            }
            entry->mag = ntohs(entry->mag);
            entry->_xrpm = ntohl(entry->_xrpm);
            entry->_xdpm = ntohl(entry->_xdpm);

            is[0] = entry->is[0];
            is[1] = entry->is[1];

            return Entry(entry->xno, entry->sra0, entry->sdec0, is, entry->mag, entry->xrpm,
                         entry->xdpm);
        }
    }
    return {};
}

void Entry::print()
{
    print_out("Catalog number of star: %.0f", this->xno);
    print_out("B1950 Right Ascension (radians): %.20f", this->sra0);
    print_out("B1950 Declination (radians): %.20f", this->sdec0);
    print_out("Spectral type (2 characters): %s", this->is);
    print_out("V Magnitude * 100: %d", this->mag);
    print_out("R.A. proper motion (radians per year): %.20f", this->xrpm);
    print_out("Dec. proper motion (radians per year): %.20f", this->xdpm);
}

void BSC5::printHeader()
{
    bsc5_header_t _header;
    bsc5_header_t* header = &_header;
    bcopy(_start, &_header, sizeof(bsc5_header_t));

    header->star0 = ntohl(header->star0);
    header->star1 = ntohl(header->star1);
    header->starn = abs(static_cast<int32_t>(ntohl(header->starn)));
    header->stnum = ntohl(header->stnum);
    header->mprop = ntohl(header->mprop);
    header->nmag = ntohl(header->nmag);
    header->nbent = ntohl(header->nbent);

    print_out("Subtract from star number to get sequence number: %d", header->star0);
    print_out("First star number in file: %d", header->star1);
    print_out("Number of stars in file: %d", header->starn);
    print_out("i.d. numbers: %d", header->stnum);
    print_out("proper motion is included: %d", header->mprop);
    print_out("Number of magnitudes present (-1=J2000 instead of B1950): %d", header->nmag);
    print_out("Number of bytes per star entry: %d", header->nbent);
}

void BSC5::printStar(int32_t index)
{
    bsc5_entry_t _entry;
    bsc5_entry_t* entry = &_entry;
    bcopy(_start + sizeof(bsc5_header_t) + index * sizeof(bsc5_entry_t), &_entry,
          sizeof(bsc5_entry_t));

    //	print_out_nonl("Hex: ");
    //	for(int i = 0; i<sizeof(bsc5_entry_t); i++) {
    //		print_out_nonl("%2X ", *((char *)entry+i));
    //	}
    //	print_out("");

    entry->_xno = ntohl(entry->_xno);
    {
        uint64_t tmp = entry->_sra0;
        entry->_sra0 = ntoh64(&tmp);
    }
    {
        uint64_t tmp = entry->_sdec0;
        entry->_sdec0 = ntoh64(&tmp);
    }
    entry->mag = ntohs(entry->mag);
    entry->_xrpm = ntohl(entry->_xrpm);
    entry->_xdpm = ntohl(entry->_xdpm);

    //	print_out_nonl("Hex: ");
    //	for(int i = 0; i<sizeof(bsc5_entry_t); i++) {
    //		print_out_nonl("%2X ", *((char *)entry+i));
    //	}
    //	print_out("");

    print_out("Catalog number of star: %.0f", entry->xno);
    print_out("B1950 Right Ascension (radians): %.20f", entry->sra0);
    print_out("B1950 Declination (radians): %.20f", entry->sdec0);
    print_out("Spectral type (2 characters): %c%c", entry->is[0], entry->is[1]);
    print_out("V Magnitude * 100: %d", entry->mag);
    print_out("R.A. proper motion (radians per year): %.20f", entry->xrpm);
    print_out("Dec. proper motion (radians per year): %.20f", entry->xdpm);
}
