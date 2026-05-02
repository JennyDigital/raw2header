#include "adpcm.h"
#include <stdlib.h>

// IMA ADPCM encoder tables
const int indexTable[16] = {
  -1, -1, -1, -1, 2, 4, 6, 8,
  -1, -1, -1, -1, 2, 4, 6, 8
};

const int stepTable[89] = {
  7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
  34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
  157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
  724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
  3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
  15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static uint8_t encode_ima_adpcm_nibble( int sample, int* predictor, int* index )
{
  int step = stepTable[*index];
  int diff = sample - *predictor;
  int sign = ( diff < 0 ) ? 8 : 0;
  int delta = 0;
  int tempstep = step;
  int diffq = 0;

  if( sign )
  {
    diff = -diff;
  }

  if( diff >= tempstep )
  {
    delta = 4;
    diff -= tempstep;
  }
  tempstep >>= 1;

  if( diff >= tempstep )
  {
    delta |= 2;
    diff -= tempstep;
  }
  tempstep >>= 1;

  if( diff >= tempstep )
  {
    delta |= 1;
  }

  diffq = step >> 3;
  if( delta & 4 ) diffq += step;
  if( delta & 2 ) diffq += step >> 1;
  if( delta & 1 ) diffq += step >> 2;

  if( sign )
  {
    *predictor -= diffq;
  }
  else
  {
    *predictor += diffq;
  }

  if( *predictor > 32767 ) *predictor = 32767;
  if( *predictor < -32768 ) *predictor = -32768;

  *index += indexTable[( sign | delta ) & 0x0F];
  if( *index < 0 ) *index = 0;
  if( *index > 88 ) *index = 88;

  return (uint8_t)(( sign | delta ) & 0x0F);
}

// IMA ADPCM encoder for 8-bit or 16-bit PCM input
// Returns malloc'd buffer and sets out_size. Returns NULL on error.
// Output size is (num_samples + 1) / 2 (since each sample is 4 bits)
uint8_t* encode_ima_adpcm( const void* pcm, size_t num_samples, int is16bit,
                           int channels, size_t* out_size )
{
  if( !pcm || num_samples == 0 || !out_size ) return NULL;
  if( channels != 1 && channels != 2 ) return NULL;
  if( channels == 2 && ( num_samples % 2 ) != 0 ) return NULL;

  *out_size = ( num_samples + 1 ) / 2;
  uint8_t* out = malloc( *out_size );
  if( !out ) return NULL;

  int predictor[2] = { 0, 0 };
  int index[2] = { 0, 0 };
  int sample;
  int pcm8;
  int channel;
  uint8_t nibble;
  size_t i, o = 0;

  for( i = 0; i < num_samples; i++ ) {
    channel = ( channels == 2 ) ? (int)( i & 0x1 ) : 0;

    if( is16bit )
      sample = ((const int16_t*)pcm)[i];
    else
    {
      /* Firmware treats 8-bit PCM as unsigned 0..255. Convert to signed 16-bit range. */
      pcm8 = ((const uint8_t*)pcm)[i];
      sample = ( pcm8 - 128 ) << 8;
    }

    if( sample > 32767 ) sample = 32767;
    if( sample < -32768 ) sample = -32768;
    nibble = encode_ima_adpcm_nibble( sample, &predictor[channel], &index[channel] );

    /* ADPCM nibbles are packed low then high, following input sample order. */
    if( ( i & 0x1 ) == 0 )
    {
      out[o] = nibble;
    }
    else
    {
      out[o] |= (uint8_t)( nibble << 4 );
      o++;
    }
  }

  if( ( num_samples & 0x1 ) != 0 )
  {
    o++;
  }

  return out;
}
