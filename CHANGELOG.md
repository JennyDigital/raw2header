# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.01.1] - 2026-03-13

### Security
- Fixed buffer overflow vulnerability in `writeFile()` and `writeFile16()` functions
- Replaced unsafe `strcpy()` calls with bounds-checked `strncpy()` to prevent buffer overflow when variable names exceed 254 characters
- Added explicit null termination to ensure strings are properly terminated

### Changed
- Variable name buffer now safely handles input strings up to 254 characters with proper bounds checking

## [2.01.0] - 2024

### Added
- Initial release of raw2header file conversion utility
- Support for converting raw binary files to C header files
- 8-bit (`uint8_t`) array generation
- 16-bit (`uint16_t`) array generation with little-endian (`-16`) and big-endian (`-b16`) options
- Mono (`--mono`/`-m`) and stereo (`--stereo`/`-s`) mode defines for audio data
- Padding support (`--pad=NN`) for odd-sized files when generating 16-bit arrays
- Combined short flag support (e.g., `-ms` for mono+stereo)
- Configurable number of columns in output (8 elements per line)
- Header guard generation based on variable name
- Size macro generation (`_SZ`) for array length

### Features
- Command-line argument parsing with multiple flag formats
- Input validation and comprehensive error handling
- System error reporting with context using `errno` and `strerror()`
- Memory management with proper cleanup on error paths
- File operation error checking (open, read, write, seek, tell, close)
