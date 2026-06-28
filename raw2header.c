#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "adpcm.h"
#include "raw2header_io.h"
#include "raw2header_cli.h"

// Private variables
//
int8_t *  rawdata_p;
off_t     table_size;
uint8_t   wordmode          = 0;
uint8_t   bigendian         = 0;
uint8_t   channelmode       = MODE_NONE;
uint8_t   pad_enabled       = 0;
uint8_t   pad_value         = 0;
uint8_t   adpcm_enabled     = 0;
uint8_t   sourcepair_enabled = 0;
char      g_generated_with[256] = "";

static int normalizeOutputHeaderPath( const char* input_path, char* output_path, size_t output_path_sz )
{
  const char* slash = strrchr( input_path, '/' );
  const char* dot = strrchr( input_path, '.' );
  int has_extension = ( dot != 0 && ( slash == 0 || dot > slash ) );
  size_t in_len = strlen( input_path );

  if( has_extension )
  {
    if( in_len + 1 > output_path_sz )
    {
      return -1;
    }

    memcpy( output_path, input_path, in_len + 1 );
    return 0;
  }

  if( in_len + 3 > output_path_sz )
  {
    return -1;
  }

  memcpy( output_path, input_path, in_len );
  output_path[ in_len ] = '\0';
  strcat( output_path, ".h" );

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
  char normalized_output_file[1024] = {0};

  state = parseArgs( argc, argv, &input_file, &output_file, &varname );
  if( state == 1 )
  {
    printUsage();
    return EXIT_SUCCESS;
  }

  if( state != 0 )
  {
    printUsage();
    return EXIT_FAILURE;
  }

  // Build switches string for header comment
  g_generated_with[0] = '\0';
  for( int i = 1; i < argc - 3; i++ )
  {
    strncat( g_generated_with, argv[i], sizeof( g_generated_with ) - strlen( g_generated_with ) - 1 );
    strncat( g_generated_with, " ", sizeof( g_generated_with ) - strlen( g_generated_with ) - 1 );
  }
  {
    size_t len = strlen( g_generated_with );
    if( len > 0 ) g_generated_with[ len - 1 ] = '\0';
  }

  if( normalizeOutputHeaderPath( output_file, normalized_output_file, sizeof( normalized_output_file ) ) != 0 )
  {
    fprintf( stderr, "Error: output filename is too long.\n" );
    return EXIT_FAILURE;
  }

  if( pad_enabled && !wordmode )
  {
    // Padding only applies to uint16_t output.
    fprintf( stderr, "Error: --pad requires -16 or -b16.\n" );
    printUsage();
    return EXIT_FAILURE;
  }

  printf( "Processing\n" );
  
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

  if( adpcm_enabled )
  {
    size_t frame_bytes = 1;

    if( channelmode == MODE_STEREO )
    {
      frame_bytes = ( wordmode == 1 ) ? 4 : 2;
    }
    else if( wordmode == 1 )
    {
      frame_bytes = 2;
    }

    if( ( table_size % (off_t)frame_bytes ) != 0 )
    {
      fprintf( stderr, "Error: ADPCM input size must align to %zu-byte %s frame size.\n",
               frame_bytes, ( channelmode == MODE_STEREO ) ? "stereo" : "mono" );
      return EXIT_FAILURE;
    }
  }
  else if( ( ( table_size % 2) != 0 ) && wordmode )
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
      fprintf( stderr, "\nError: uint16_t modes require an even sized file or --pad=NN\n\n" );
      printUsage();
      return EXIT_FAILURE;
    }
  } 


  // If ADPCM is enabled, encode and replace rawdata_p
  if (adpcm_enabled) {
    size_t adpcm_size = 0;
    int is16bit = 0;
    size_t num_samples = table_size;
    if (table_size % 2 == 0 && wordmode == 1) {
      if( bigendian == 1 )
      {
        // Convert big-endian 16-bit PCM bytes to host-endian int16_t samples.
        for( off_t i = 0; i < table_size; i += 2 )
        {
          int8_t temp = rawdata_p[i];
          rawdata_p[i] = rawdata_p[i + 1];
          rawdata_p[i + 1] = temp;
        }
      }

      // -16/-b16 indicates 16-bit PCM input for ADPCM mode.
      is16bit = 1;
      num_samples = table_size / 2;
    }

    int channels = ( channelmode == MODE_STEREO ) ? 2 : 1;
    uint8_t* adpcm_data = encode_ima_adpcm(rawdata_p, num_samples, is16bit,
                         channels, &adpcm_size);
    if (!adpcm_data) {
      fprintf(stderr, "Error: failed to encode IMA ADPCM.\n");
      free(rawdata_p);
      rawdata_p = 0;
      return EXIT_FAILURE;
    }
    free(rawdata_p);
    rawdata_p = (int8_t*)adpcm_data;
    table_size = adpcm_size;
  }

  // Write the output file.
  if (adpcm_enabled)
    state = writeFile(normalized_output_file, varname); // Output as uint8_t array
  else if( wordmode == 0 )
    state = writeFile( normalized_output_file, varname );
  else
    state = writeFile16( normalized_output_file, varname );

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
