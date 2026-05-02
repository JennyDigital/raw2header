#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include "adpcm.h"

// Forward declarations for ADPCM tables (used in decoder)
extern const int indexTable[16];
extern const int stepTable[89];

// IMA ADPCM decoder for verification (optional: decodes ADPCM back to PCM for error checking)
static void decode_ima_adpcm( const uint8_t* adpcm_data, size_t num_encoded_bytes,
                               int16_t* pcm_out, size_t num_samples )
{
  int predictor = 0;
  int index = 0;
  int step = stepTable[index];
  size_t sample_idx = 0;
  uint8_t nibble;
  int diff;

  for( size_t byte_idx = 0; byte_idx < num_encoded_bytes && sample_idx < num_samples; byte_idx++ ) {
    // Decode high nibble
    nibble = ( adpcm_data[byte_idx] >> 4 ) & 0x0F;
    int sign = nibble & 8;
    int delta = nibble & 7;
    
    diff = 0;
    if( delta & 4 ) diff += step;
    if( delta & 2 ) diff += step >> 1;
    if( delta & 1 ) diff += step >> 2;
    diff += step >> 3;
    
    if( sign ) predictor -= diff;
    else predictor += diff;
    
    if( predictor > 32767 ) predictor = 32767;
    if( predictor < -32768 ) predictor = -32768;
    
    pcm_out[sample_idx++] = (int16_t)predictor;
    
    index += indexTable[nibble];
    if( index < 0 ) index = 0;
    if( index > 88 ) index = 88;
    step = stepTable[index];

    if( sample_idx >= num_samples ) break;

    // Decode low nibble
    nibble = adpcm_data[byte_idx] & 0x0F;
    sign = nibble & 8;
    delta = nibble & 7;
    
    diff = 0;
    if( delta & 4 ) diff += step;
    if( delta & 2 ) diff += step >> 1;
    if( delta & 1 ) diff += step >> 2;
    diff += step >> 3;
    
    if( sign ) predictor -= diff;
    else predictor += diff;
    
    if( predictor > 32767 ) predictor = 32767;
    if( predictor < -32768 ) predictor = -32768;
    
    pcm_out[sample_idx++] = (int16_t)predictor;
    
    index += indexTable[nibble];
    if( index < 0 ) index = 0;
    if( index > 88 ) index = 88;
    step = stepTable[index];
  }
}

// Test 1: Verify output size is correct
static int test_output_size( void )
{
  printf( "Test 1: Output size verification\n" );
  
  int16_t input_samples[10] = { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900 };
  size_t out_size = 0;
  uint8_t* output = encode_ima_adpcm( input_samples, 10, 1, 1, &out_size );
  
  // 10 samples -> (10+1)/2 = 5.5 -> 5 bytes expected
  int expected_size = (10 + 1) / 2;
  
  if( out_size != expected_size ) {
    printf( "  FAIL: Expected size %d, got %zu\n", expected_size, out_size );
    free( output );
    return 1;
  }
  
  printf( "  PASS: Output size is correct (%zu bytes)\n", out_size );
  free( output );
  return 0;
}

// Test 2: Verify odd-sized input is handled
static int test_odd_size( void )
{
  printf( "Test 2: Odd-sized input handling\n" );
  
  int16_t input_samples[11] = { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000 };
  size_t out_size = 0;
  uint8_t* output = encode_ima_adpcm( input_samples, 11, 1, 1, &out_size );
  
  // 11 samples -> (11+1)/2 = 6 bytes expected
  int expected_size = (11 + 1) / 2;
  
  if( out_size != expected_size ) {
    printf( "  FAIL: Expected size %d, got %zu\n", expected_size, out_size );
    free( output );
    return 1;
  }
  
  printf( "  PASS: Odd-sized input handled correctly (%zu bytes)\n", out_size );
  free( output );
  return 0;
}

// Test 3: Verify 8-bit input encoding
static int test_8bit_input( void )
{
  printf( "Test 3: 8-bit input encoding\n" );
  
  int8_t input_samples[8] = { 0, 20, 40, 60, 80, 100, 120, 127 };
  size_t out_size = 0;
  uint8_t* output = encode_ima_adpcm( input_samples, 8, 0, 1, &out_size );
  
  int expected_size = (8 + 1) / 2;
  
  if( out_size != expected_size ) {
    printf( "  FAIL: Expected size %d, got %zu\n", expected_size, out_size );
    free( output );
    return 1;
  }
  
  printf( "  PASS: 8-bit input encoded to %zu bytes\n", out_size );
  free( output );
  return 0;
}

// Test 4: Verify encoding and decoding produces reasonable reconstruction
static int test_round_trip( void )
{
  printf( "Test 4: Round-trip encode/decode verification\n" );
  
  // Create a simple test signal: low frequency sine wave
  int16_t input_samples[32];
  for( int i = 0; i < 32; i++ ) {
    input_samples[i] = (int16_t)(10000 * sin( 2.0 * 3.14159 * i / 32.0 ));
  }
  
  size_t out_size = 0;
  uint8_t* encoded = encode_ima_adpcm( input_samples, 32, 1, 1, &out_size );
  
  int16_t decoded_samples[32];
  decode_ima_adpcm( encoded, out_size, decoded_samples, 32 );
  
  // Calculate RMS error
  double sum_squared_error = 0.0;
  for( int i = 0; i < 32; i++ ) {
    int16_t diff = input_samples[i] - decoded_samples[i];
    sum_squared_error += (double)diff * diff;
  }
  double rms_error = sqrt( sum_squared_error / 32.0 );
  
  printf( "  Encoded to %zu bytes\n", out_size );
  printf( "  RMS reconstruction error: %.2f\n", rms_error );
  
  // ADPCM typically has RMS error in range of ~200-1000 depending on signal
  // For this low-frequency sine, error should be reasonable
  if( rms_error > 5000.0 ) {
    printf( "  FAIL: RMS error too high (%.2f)\n", rms_error );
    free( encoded );
    return 1;
  }
  
  printf( "  PASS: Reconstruction error within acceptable range\n" );
  free( encoded );
  return 0;
}

// Test 5: Verify silence (zero samples) encodes correctly
static int test_silence( void )
{
  printf( "Test 5: Silence encoding\n" );
  
  int16_t silence[16];
  memset( silence, 0, sizeof( silence ) );
  
  size_t out_size = 0;
  uint8_t* output = encode_ima_adpcm( silence, 16, 1, 1, &out_size );
  
  if( out_size != 8 ) {
    printf( "  FAIL: Expected 8 bytes, got %zu\n", out_size );
    free( output );
    return 1;
  }
  
  // Silence should all encode to 0x00 (zero nibbles)
  int all_zero = 1;
  for( size_t i = 0; i < out_size; i++ ) {
    if( output[i] != 0x00 ) {
      all_zero = 0;
      break;
    }
  }
  
  if( !all_zero ) {
    printf( "  FAIL: Silence not encoded as zeros\n" );
    free( output );
    return 1;
  }
  
  printf( "  PASS: Silence encoded correctly\n" );
  free( output );
  return 0;
}

// Test 6: Verify NULL input handling
static int test_null_input( void )
{
  printf( "Test 6: NULL input handling\n" );
  
  size_t out_size = 0;
  uint8_t* output = encode_ima_adpcm( NULL, 10, 1, 1, &out_size );
  
  if( output != NULL ) {
    printf( "  FAIL: Expected NULL for NULL input\n" );
    free( output );
    return 1;
  }
  
  printf( "  PASS: NULL input handled correctly\n" );
  return 0;
}

// Test 7: Verify zero-size input handling
static int test_zero_size( void )
{
  printf( "Test 7: Zero-size input handling\n" );
  
  int16_t dummy[1];
  size_t out_size = 0;
  uint8_t* output = encode_ima_adpcm( dummy, 0, 1, 1, &out_size );
  
  if( output != NULL ) {
    printf( "  FAIL: Expected NULL for zero-size input\n" );
    free( output );
    return 1;
  }
  
  printf( "  PASS: Zero-size input handled correctly\n" );
  return 0;
}

// Test 8: Verify stereo interleaved input encoding
static int test_stereo_input( void )
{
  printf( "Test 8: Stereo interleaved input encoding\n" );

  // 8 stereo frames (L,R interleaved) => 16 samples => 8 ADPCM bytes.
  int16_t stereo_samples[16] = {
    -12000, 12000, -10000, 10000, -8000, 8000, -6000, 6000,
    -4000, 4000, -2000, 2000, 0, 0, 2000, -2000
  };
  size_t out_size = 0;
  uint8_t* output = encode_ima_adpcm( stereo_samples, 16, 1, 2, &out_size );

  if( output == NULL ) {
    printf( "  FAIL: Stereo input returned NULL\n" );
    return 1;
  }

  if( out_size != 8 ) {
    printf( "  FAIL: Expected 8 bytes, got %zu\n", out_size );
    free( output );
    return 1;
  }

  printf( "  PASS: Stereo input encoded to %zu bytes\n", out_size );
  free( output );
  return 0;
}

int main( void )
{
  printf( "=== IMA ADPCM Encoder Test Suite ===\n\n" );
  
  int total_tests = 8;
  int passed_tests = 0;
  
  passed_tests += !test_output_size();
  passed_tests += !test_odd_size();
  passed_tests += !test_8bit_input();
  passed_tests += !test_round_trip();
  passed_tests += !test_silence();
  passed_tests += !test_null_input();
  passed_tests += !test_zero_size();
  passed_tests += !test_stereo_input();
  
  printf( "\n=== Test Results ===\n" );
  printf( "Passed: %d/%d\n", passed_tests, total_tests );
  
  return ( passed_tests == total_tests ) ? 0 : 1;
}
