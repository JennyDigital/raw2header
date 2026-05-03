#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include "raw2header_io.h"

// Configuration constants
#define NUM_COLUMNS         8


static const char* getFilenamePart( const char* path )
{
  const char* sep = strrchr( path, '/' );
  return ( sep == 0 ) ? path : ( sep + 1 );
}


static int buildSourcePath( const char* header_path, char* source_path, size_t source_path_sz )
{
  const char* slash = strrchr( header_path, '/' );
  const char* dot = strrchr( header_path, '.' );
  size_t base_len = strlen( header_path );

  if( dot != 0 && ( slash == 0 || dot > slash ) )
  {
    base_len = (size_t)( dot - header_path );
  }

  if( base_len + 3 >= source_path_sz )
  {
    return -1;
  }

  memcpy( source_path, header_path, base_len );
  source_path[ base_len ] = '\0';
  strcat( source_path, ".c" );

  return 0;
}


/** Get the size of the named file
  *
  * @param file_to_size
  * @retval off_t file size.  -1 if invalid in any way
  *
  */
off_t getFileSize( char* file_to_size )
{
  long int fsize;
  FILE* rawfile_p;

  rawfile_p = fopen( file_to_size, "rb" );
  if( rawfile_p == NULL )
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
  fsize = ftell( rawfile_p );
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
  char outp_header_name[255] = {0};
  FILE* headerfile_p;
  FILE* sourcefile_p = 0;

  strncpy( outp_header_name, varname, 254 );
  outp_header_name[254] = '\0';

  st_p = (char*) outp_header_name;
  while( *st_p != 0 )
  {
    *st_p = toupper( *st_p );
    st_p++;
  }

  printf( "OF: %s\n", output_file );

  headerfile_p = fopen( output_file, "w" );
  if( headerfile_p == 0 )
  {
    printSystemError( "open output header", output_file );
    return ERROR_NOT_OPEN;
  }

  fprintf( headerfile_p, "#ifndef _%s_H\n", outp_header_name );
  fprintf( headerfile_p, "#define _%s_H\n\n", outp_header_name );
  fprintf( headerfile_p, "#include <stdint.h>\n\n" );
  if( channelmode != MODE_NONE )
  {
    if( adpcm_enabled )
    {
      fprintf( headerfile_p, "#define %s_PB_FMT Mode_%s_ADPCM\n", outp_header_name,
               ( channelmode == MODE_MONO ) ? "mono" : "stereo" );
    }
    else
    {
      fprintf( headerfile_p, "#define %s_PB_FMT Mode_%s\n", outp_header_name,
               ( channelmode == MODE_MONO ) ? "mono" : "stereo" );
    }
  }
  fprintf( headerfile_p, "#define %s_SZ %li\n\n", outp_header_name, table_size );

  if( sourcepair_enabled )
  {
    char source_file[512] = {0};
    const char* header_include = getFilenamePart( output_file );

    fprintf( headerfile_p, "extern const uint8_t %s[ %s_SZ ];\n\n", varname, outp_header_name );
    fprintf( headerfile_p, "#endif // End of _%s_H\n", outp_header_name );

    if( ferror( headerfile_p ) != 0 )
    {
      printSystemError( "write output header", output_file );
      fclose( headerfile_p );
      return ERROR_NOT_OPEN;
    }
    if( fclose( headerfile_p ) != 0 )
    {
      printSystemError( "close output header", output_file );
      return ERROR_NOT_OPEN;
    }

    if( buildSourcePath( output_file, source_file, sizeof( source_file ) ) != 0 )
    {
      fprintf( stderr, "Error: output filename is too long to derive source pair path.\n" );
      return ERROR_NOT_OPEN;
    }

    printf( "CF: %s\n", source_file );
    sourcefile_p = fopen( source_file, "w" );
    if( sourcefile_p == 0 )
    {
      printSystemError( "open output source", source_file );
      return ERROR_NOT_OPEN;
    }

    fprintf( sourcefile_p, "#include \"%s\"\n\n", header_include );
    fprintf( sourcefile_p, "const uint8_t %s[ %s_SZ ] =\n{\n", varname, outp_header_name );

    for( long int element = 0; element < table_size; element++ )
    {
      if( element % NUM_COLUMNS == 0 )
      {
        fprintf( sourcefile_p, " " );
      }

      fprintf( sourcefile_p, " 0x%02X", (uint8_t) rawdata_p[ element ] );

      if( element < ( table_size - 1 ) )
      {
        fprintf( sourcefile_p, "," );
      }

      if( element % NUM_COLUMNS == NUM_COLUMNS - 1 )
      {
        fprintf( sourcefile_p, "\n" );
      }
    }
    fprintf( sourcefile_p, "\n};\n" );

    printf( "Size of output source file: %li\n", ftell( sourcefile_p ) );
    if( ferror( sourcefile_p ) != 0 )
    {
      printSystemError( "write output source", source_file );
      fclose( sourcefile_p );
      return ERROR_NOT_OPEN;
    }
    if( fclose( sourcefile_p ) != 0 )
    {
      printSystemError( "close output source", source_file );
      return ERROR_NOT_OPEN;
    }

    return WRITE_SUCCESS;
  }

  fprintf( headerfile_p, "const uint8_t %s[ %s_SZ ] =\n{\n", varname, outp_header_name );

  for( long int element = 0; element < table_size; element++ )
  {
    if( element % NUM_COLUMNS == 0 )
    {
      fprintf( headerfile_p, " " );
    }

    fprintf( headerfile_p, " 0x%02X", (uint8_t) rawdata_p[ element ] );

    if( element < ( table_size - 1 ) )
    {
      fprintf( headerfile_p, "," );
    }

    if( element % NUM_COLUMNS == NUM_COLUMNS - 1 )
    {
      fprintf( headerfile_p, "\n" );
    }
  }
  fprintf( headerfile_p, "\n};\n\n" );
  fprintf( headerfile_p, "#endif // End of _%s_H\n", outp_header_name );

  printf( "Size of output file: %li\n", ftell( headerfile_p ) );
  if( ferror( headerfile_p ) != 0 )
  {
    printSystemError( "write output file", output_file );
    fclose( headerfile_p );
    return ERROR_NOT_OPEN;
  }

  if( fclose( headerfile_p ) != 0 )
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
  char outp_header_name[255] = {0};
  FILE* headerfile_p;
  FILE* sourcefile_p = 0;

  strncpy( outp_header_name, varname, 254 );
  outp_header_name[254] = '\0';

  st_p = (char*) outp_header_name;
  while( *st_p != 0 )
  {
    *st_p = toupper( *st_p );
    st_p++;
  }

  printf( "OF: %s\n", output_file );

  headerfile_p = fopen( output_file, "w" );
  if( headerfile_p == 0 )
  {
    printSystemError( "open output header", output_file );
    return ERROR_NOT_OPEN;
  }

  fprintf( headerfile_p, "#ifndef _%s_H\n", outp_header_name );
  fprintf( headerfile_p, "#define _%s_H\n\n", outp_header_name );
  fprintf( headerfile_p, "#define %s_", outp_header_name );
  if( bigendian == 1 )
  {
    fprintf( headerfile_p, "BIG_ENDIAN\n" );
  }
  else
  {
    fprintf( headerfile_p, "LITTLE_ENDIAN\n" );
  }
  fprintf( headerfile_p, "#include <stdint.h>\n\n" );
  if( channelmode != MODE_NONE )
  {
    if( adpcm_enabled )
    {
      fprintf( headerfile_p, "#define %s_PB_FMT Mode_%s_ADPCM\n", outp_header_name,
               ( channelmode == MODE_MONO ) ? "mono" : "stereo" );
    }
    else
    {
      fprintf( headerfile_p, "#define %s_PB_FMT Mode_%s\n", outp_header_name,
               ( channelmode == MODE_MONO ) ? "mono" : "stereo" );
    }
  }
  fprintf( headerfile_p, "#define %s_SZ %li\n\n", outp_header_name, table_size / 2 );

  if( sourcepair_enabled )
  {
    char source_file[512] = {0};
    const char* header_include = getFilenamePart( output_file );

    fprintf( headerfile_p, "extern const uint16_t %s[ %s_SZ ];\n\n", varname, outp_header_name );
    fprintf( headerfile_p, "#endif // End of _%s_H\n", outp_header_name );

    if( ferror( headerfile_p ) != 0 )
    {
      printSystemError( "write output header", output_file );
      fclose( headerfile_p );
      return ERROR_NOT_OPEN;
    }
    if( fclose( headerfile_p ) != 0 )
    {
      printSystemError( "close output header", output_file );
      return ERROR_NOT_OPEN;
    }

    if( buildSourcePath( output_file, source_file, sizeof( source_file ) ) != 0 )
    {
      fprintf( stderr, "Error: output filename is too long to derive source pair path.\n" );
      return ERROR_NOT_OPEN;
    }

    printf( "CF: %s\n", source_file );
    sourcefile_p = fopen( source_file, "w" );
    if( sourcefile_p == 0 )
    {
      printSystemError( "open output source", source_file );
      return ERROR_NOT_OPEN;
    }

    fprintf( sourcefile_p, "#include \"%s\"\n\n", header_include );
    fprintf( sourcefile_p, "const uint16_t %s[ %s_SZ ] =\n{\n", varname, outp_header_name );

    for( long int element = 0; element < table_size; element += 2 )
    {
      if( element % NUM_COLUMNS == 0 )
      {
        fprintf( sourcefile_p, " " );
      }

      if( bigendian == 1 )
      {
        fprintf( sourcefile_p, " 0x%02X%02X", (uint8_t) rawdata_p[ element ], (uint8_t) rawdata_p[ element + 1 ] );
      }
      else
      {
        fprintf( sourcefile_p, " 0x%02X%02X", (uint8_t) rawdata_p[ element + 1 ], (uint8_t) rawdata_p[ element ] );
      }

      if( element < ( table_size - 2 ) )
      {
        fprintf( sourcefile_p, "," );
      }

      if( ( element / 2 ) % NUM_COLUMNS == NUM_COLUMNS - 1 )
      {
        fprintf( sourcefile_p, "\n" );
      }
    }
    fprintf( sourcefile_p, "\n};\n" );

    printf( "Size of output source file: %li\n", ftell( sourcefile_p ) );
    if( ferror( sourcefile_p ) != 0 )
    {
      printSystemError( "write output source", source_file );
      fclose( sourcefile_p );
      return ERROR_NOT_OPEN;
    }
    if( fclose( sourcefile_p ) != 0 )
    {
      printSystemError( "close output source", source_file );
      return ERROR_NOT_OPEN;
    }

    return WRITE_SUCCESS;
  }

  fprintf( headerfile_p, "const uint16_t %s[ %s_SZ ] =\n{\n", varname, outp_header_name );

  for( long int element = 0; element < table_size; element += 2 )
  {
    if( element % NUM_COLUMNS == 0 )
    {
      fprintf( headerfile_p, " " );
    }

    if( bigendian == 1 )
    {
      fprintf( headerfile_p, " 0x%02X%02X", (uint8_t) rawdata_p[ element ], (uint8_t) rawdata_p[ element + 1 ] );
    }
    else
    {
      fprintf( headerfile_p, " 0x%02X%02X", (uint8_t) rawdata_p[ element + 1 ], (uint8_t) rawdata_p[ element ] );
    }

    if( element < ( table_size - 2 ) )
    {
      fprintf( headerfile_p, "," );
    }

    if( ( element / 2 ) % NUM_COLUMNS == NUM_COLUMNS - 1 )
    {
      fprintf( headerfile_p, "\n" );
    }
  }
  fprintf( headerfile_p, "\n};\n\n" );
  fprintf( headerfile_p, "#endif // End of _%s_H\n", outp_header_name );

  printf( "Size of output file: %li\n", ftell( headerfile_p ) );
  if( ferror( headerfile_p ) != 0 )
  {
    printSystemError( "write output file", output_file );
    fclose( headerfile_p );
    return ERROR_NOT_OPEN;
  }

  if( fclose( headerfile_p ) != 0 )
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
  FILE* rawfile_p;

  if( input_file == 0 || strlen( input_file ) < 1 )
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
