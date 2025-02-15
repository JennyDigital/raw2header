//#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

// Error Codes
//
#define INVALID_FN          -99
#define ARGUMENTS_ERROR     -98
#define ERROR_NOT_OPEN      -97
#define NO_MALLOC           -96
#define EMPTY_FILE          -95
#define WRITE_SUCCESS       -94
#define READ_SUCCESS        -93

// Configuration constants
//
#define NUM_COLUMNS         8

// Private variables
//
int8_t *  rawdata_p;
off_t     table_size;
FILE *    outputfile_p;
FILE *    rawfile_p;
uint8_t   wordmode          = 0;
uint8_t   bigendian         = 0;
uint8_t   curr_arg          = 1;

// Private function declarations
//
int   getRaw      ( char* input_file );
int   writeFile   ( char* output_file, char* varname );
off_t getFileSize ( char* input_file );
void  printUsage  ( void );


/** Get the size of the named file
  *
  * @param file_to_size
  * @retval off_t file size.  -1 if invalid in any way
  *
  */
off_t getFileSize ( char* file_to_size )
{
  long int fsize;

  #define BAD_FILENAME    -1

  rawfile_p = fopen( file_to_size, "r" );
  if (rawfile_p == NULL)
  { 
    printf("File Not Found!\n");
    return -1; 
  } 
  fseek( rawfile_p, 0L, SEEK_END ); 
  fsize =  ftell( rawfile_p );
  fclose( rawfile_p );
  
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
    *st_p &= *st_p & 0xDF;
    st_p++;
  }

  printf( "OF: %s\n", output_file );

  outputfile_p = fopen( output_file, "w" );

  if( outputfile_p == 0 )
  {
    printf( "Error opening file\n" );
    return ERROR_NOT_OPEN;
  }

  fprintf( outputfile_p, "#ifndef _%s_H\n", outp_header_name );
  fprintf( outputfile_p, "#define _%s_H\n\n", outp_header_name );
  fprintf( outputfile_p, "#include <stdint.h>\n\n" );
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
  fclose( outputfile_p );
  
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
    *st_p &= *st_p & 0xDF;
    st_p++;
  }

  printf( "OF: %s\n", output_file );

  outputfile_p = fopen( output_file, "w" );

  if( outputfile_p == 0 )
  {
    printf( "Error opening file\n" );
    return ERROR_NOT_OPEN;
  }

  fprintf( outputfile_p, "#ifndef _%s_H\n", outp_header_name );
  fprintf( outputfile_p, "#define _%s_H\n\n", outp_header_name );
  fprintf( outputfile_p, "#define %s_",outp_header_name );
  if( bigendian == 1 )
  {
    fprintf( outputfile_p, "BIG_ENDIAN\n" );
  }
  else
  {
    fprintf( outputfile_p, "LITTLE_ENDIAN\n" );
  }
  fprintf( outputfile_p, "#include <stdint.h>\n\n" );
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
  fclose( outputfile_p );

  return WRITE_SUCCESS;
}


/** Read in the file to be converted to the header
  *
  * @param char* input filename */
 int getRaw( char* input_file )
 {
  off_t count;

  if( strlen( input_file )  < 1 ) return INVALID_FN;
  printf( "IF: %s.  ", input_file );
    
  table_size = getFileSize( input_file );
  
  if( table_size == BAD_FILENAME )
  {
    printf( "Bad input file!\n" );
    return ERROR_NOT_OPEN;
  }

  if( table_size == -1 )
  {
    printf( "Empty file!\n\n" );
    return EMPTY_FILE;
  }
  else 
  {
    printf( "Size of input file: %li\n", table_size );
  }

  rawdata_p = malloc( table_size );
  
  if( rawdata_p == 0 )
  {
    printf( "Failed to get required memory.\n" );
    return NO_MALLOC;
  }
  rawfile_p = fopen( input_file, "r" );
  if( rawfile_p == NULL )
  {
    printf("Failed to open file\n");
    return ERROR_NOT_OPEN;
  }

  count = fread( (void*) rawdata_p, 1, table_size, rawfile_p );
  
  fclose( rawfile_p );

  return READ_SUCCESS;
}


void printUsage( void )
{
  printf( "\nraw2header file convertion utility V1.02.3\n\n" );
  printf( "Written in 2024, by Jennifer Gunn.\n\n" );
  printf( "Takes the input file and converts it to a header file.\n\n" );
  printf( "Usage: raw2header [-16/-b16] <input_file> <output_file> <varname>\n" );
  printf( "where -b16 generate a big-endian uint16_t and -16 generates a\n" );
  printf( "little endian uint16_t array.\n\n" );
  printf( "uint16_t arrays require an even sized file.\n\n" );
}


int main( int argc, char* argv[] )
{
  int state = 0;

  if( argc > 1 )
  {
    if( *(argv[1]) == '-' )
    {
      if( strcmp( argv[1], "-16" ) == 0 )
      {
        curr_arg++;
        wordmode = 1;
      }
      else if( strcmp( argv[1], "-b16" ) == 0  )
      {
        curr_arg++;
        wordmode = 1;
        bigendian = 1;
      }
      else
      {
        printUsage();
        return EXIT_SUCCESS;
      }
    }
  }

  if( ( argc < ( 4 + wordmode ) ) )
  {
    printUsage();
    return EXIT_SUCCESS;
  }

  printf( "Processing\n");
  
  state = getRaw( argv[curr_arg++] );
  if( state == ERROR_NOT_OPEN )
  {
    printf( "Couldn't open input file\n" );
    return state;
  }
  if( state == EMPTY_FILE )
  {
    printf( "Empty file, nothing to do." );
    return state;
  }

  if( ( ( table_size % 2) != 0 ) && wordmode )
  {
    printf( "\nError: uint16_t modes require an even sized file\n\n" );
    printUsage();
    return EXIT_SUCCESS;
  } 

  if( wordmode == 0 )
    state = writeFile( argv[curr_arg], argv[curr_arg+1] );
  else
    state = writeFile16( argv[curr_arg], argv[curr_arg+1] );

  if( state == ERROR_NOT_OPEN )
  {
    printf( "Couldn't write file\n" );
    return state;
  }

  printf( "Writing completed successfully\n" );
  free( rawdata_p );

  return EXIT_SUCCESS;
}
