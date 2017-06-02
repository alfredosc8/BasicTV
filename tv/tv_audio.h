#include "../id/id.h"
#ifndef TV_AUDIO_H
#define TV_AUDIO_H
#include "../util.h"
#include <SDL2/SDL_mixer.h>
#include <algorithm> // std::reverse

// 48000 is the default for Opus
#define TV_AUDIO_DEFAULT_SAMPLING_RATE 48000
#define TV_AUDIO_DEFAULT_BIT_DEPTH 16
/*
  Size of chunk set to speakers at the same time.
  If it is set too high, latencies will be a problem
  If it is set too low, too much computing time will be used

  256 is pretty low, but I think even Raspberry Pis can handle it
*/
#define TV_AUDIO_DEFAULT_CHUNK_SIZE 256

// shouldn't ever be the case
#define TV_AUDIO_FORMAT_UNDEFINED 0
// At least use FLAC if you are looking for quality, this is for testing
#define TV_AUDIO_FORMAT_RAW 1
// default, works pretty well, nice licensing
#define TV_AUDIO_FORMAT_OPUS 2
// not used, but planned (soon enough)
#define TV_AUDIO_FORMAT_FLAC 3
#define TV_AUDIO_FORMAT_MP3 4

#define TV_AUDIO_FLAG_OPUS_VBR (1 << 0)


// first 27 bits for Opus
#define TV_AUDIO_SET_VAR_OPUS_VBR(val) ((val >> 0)&0b1)
#define TV_AUDIO_SET_VAR_OPUS_APPLICATION(var) ((var >> 1) & 0b11)
#define TV_AUDIO_SET_VAR_OPUS_COMPLEXITY(var) ((var >> 3) & 0b1111)
#define TV_AUDIO_SET_VAR_OPUS_SIGNAL(var) ((var >> 7) & 0b11)
#define TV_AUDIO_SET_VAR_OPUS_VBR_CONSTRAINED(var) ((var >> 9) & 0b1)
#define TV_AUDIO_SET_VAR_OPUS_LOSS_PERC(var) ((var >> 10) & 0b1111111)
#define TV_AUDIO_SET_VAR_OPUS_DTX(var) ((var >> 17) & 0b1)
#define TV_AUDIO_SET_VAR_OPUS_FORCE_CHANNELS(var) ((var >> 18) & 0b11)
#define TV_AUDIO_SET_VAR_OPUS_INBAND_FEC(var) ((var >> 20) & 0b1)
#define TV_AUDIO_SET_VAR_OPUS_BANDWIDTH(var) ((var >> 21) & 0b111)

#pragma message("tv audio get flags aren't implemented yet")

/*
  Audio properties:
  This is non-exportbale, non-networkable, and inhreitable type that is
  used for reading, writing, and converting in audio streams.

  Flags is a dumping ground for anything encoder specific. Macros are used
  to fetch the proper bits and shift them down. 
 */

struct tv_audio_prop_t{
private:
	uint64_t flags = 0;
	uint8_t format = 0;
	uint8_t bit_depth = 0;
	uint32_t bit_rate = 0;
	uint32_t sampling_freq = 0;
	uint8_t channel_count = 0;
public:
	void list_virtual_data(data_id_t *id);
	tv_audio_prop_t();
	~tv_audio_prop_t();
	GET_SET(flags, uint32_t);
	GET_SET(format, uint8_t);
	GET_SET(sampling_freq, uint32_t);
	GET_SET(bit_depth, uint8_t);
	GET_SET(bit_rate, uint32_t);
	GET_SET(channel_count, uint8_t);
	
};

extern std::vector<id_t_> tv_audio_load_wav(id_t_ channel_id, uint64_t start_time_micro_s, std::string file);
extern void tv_audio_init();
extern void tv_audio_loop();
extern void tv_audio_close();
#endif
