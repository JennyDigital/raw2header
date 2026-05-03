# raw2header

### Takes in any file, spits out a header file.

Current version: V3.00.1

Why did I create this... It is because I wanted a tool to help me with embedding data (mostly sound samples) into my projects FLASH in a way that suited my needs.  I don't know if there is an equivalent tool but it was fun to produce and that was excuse enough.  I didn't take the project too seriously and as such it did need some cleanup, but it is doing what it needs to and I will revisit it as and when the interest takes hold of me.

The first parameter can be --mono/-m or --stereo/-s to emit a mode define in the output header.
Then you can use -16 to generate a 16 bit per entry little-endian header file or -b16 for the same in big-endian.
Then you specify the input filename, the output filename, and lastly the variable name you want in your header file.

Usage:
- raw2header [--mono|-m|--stereo|-s] [-16|-b16] [--adpcm|-a|-a16|-ab16] [--source-pair|--split-c|-c] <input_file> <output_file> <varname>

Source pair mode:
- `--source-pair` (aliases: `--split-c`, `-c`) writes declarations to `<output_file>` and writes the array definition to a paired `.c` file derived from the same path.
- Example: output path `audio_data.h` generates `audio_data.h` + `audio_data.c`.

For ADPCM output (--adpcm/-a), the generated array is always uint8_t. In this mode, -16 and -b16 select 16-bit PCM input endianness.
ADPCM now supports both --mono/-m and --stereo/-s input modes.
Stereo input is expected to be interleaved frames (L, R, L, R, ...).

For convenience, one-step flags are also supported:
- -a16 / --adpcm16: ADPCM output with 16-bit little-endian PCM input
- -ab16 / --adpcm16be: ADPCM output with 16-bit big-endian PCM input

Examples:
- 8-bit PCM mono to ADPCM: `raw2header -a -m input.raw output.h sample`
- 8-bit PCM stereo (interleaved) to ADPCM: `raw2header -a -s input.raw output.h sample`
- 16-bit PCM little-endian mono to ADPCM: `raw2header -a16 -m input.raw output.h sample`
- 16-bit PCM big-endian stereo to ADPCM: `raw2header -ab16 -s input.raw output.h sample`
- Emit declaration/header plus separate source definition: `raw2header -c input.raw sample_data.h sample_data`

Switch combination notes:
- Combined short single-letter flags are supported (for example `-am`, `-ma`, `-as`, `-sa`, `-ac`).
- Mixed compact forms with numeric flags are not supported (for example `-a16s`); use separate tokens like `-a16 -s`.

Take note that if you have an odd number of bytes, it'll complain.  I am considering it offering a 0 to even up the data as an option.

## Build (Portable: Linux + macOS)

This project now uses an out-of-source CMake preset so build files are not tied to one machine's absolute paths.

Requirements:
- CMake 3.22+
- A C compiler (clang or gcc)

Configure and build:
- `cmake --preset dev`
- `cmake --build --preset dev`

Run tests:
- `ctest --preset dev`

Install to `$HOME/.local` (default):
- `cmake --build --preset dev --target install`

If you previously built with a different CMake path or generator, remove stale outputs and reconfigure:
- `rm -rf build CMakeFiles CMakeCache.txt cmake_install.cmake CTestTestfile.cmake install_manifest.txt Makefile`

This builds on mac, or Linux for certain.  It should build on a Windows PC, but I don't have Windows on *any* of my computers.
