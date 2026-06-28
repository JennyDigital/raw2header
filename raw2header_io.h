#ifndef RAW2HEADER_IO_H
#define RAW2HEADER_IO_H

#include <stdint.h>
#include <sys/types.h>

// Error Codes
#define INVALID_FN          -99
#define ARGUMENTS_ERROR     -98
#define ERROR_NOT_OPEN      -97
#define NO_MALLOC           -96
#define EMPTY_FILE          -95
#define WRITE_SUCCESS       -94
#define READ_SUCCESS        -93
#define FILE_NOT_FOUND      -92

// Channel mode constants
#define MODE_NONE           0
#define MODE_MONO           1
#define MODE_STEREO         2

extern int8_t* rawdata_p;
extern off_t table_size;
extern uint8_t wordmode;
extern uint8_t bigendian;
extern uint8_t channelmode;
extern uint8_t pad_enabled;
extern uint8_t pad_value;
extern uint8_t adpcm_enabled;
extern uint8_t sourcepair_enabled;
extern char g_generated_with[256];

off_t getFileSize( char* file_to_size );
int getRaw( char* input_file );
int writeFile( char* output_file, char* varname );
int writeFile16( char* output_file, char* varname );
void printSystemError( const char* context, const char* path );

#endif
