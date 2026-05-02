#ifndef ADPCM_H
#define ADPCM_H

#include <stdint.h>
#include <stddef.h>

/**
 * Encodes PCM audio to IMA ADPCM format.
 *
 * @param pcm Pointer to PCM samples.
 *            - is16bit=1: int16_t PCM
 *            - is16bit=0: uint8_t PCM (unsigned 0..255)
 * @param num_samples Number of samples to encode
 * @param is16bit 1 if input is int16_t, 0 if input is uint8_t
 * @param channels Number of channels in interleaved input (1=mono, 2=stereo)
 * @param out_size Output parameter: will be set to encoded data size in bytes
 * @return Allocated buffer containing encoded ADPCM data, or NULL on failure
 */
uint8_t* encode_ima_adpcm( const void* pcm, size_t num_samples, int is16bit,
						   int channels, size_t* out_size );

#endif // ADPCM_H
