#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include "raw2header_io.h"

// Globals provided by raw2header.c in production; test owns them here.
int8_t* rawdata_p = 0;
off_t table_size = 0;
uint8_t wordmode = 0;
uint8_t bigendian = 0;
uint8_t channelmode = MODE_NONE;
uint8_t pad_enabled = 0;
uint8_t pad_value = 0;
uint8_t adpcm_enabled = 0;
uint8_t sourcepair_enabled = 0;
char    g_generated_with[256] = "";

static int load_text_file( const char* path, char* buf, size_t buf_sz )
{
  FILE* fp = fopen( path, "r" );
  size_t read_count;

  if( fp == 0 )
  {
    return -1;
  }

  read_count = fread( buf, 1, buf_sz - 1, fp );
  buf[ read_count ] = '\0';

  fclose( fp );
  return 0;
}


static int file_contains( const char* text, const char* needle )
{
  return strstr( text, needle ) != 0;
}


int main( void )
{
  char base[256] = {0};
  char header_path[300] = {0};
  char source_path[300] = {0};
  char header_text[4096] = {0};
  char source_text[4096] = {0};
  time_t now = time( 0 );

  snprintf( base, sizeof( base ), "/tmp/raw2header_pair_test_%ld_%ld", (long) getpid(), (long) now );
  snprintf( header_path, sizeof( header_path ), "%s.h", base );
  snprintf( source_path, sizeof( source_path ), "%s.c", base );

  rawdata_p = malloc( 4 );
  if( rawdata_p == 0 )
  {
    fprintf( stderr, "FAIL: malloc failed\n" );
    return 1;
  }

  rawdata_p[0] = 0x11;
  rawdata_p[1] = 0x22;
  rawdata_p[2] = 0x33;
  rawdata_p[3] = 0x44;

  table_size = 4;
  sourcepair_enabled = 1;
  wordmode = 0;
  bigendian = 0;
  channelmode = MODE_NONE;
  adpcm_enabled = 0;

  if( writeFile( header_path, "pair_data" ) != WRITE_SUCCESS )
  {
    fprintf( stderr, "FAIL: writeFile source-pair mode failed\n" );
    free( rawdata_p );
    rawdata_p = 0;
    return 1;
  }

  if( load_text_file( header_path, header_text, sizeof( header_text ) ) != 0 )
  {
    fprintf( stderr, "FAIL: could not read generated header\n" );
    free( rawdata_p );
    rawdata_p = 0;
    return 1;
  }

  if( load_text_file( source_path, source_text, sizeof( source_text ) ) != 0 )
  {
    fprintf( stderr, "FAIL: could not read generated source\n" );
    free( rawdata_p );
    rawdata_p = 0;
    return 1;
  }

  if( !file_contains( header_text, "extern const uint8_t pair_data[ PAIR_DATA_SZ ];" ) )
  {
    fprintf( stderr, "FAIL: header missing extern declaration\n" );
    free( rawdata_p );
    rawdata_p = 0;
    return 1;
  }

  if( !file_contains( source_text, "#include \"raw2header_pair_test_" )
      || !file_contains( source_text, "const uint8_t pair_data[ PAIR_DATA_SZ ] =" ) )
  {
    fprintf( stderr, "FAIL: source missing include/definition\n" );
    free( rawdata_p );
    rawdata_p = 0;
    return 1;
  }

  unlink( header_path );
  unlink( source_path );

  free( rawdata_p );
  rawdata_p = 0;

  printf( "PASS: source-pair output generation\n" );
  return 0;
}
