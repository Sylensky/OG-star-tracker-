# NGC 2000.0 Data and Conversion

## Source
- The NGC 2000.0 catalogue is available from here:
- https://cdsarc.cds.unistra.fr/ftp/cats/VII/118/
- Download the `ngc2000.dat` file from the CDS archive.

## Conversion Script
- Use the provided Python script to convert the NGC data to JSON:
  ```sh
  python ngc2000_to_json.py --compact --preset minimal
  ```
- You can filter by object type and magnitude using command-line options. For example:
  ```sh
  python ngc2000_to_json.py --types Gx Pl '*' --max-magnitude 15
  python ngc2000_to_json.py --preset deep-sky
  ```
- Run `python ngc2000_to_json.py -h` for all options and usage examples.
