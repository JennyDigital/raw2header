#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>

// Error Codes
//
#define INVALID_FN          -99
#define ARGUMENTS_ERROR     -98
#define ERROR_NOT_OPEN      -97
#define NO_MALLOC           -96
#define EMPTY_FILE          -95
#define WRITE_SUCCESS       -94
#define READ_SUCCESS        -93
#define FILE_NOT_FOUND      -92


// Configuration constants
//
#define NUM_COLUMNS         8

// Channel mode constants
//
#define MODE_NONE           0
#define MODE_MONO           1
#define MODE_STEREO         2

// Private variables
//
int8_t *  rawdata_p;
off_t     table_size;
FILE *    outputfile_p;
FILE *    rawfile_p;
uint8_t   wordmode          = 0;
uint8_t   bigendian         = 0;
uint8_t   channelmode       = MODE_NONE;
uint8_t   pad_enabled       = 0;
uint8_t   pad_value         = 0;

// Private function declarations
//
int   getRaw      ( char* input_file );
int   writeFile   ( char* output_file, char* varname );
off_t getFileSize ( char* input_file );
void  printUsage  ( void );
int   parseArgs   ( int argc, char** argv, char** input, char** output, char** varname );
void  printSystemError( const char* context, const char* path );
int   parseCombinedShortFlags( const char* arg );
int   parsePadFlag( const char* arg );


/** Get the size of the named file
  *
  * @param file_to_size
  * @retval off_t file size.  -1 if invalid in any way
  *
  */
off_t getFileSize ( char* file_to_size )
{
  long int fsize;

  rawfile_p = fopen( file_to_size, "rb" );
  if (rawfile_p == NULL)
  { 
    printSystemError( "open input file", file_to_size );
    return FILE_NOT_FOUND; 
  } 

  if( fseek( rawfile_p, 0L, SEEK_END ) != 0 )
  {
    printSystemError( "seek input file", file_to_size );
    fclose( rawfile_p );
    return ERROR_NOT_OPEN;
  }
  fsize =  ftell( rawfile_p );
  if( fsize < 0 )
  {
    printSystemError( "tell input file size", file_to_size );
    fclose( rawfile_p );
    return ERROR_NOT_OPEN;
  }

  if( fclose( rawfile_p ) != 0 )
  {
    printSystemError( "close input file", file_to_size );
    return ERROR_NOT_OPEN;
  }

  return fsize;
}


/** Write a file given the filename passed containing the specified varname as a header.
 *
 * @param char* output_file
 * @retval int status
 */
int writeFile( char* output_file, char* varname )
{
  char* st_p;

  char outp_header_fname[255] = {0},
        outp_header_name[255] = {0};

  strcpy( outp_header_name, varname );

  st_p  = (char*) outp_header_name;

  while( *st_p != 0 )
  {
    *st_p = toupper(*st_p);// & 0xDF;
    st_p++;
  }

  // Uppercase name is used for header guards and macro prefixes.

  printf( "OF: %s\n", output_file );

  outputfile_p = fopen( output_file, "w" );

  if( outputfile_p == 0 )
  {
    printSystemError( "open output file", output_file );
    return ERROR_NOT_OPEN;
  }

  fprintf( outputfile_p, "#ifndef _%s_H\n", outp_header_name );
  fprintf( outputfile_p, "#define _%s_H\n\n", outp_header_name );
  fprintf( outputfile_p, "#include <stdint.h>\n\n" );
  if( channelmode != MODE_NONE )
  {
    // Optional mode macro for downstream selection.
    fprintf( outputfile_p, "#define %s_PB_FMT Mode_%s\n", outp_header_name,
             ( channelmode == MODE_MONO ) ? "mono" : "stereo" );
  }
  fprintf( outputfile_p, "#define %s_SZ %li\n\n",outp_header_name, table_size );
  fprintf( outputfile_p, "const uint8_t %s[ %s_SZ ] =\n{\n", varname, outp_header_name );

  for( long int element = 0; element < table_size; element++ )
  {
    // Provide indentation at the start of the line.
    //
    if( element % NUM_COLUMNS == 0 )
    {
      fprintf( outputfile_p, " " );
    }

    // print element
    //
    fprintf( outputfile_p, " 0x%02X", (uint8_t) rawdata_p[ element ] );

    if( element < ( table_size-1 ) )
    {
      fprintf( outputfile_p, ",");
    }

    // If we are at 8 elements in then start a new line
    //
    if( element % NUM_COLUMNS == NUM_COLUMNS-1 )
    {
      fprintf( outputfile_p, "\n" );
    }

    if( element == ( table_size-1 ) )
    {
      fprintf( outputfile_p, "\n};\n\n" );
      fprintf( outputfile_p, "#endif // End of _%s_H\n",outp_header_name );
    }
  }
  printf( "Size of output file: %li\n", ftell( outputfile_p ) );
  if( ferror( outputfile_p ) != 0 )
  {
    printSystemError( "write output file", output_file );
    fclose( outputfile_p );
    return ERROR_NOT_OPEN;
  }

  if( fclose( outputfile_p ) != 0 )
  {
    printSystemError( "close output file", output_file );
    return ERROR_NOT_OPEN;
  }
  
  return WRITE_SUCCESS;
}


/** Write a file given the filename passed containing the specified varname as a header
 *  as a 16 bit array
 *
 * @param char* output_file
 * @retval int status
 */
int writeFile16( char* output_file, char* varname )
{
  char* st_p;

  char outp_header_fname[255] = {0},
        outp_header_name[255] = {0};

  strcpy( outp_header_name, varname );

  st_p  = (char*) outp_header_name;

  while( *st_p != 0 )
  {
    *st_p = toupper(*st_p);// & 0xDF;
    st_p++;
  }

  // Uppercase name is used for header guards and macro prefixes.

  printf( "OF: %s\n", output_file );

  outputfile_p = fopen( output_file, "w" );

  if( outputfile_p == 0 )
  {
    printSystemError( "open output file", output_file );
    return ERROR_NOT_OPEN;
  }

  fprintf( outputfile_p, "#ifndef _%s_H\n", outp_header_name );
  fprintf( outputfile_p, "#define _%s_H\n\n", outp_header_name );
  fprintf( outputfile_p, "#define %s_",outp_header_name );
  if( bigendian == 1 )
  {
    // Select output byte order for uint16_t values.
    fprintf( outputfile_p, "BIG_ENDIAN\n" );
  }
  else
  {
    // Select output byte order for uint16_t values.
    fprintf( outputfile_p, "LITTLE_ENDIAN\n" );
  }
  fprintf( outputfile_p, "#include <stdint.h>\n\n" );
  if( channelmode != MODE_NONE )
  {
    // Optional mode macro for downstream selection.
    fprintf( outputfile_p, "#define %s_PB_FMT Mode_%s\n", outp_header_name,
             ( channelmode == MODE_MONO ) ? "mono" : "stereo" );
  }
  fprintf( outputfile_p, "#define %s_SZ %li\n\n",outp_header_name, table_size / 2 );
  fprintf( outputfile_p, "const uint16_t %s[ %s_SZ ] =\n{\n", varname, outp_header_name );

  for( long int element = 0; element < table_size; element+=2 )
  {
    // Provide indentation at the start of the line.
    //
    if( element % NUM_COLUMNS == 0 )
    {
      fprintf( outputfile_p, " " );
    }

    // print element
    //
    if( bigendian == 1 )
    {
      fprintf( outputfile_p, " 0x%02X%02X", (uint8_t) rawdata_p[ element ], (uint8_t) rawdata_p[ element+1 ] );
    }
    else
    {
      fprintf( outputfile_p, " 0x%02X%02X", (uint8_t) rawdata_p[ element+1 ], (uint8_t) rawdata_p[ element ] );
    }

    if( element < ( table_size-2 ) )
    {
      fprintf( outputfile_p, ",");
    }

    // If we are at 8 elements in then start a new line
    //
    if( ( element / 2 ) % NUM_COLUMNS == NUM_COLUMNS-1 )
    {
      fprintf( outputfile_p, "\n" );
    }

    if( element == ( table_size-2 ) )
    {
      fprintf( outputfile_p, "\n};\n\n" );
      fprintf( outputfile_p, "#endif // End of _%s_H\n",outp_header_name );
    }
  }
  printf( "Size of output file: %li\n", ftell( outputfile_p ) );
  if( ferror( outputfile_p ) != 0 )
  {
    printSystemError( "write output file", output_file );
    fclose( outputfile_p );
    return ERROR_NOT_OPEN;
  }

  if( fclose( outputfile_p ) != 0 )
  {
    printSystemError( "close output file", output_file );
    return ERROR_NOT_OPEN;
  }

  return WRITE_SUCCESS;
}


/** Read in the file to be converted to the header
  *
  * @param char* input filename to read
  * @retval int status code
  */
 int getRaw( char* input_file )
 {
  size_t count = 0;

  if( input_file == 0 || strlen( input_file )  < 1 )
  {
    fprintf( stderr, "Error: invalid input filename.\n" );
    return INVALID_FN;
  }
  printf( "IF: %s.  ", input_file );
    
  table_size = getFileSize( input_file );
  
  if( table_size == FILE_NOT_FOUND )
  {
    return ERROR_NOT_OPEN;
  }

  if( table_size <= 0 )
  {
    fprintf( stderr, "Error: empty file.\n" );
    return EMPTY_FILE;
  }
  else 
  {
    printf( "Size of input file: %li\n", table_size );
  }

  rawdata_p = malloc( table_size );
  
  if( rawdata_p == 0 )
  {
    fprintf( stderr, "Error: failed to allocate %li bytes.\n", table_size );
    return NO_MALLOC;
  }
  rawfile_p = fopen( input_file, "rb" );
  if( rawfile_p == NULL )
  {
    printSystemError( "open input file", input_file );
    free( rawdata_p );
    rawdata_p = 0;
    return ERROR_NOT_OPEN;
  }

  count = fread( (void*) rawdata_p, 1, (size_t) table_size, rawfile_p );
  if( count != (size_t) table_size )
  {
    printSystemError( "read input file", input_file );
    fclose( rawfile_p );
    free( rawdata_p );
    rawdata_p = 0;
    return ERROR_NOT_OPEN;
  }
  
  if( fclose( rawfile_p ) != 0 )
  {
    printSystemError( "close input file", input_file );
    free( rawdata_p );
    rawdata_p = 0;
    return ERROR_NOT_OPEN;
  }

  return READ_SUCCESS;
}


/**
  * Prints usage information for the raw2header utility.
  */
void printUsage( void )
{
  printf( "\nraw2header file convertion utility V2.01.0\n\n" );
  printf( "Written in 2024, by Jennifer Gunn.\n\n" );
  printf( "Takes the input file and converts it to a header file.\n\n" );
  printf( "Usage: raw2header [--mono|-m|--stereo|-s] [-16/-b16] <input_file> <output_file> <varname>\n" );
  printf( "where -b16 generate a big-endian uint16_t and -16 generates a\n" );
  printf( "little endian uint16_t array.\n\n" );
  printf( "--pad=NN or --pad=0xNN appends one byte for odd sized files.\n\n" );
  printf( "--mono/-m or --stereo/-s emits a mode define in the output header.\n\n" );
  printf( "uint16_t arrays require an even sized file.\n\n" );
}


/**
  * Parses combined short flags like -ms for mono and stereo.
  * @param arg The command-line argument string starting with a single dash followed by multiple characters.
  * @retval int status code: 0 on success, 1 for help, -1 on invalid flag
  * This function iterates through each character in the combined short flags and sets the appropriate
  * configuration variables. It returns 1 if the help flag is encountered, -1 if an invalid flag is found,
  * and 0 if all flags are processed successfully.
  */
int parseCombinedShortFlags( const char* arg )
{
  // Parses combined short flags like -ms; returns 1 for help, -1 on invalid.
  const char* shortflags = arg + 1;

  while( *shortflags != '\0' )
  {
    switch( *shortflags )
    {
      case 'm':
        channelmode = MODE_MONO;
        break;
      case 's':
        channelmode = MODE_STEREO;
        break;
      case 'h':
        return 1;
      default:
        return -1;
    }
    shortflags++;
  }

  return 0;
}


/**
  * Parses the --pad=NN flag to enable padding for odd byte counts in uint16_t modes.
  * @param arg The command-line argument string starting with "--pad=".
  * @retval int status code: 0 on success, -1 on invalid format or value
  * This function extracts the padding byte value from the argument, validates it, and sets the global
  * padding configuration. The padding byte must be a valid hexadecimal value between 0x00 and 0xFF.
  */
int parsePadFlag( const char* arg )
{
  // Parses --pad=NN or --pad=0xNN (hex) and stores the padding byte.
  const char* pad_text = arg + 6;
  char* endptr = 0;
  unsigned long pad = 0;

  if( pad_text[0] == '\0' )
  {
    return -1;
  }

  pad = strtoul( pad_text, &endptr, 16 );
  if( endptr == pad_text || *endptr != '\0' || pad > 0xFF )
  {
    return -1;
  }

  pad_enabled = 1;
  pad_value = (uint8_t) pad;
  return 0;
}


/**
 * Parses command-line arguments and sets configuration variables.
 * @param argc Argument count from main.
 * @param argv Argument vector from main.
 * @param input Output pointer for input filename.
 * @param output Output pointer for output filename.
 * @param varname Output pointer for variable name in the header.
 * @retval int status code: 0 on success, 1 for help, -1 for invalid flag, -2 for missing positional args.
 * This function processes flags in any order before the three required positional arguments (input_file,
 * output_file, varname). It supports combined short flags
 */
int parseArgs( int argc, char** argv, char** input, char** output, char** varname )
{
  int i = 1;
  size_t option = 0;

  // Define an enumeration for the possible actions corresponding to command-line flags.
  typedef enum
  {
    OPT_WORD_LE,
    OPT_WORD_BE,
    OPT_HELP,
    OPT_MONO,
    OPT_STEREO
  } option_action_t;

  // Define a structure to map command-line flags to their corresponding actions.
  typedef struct
  {
    const char* flag;
    option_action_t action;
  } option_map_t;

  // Define the mapping of command-line flags to actions.
  const option_map_t options[] = {
    { "-16",      OPT_WORD_LE },  // 16-bit little-endian output
    { "-b16",     OPT_WORD_BE },  // 16-bit big-endian output
    { "-h",       OPT_HELP },     // short help
    { "--help",   OPT_HELP },     // long help
    { "--mono",   OPT_MONO },     // mono mode define
    { "-m",       OPT_MONO },     // mono mode define (short)
    { "--stereo", OPT_STEREO },   // stereo mode define
    { "-s",       OPT_STEREO }    // stereo mode define (short)
  };

  // Calculate the number of options in the table.
  const size_t options_count = sizeof( options ) / sizeof( options[0] );

  // Initialize defaults.
  wordmode = 0;
  bigendian = 0;
  channelmode = MODE_NONE;
  pad_enabled = 0;
  pad_value = 0;

  // Consume leading flags before positional args.
  while( i < argc && argv[i][0] == '-' )
  {
    if( strncmp( argv[i], "--pad=", 6 ) == 0 )
    {
      int pad_state = parsePadFlag( argv[i] );
      if( pad_state != 0 )
      {
        return pad_state;
      }
      i++;
      continue;
    }

    // Check for combined short flags like -ms.
    if( strcmp( argv[i], "-16" ) != 0 && strcmp( argv[i], "-b16" ) != 0
        && strncmp( argv[i], "--", 2 ) != 0 && strlen( argv[i] ) > 2 )
    {
      int short_state = parseCombinedShortFlags( argv[i] );
      if( short_state != 0 )
      {
        return short_state;
      }

      i++;
      continue;
    }

    // Check for matches in the options table.
    int matched = 0;

    for( option = 0; option < options_count; option++ )
    {
      if( strcmp( argv[i], options[ option ].flag ) == 0 )
      {
        matched = 1;
        switch( options[ option ].action )
        {
          case OPT_WORD_LE:
            wordmode = 1;
            break;
          case OPT_WORD_BE:
            wordmode = 1;
            bigendian = 1;
            break;
          case OPT_HELP:
            return 1;
          case OPT_MONO:
            channelmode = MODE_MONO;
            break;
          case OPT_STEREO:
            channelmode = MODE_STEREO;
            break;
        }
        break;
      }
    }

    if( !matched )
    {
      return -1;
    }
    i++;
  }

  if( ( argc - i ) < 3 )
  {
    return -2;
  }

  *input = argv[i];
  *output = argv[i + 1];
  *varname = argv[i + 2];

  return 0;
}


/** Application entry point
 * 
 */
int main( int argc, char* argv[] )
{
  int state = 0;
  char* input_file = 0;
  char* output_file = 0;
  char* varname = 0;

  state = parseArgs( argc, argv, &input_file, &output_file, &varname );
  if( state != 0 )
  {
    printUsage();
    return EXIT_FAILURE;
  }

  if( pad_enabled && !wordmode )
  {
    // Padding only applies to uint16_t output.
    fprintf( stderr, "Error: --pad requires -16 or -b16.\n" );
    printUsage();
    return EXIT_FAILURE;
  }

  printf( "Processing\n");
  
  state = getRaw( input_file );
  switch( state )
  {
    case ERROR_NOT_OPEN:
      fprintf( stderr, "Error: could not read input file.\n" );
      return EXIT_FAILURE;
    case EMPTY_FILE:
      fprintf( stderr, "Error: empty file, nothing to do.\n" );
      return EXIT_FAILURE;
    case READ_SUCCESS:
      break;
    default:
      fprintf( stderr, "Error: failed to load input file.\n" );
      return EXIT_FAILURE;
  }

  if( ( ( table_size % 2) != 0 ) && wordmode )
  {
    // Pad odd byte counts to form complete uint16_t pairs.
    if( pad_enabled )
    {
      void* padded = realloc( rawdata_p, (size_t) table_size + 1 );
      if( padded == 0 )
      {
        fprintf( stderr, "Error: failed to allocate padding byte.\n" );
        free( rawdata_p );
        rawdata_p = 0;
        return EXIT_FAILURE;
      }

      rawdata_p = padded;
      rawdata_p[ table_size ] = (int8_t) pad_value;
      table_size += 1;
    }
    else
    {
      fprintf( stderr, "\nError: uint16_t modes require an even sized file\n\n" );
      printUsage();
      return EXIT_FAILURE;
    }
  } 

  // Write the output file.
  if( wordmode == 0 )
    state = writeFile( output_file, varname );
  else
    state = writeFile16( output_file, varname );

  if( state == ERROR_NOT_OPEN )
  {
    fprintf( stderr, "Error: could not write output file.\n" );
    free( rawdata_p );
    rawdata_p = 0;
    return EXIT_FAILURE;
  }

  printf( "Header file completed successfully\n" );
  free( rawdata_p );
  rawdata_p = 0;

  return EXIT_SUCCESS;
}


/**
 * Prints a system error message to stderr with context and optional path.
 * @param context Description of the operation that failed (e.g., "open file").
 * @param path Optional file path related to the error; can be NULL.
 * This function uses errno to provide detailed error information.
 * Example usage:
 *  printSystemError("open input file", "data.bin");
 */
void printSystemError( const char* context, const char* path )
{
  if( path != 0 )
  {
    fprintf( stderr, "Error: failed to %s '%s': %s\n", context, path, strerror( errno ) );
  }
  else
  {
    fprintf( stderr, "Error: failed to %s: %s\n", context, strerror( errno ) );
  }
}
