//#include <math.h>
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

// Private function declarations
//
int   getRaw      ( char* input_file );
int   writeFile   ( char* output_file, char* varname );
off_t getFileSize ( char* input_file );
void  printUsage  ( void );
int   parseArgs   ( int argc, char** argv, char** input, char** output, char** varname );
void  printSystemError( const char* context, const char* path );


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
  * @param char* input filename */
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


void printUsage( void )
{
  printf( "\nraw2header file convertion utility V2.00.0\n\n" );
  printf( "Written in 2024, by Jennifer Gunn.\n\n" );
  printf( "Takes the input file and converts it to a header file.\n\n" );
  printf( "Usage: raw2header [--mono|-m|--stereo|-s] [-16/-b16] <input_file> <output_file> <varname>\n" );
  printf( "where -b16 generate a big-endian uint16_t and -16 generates a\n" );
  printf( "little endian uint16_t array.\n\n" );
  printf( "--mono/-m or --stereo/-s emits a mode define in the output header.\n\n" );
  printf( "uint16_t arrays require an even sized file.\n\n" );
}


int parseArgs( int argc, char** argv, char** input, char** output, char** varname )
{
  int i = 1;

  wordmode = 0;
  bigendian = 0;
  channelmode = MODE_NONE;

  // Consume leading flags before positional args.
  while( i < argc && argv[i][0] == '-' )
  {
    if( strcmp( argv[i], "-16" ) == 0 )
    {
      wordmode = 1;
    }
    else if( strcmp( argv[i], "-b16" ) == 0 )
    {
      wordmode = 1;
      bigendian = 1;
    }
    else if( ( strcmp( argv[i], "-h" ) == 0 ) || ( strcmp( argv[i], "--help" ) == 0 ) )
    {
      return 1;
    }
    else if( strcmp( argv[i], "--mono" ) == 0 )
    {
      channelmode = MODE_MONO;
    }
    else if( strcmp( argv[i], "-m" ) == 0 )
    {
      channelmode = MODE_MONO;
    }
    else if( strcmp( argv[i], "--stereo" ) == 0 )
    {
      channelmode = MODE_STEREO;
    }
    else if( strcmp( argv[i], "-s" ) == 0 )
    {
      channelmode = MODE_STEREO;
    }
    else
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

  printf( "Processing\n");
  
  state = getRaw( input_file );
  if( state == ERROR_NOT_OPEN )
  {
    fprintf( stderr, "Error: could not read input file.\n" );
    return EXIT_FAILURE;
  }
  if( state == EMPTY_FILE )
  {
    fprintf( stderr, "Error: empty file, nothing to do.\n" );
    return EXIT_FAILURE;
  }
  if( state != READ_SUCCESS )
  {
    fprintf( stderr, "Error: failed to load input file.\n" );
    return EXIT_FAILURE;
  }

  if( ( ( table_size % 2) != 0 ) && wordmode )
  {
    fprintf( stderr, "\nError: uint16_t modes require an even sized file\n\n" );
    printUsage();
    return EXIT_FAILURE;
  } 

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
