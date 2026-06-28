#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "raw2header_io.h"
#include "raw2header_cli.h"

static int parseCombinedShortFlags( const char* arg );
static int parsePadFlag( const char* arg );


/**
  * Prints usage information for the raw2header utility.
  */
void printUsage( void )
{
  printf( "\nraw2header file convertion utility V3.02.0\n\n" );
  printf( "Written in 2024, by Jennifer Gunn.\n\n" );
  printf( "Takes the input file and converts it to a header file.\n\n" );
  printf( "Usage: raw2header [--mono|-m|--stereo|-s] [-16/-b16] [--adpcm|-a|-a16|-ab16] [--source-pair|--split-c|-c] <input_file> <output_file> <varname>\n" );
  printf( "If <output_file> has no extension, .h is appended automatically.\n" );
  printf( "where -b16 generate a big-endian uint16_t and -16 generates a\n" );
  printf( "little endian uint16_t array.\n\n" );
  printf( "--adpcm/-a encodes the input as IMA ADPCM and stores it as a uint8_t array.\n" );
  printf( "With --adpcm, -16/-b16 select 16-bit PCM input endianness.\n" );
  printf( "-a16/--adpcm16 and -ab16/--adpcm16be are one-step ADPCM + 16-bit PCM input flags.\n\n" );
  printf( "--source-pair/--split-c/-c writes externs to <output_file> and data to a paired .c file.\n\n" );
  printf( "--pad=NN or --pad=0xNN appends one byte for odd sized files.\n\n" );
  printf( "--mono/-m or --stereo/-s emits a mode define in the output header.\n\n" );
  printf( "uint16_t arrays require an even sized file unless padding is enabled.\n\n" );
  printf( "For ADPCM with 8-bit PCM input, omit -16/-b16.\n\n" );
}


/**
  * Parses combined short flags like -ms for mono and stereo.
  * @param arg The command-line argument string starting with a single dash followed by multiple characters.
  * @retval int status code: 0 on success, 1 for help, -1 on invalid flag
  */
static int parseCombinedShortFlags( const char* arg )
{
  const char* shortflags = arg + 1;

  while( *shortflags != '\0' )
  {
    switch( *shortflags )
    {
      case 'm':
        if( channelmode == MODE_STEREO )
        {
          fprintf( stderr, "Error: conflicting flags -m (mono) and -s (stereo).\n" );
          return -1;
        }
        channelmode = MODE_MONO;
        break;
      case 's':
        if( channelmode == MODE_MONO )
        {
          fprintf( stderr, "Error: conflicting flags -m (mono) and -s (stereo).\n" );
          return -1;
        }
        channelmode = MODE_STEREO;
        break;
      case 'a':
        adpcm_enabled = 1;
        break;
      case 'c':
        sourcepair_enabled = 1;
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
  */
static int parsePadFlag( const char* arg )
{
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
 */
int parseArgs( int argc, char** argv, char** input, char** output, char** varname )
{
  int i = 1;
  size_t option = 0;

  typedef enum
  {
    OPT_WORD_LE,
    OPT_WORD_BE,
    OPT_HELP,
    OPT_MONO,
    OPT_STEREO,
    OPT_ADPCM,
    OPT_ADPCM16_LE,
    OPT_ADPCM16_BE,
    OPT_SOURCE_PAIR
  } option_action_t;

  typedef struct
  {
    const char* flag;
    option_action_t action;
  } option_map_t;

  const option_map_t options[] = {
    { "-16",      OPT_WORD_LE },
    { "-b16",     OPT_WORD_BE },
    { "-h",       OPT_HELP },
    { "--help",   OPT_HELP },
    { "--mono",   OPT_MONO },
    { "-m",       OPT_MONO },
    { "--stereo", OPT_STEREO },
    { "-s",       OPT_STEREO },
    { "--adpcm",  OPT_ADPCM },
    { "-a",       OPT_ADPCM },
    { "--adpcm16",   OPT_ADPCM16_LE },
    { "--adpcm16be", OPT_ADPCM16_BE },
    { "-a16",        OPT_ADPCM16_LE },
    { "-ab16",       OPT_ADPCM16_BE },
    { "--source-pair", OPT_SOURCE_PAIR },
    { "--split-c",     OPT_SOURCE_PAIR },
    { "-c",            OPT_SOURCE_PAIR }
  };

  const size_t options_count = sizeof( options ) / sizeof( options[0] );

  wordmode = 0;
  bigendian = 0;
  channelmode = MODE_NONE;
  pad_enabled = 0;
  pad_value = 0;
  adpcm_enabled = 0;
  sourcepair_enabled = 0;

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

    if( strcmp( argv[i], "-16" ) != 0 && strcmp( argv[i], "-b16" ) != 0
      && strcmp( argv[i], "-a16" ) != 0 && strcmp( argv[i], "-ab16" ) != 0
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

    int matched = 0;

    for( option = 0; option < options_count; option++ )
    {
      if( strcmp( argv[i], options[ option ].flag ) == 0 )
      {
        matched = 1;
        switch( options[ option ].action )
        {
          case OPT_WORD_LE:
            if( wordmode && bigendian )
            {
              fprintf( stderr, "Error: conflicting flags -16 and -b16.\n" );
              return -1;
            }
            wordmode = 1;
            bigendian = 0;
            break;
          case OPT_WORD_BE:
            if( wordmode && !bigendian )
            {
              fprintf( stderr, "Error: conflicting flags -16 and -b16.\n" );
              return -1;
            }
            wordmode = 1;
            bigendian = 1;
            break;
          case OPT_HELP:
            return 1;
          case OPT_MONO:
            if( channelmode == MODE_STEREO )
            {
              fprintf( stderr, "Error: conflicting flags --mono and --stereo.\n" );
              return -1;
            }
            channelmode = MODE_MONO;
            break;
          case OPT_STEREO:
            if( channelmode == MODE_MONO )
            {
              fprintf( stderr, "Error: conflicting flags --mono and --stereo.\n" );
              return -1;
            }
            channelmode = MODE_STEREO;
            break;
          case OPT_ADPCM:
            adpcm_enabled = 1;
            break;
          case OPT_ADPCM16_LE:
            adpcm_enabled = 1;
            wordmode = 1;
            bigendian = 0;
            break;
          case OPT_ADPCM16_BE:
            adpcm_enabled = 1;
            wordmode = 1;
            bigendian = 1;
            break;
          case OPT_SOURCE_PAIR:
            sourcepair_enabled = 1;
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

  if( ( argc - i ) != 3 )
  {
    return -2;
  }

  *input = argv[i];
  *output = argv[i + 1];
  *varname = argv[i + 2];

  return 0;
}
