#ifndef TV_TRANSCODE_H
#define TV_TRANSCODE_H
#include "../../id/id.h"
#include "../../id/id_api.h"
#include "../../util.h"
#include "../../convert.h"

#include "../../tv/tv_audio.h"
#include "../../tv/tv_frame_audio.h"
#include "../../tv/tv_video.h"
#include "../../tv/tv_frame_video.h"

/*
  Planned audio codec support (listed by desired popularity):
  Opus
  Codec2 (super low bitrate codec for digital HAM speech, excited for this one)
  WAV (partial already, but pretty bad and incomplete)
  FLAC
  MP3
  AAC
  Vorbis

  Planned video codec support (listed by desired popularity):
  VP9
  HEVC (h.265)
  h.264
 */

struct tv_transcode_encode_state_t{
private:
	void *state_ptr = nullptr;
	id_t_ start_frame_id = ID_BLANK_ID;
	uint64_t frame_depth = 0;
	// derived from start frame ID (easy lookup)
	uint8_t state_format = 0;
	uint64_t codec_state_ref = 0;

	tv_audio_prop_t audio_prop;
public:
	GET_SET(state_ptr, void*);
	GET_SET(audio_prop, tv_audio_prop_t);
	id_t_ get_start_frame_id();
	void set_start_frame_id(id_t_ id_);
	void inc_frame_depth(){frame_depth++;}
	uint8_t get_state_format(){return state_format;}
	uint64_t get_codec_state_ref(){return codec_state_ref;}
};


struct tv_transcode_decode_state_t{
private:
	void *state_ptr = nullptr;
	id_t_ start_frame_id = ID_BLANK_ID;
	uint64_t frame_depth = 0;
	// derived from start frame ID (easy lookup)
	uint8_t state_format = 0;
	uint64_t codec_state_ref = 0;

	tv_audio_prop_t audio_prop;
public:
	GET_SET(state_ptr, void*);
	GET_SET(audio_prop, tv_audio_prop_t);
	id_t_ get_start_frame_id();
	void set_start_frame_id(id_t_ id_);
	void inc_frame_depth(){frame_depth++;}
	uint8_t get_state_format(){return state_format;}
	uint64_t get_codec_state_ref(){return codec_state_ref;}
};

/*
  Wrappers for functions all state codecs ought to have
 */

#define CODEC_ENCODE_INIT_STATE(codec) tv_transcode_encode_state_t codec##_encode_init_state(tv_audio_prop_t audio_prop)
#define CODEC_ENCODE_SAMPLES(codec) std::vector<std::vector<uint8_t> > codec##_encode_samples(tv_transcode_encode_state_t *state, std::vector<uint8_t> sample_set, uint32_t sampling_freq, uint8_t bit_depth, uint8_t channel_count)
#define CODEC_ENCODE_CLOSE_STATE(codec) void codec##_encode_close_state(tv_transcode_encode_state_t *state)

#define CODEC_DECODE_INIT_STATE(codec) tv_transcode_decode_state_t codec##_decode_init_state(tv_audio_prop_t audio_prop)
#define CODEC_DECODE_PACKET(codec) std::vector<std::vector<uint8_t> > codec##_decode_packet(tv_transcode_decode_state_t *state, std::vector<std::vector<uint8_t> > packet, uint32_t *sampling_freq, uint8_t *bit_depth, uint8_t *channel_count)
#define CODEC_DECODE_CLOSE_STATE(codec) void codec##_decode_close_state(tv_transcode_decode_state_t *state)

/*
  TODO: make a generic interface for stripping containers from the files.
  OGG Opus is the main one right now, but I can make that inline for now in
  the demo function and create an abstraction layer later...
 */

template <typename T>
void state_sanity_check(T state){
	if(state == nullptr){
		print("state is a nullptr", P_ERR);
	}
	if(state->get_state_ptr() == nullptr){
		print("state_ptr is a nullptr",P_ERR);
	}
}

/*
  Raw means actual raw data, discrepencies are because I waited too long to 
  change the name to something that makes sense
 */

/*
  Codecs are returned as a 2D vector, each individual entry is one
  packet. The first entry (as it is now) is probably the start frame,
  and the last entry is the end frame. No codec exists outside of 
  a set of functions beyond tv_frame_audio_t, so we don't really
  need to define an easy to interpret start and end frame flag system.
 */

namespace transcode{
	namespace audio{
		namespace frames{
			std::vector<id_t_> to_frames(
				std::vector<id_t_> frame_set,
				tv_audio_prop_t *output_audio_prop);
			std::vector<std::vector<uint8_t> > to_codec(
				std::vector<id_t_> frame_set,
				tv_audio_prop_t *output_audio_prop);
			
		};
		namespace codec{
			std::vector<id_t_> to_frames(
				std::vector<std::vector<uint8_t> > codec_set,
				tv_audio_prop_t *input_audio_prop,
				tv_audio_prop_t *output_audio_prop);
			std::vector<std::vector<uint8_t> > to_codec(
				std::vector<std::vector<uint8_t> > codec_set,
				tv_audio_prop_t *input_audio_prop,
				tv_audio_prop_t *output_audio_prop);
			std::vector<uint8_t> to_raw(
				std::vector<std::vector<uint8_t> > codec_set,
				tv_audio_prop_t *input_audio_prop,
				tv_audio_prop_t *output_audio_prop);
		};
		namespace raw{
			std::vector<std::vector<uint8_t> > to_codec(
				std::vector<uint8_t> raw_set,
				uint32_t sampling_freq,
				uint8_t bit_depth,
				uint8_t channel_count,
				tv_audio_prop_t *output_audio_prop);
			/*
			  When repacketizing is popular among all codecs, there
			  should be minimal overheads in just calling that from
			  an external function
			 */
		};
		
	};
};

extern std::vector<tv_transcode_encode_state_t> transcode_encode_state_vector;
extern std::vector<tv_transcode_decode_state_t> transcode_decode_state_vector;

#include "tv_transcode_wav.h"
#include "tv_transcode_opus.h"

#endif
