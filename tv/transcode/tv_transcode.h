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

#include "../../state.h"

#include "tv_transcode_state.h"

/*
  Codecs are returned as a 2D vector, each individual entry is one
  packet. The first entry (as it is now) is probably the start frame,
  and the last entry is the end frame. No codec exists outside of 
  a set of functions beyond tv_frame_audio_t, so we don't really
  need to define an easy to interpret start and end frame flag system.
 */

struct tv_transcode_state_encode_codec_t{
private:
	uint8_t format = 0;
public:
	GET_SET(format, uint8_t);
	tv_transcode_state_encode_codec_t(
		uint8_t format_,
		tv_transcode_encode_state_t* (*encode_init_state_)(tv_audio_prop_t*),
		std::vector<std::vector<uint8_t> > (*encode_sample_vector_to_snippet_vector_)(tv_transcode_encode_state_t*, std::vector<std::vector<uint8_t> >, uint32_t, uint8_t, uint8_t),
		void (*encode_close_state_)(tv_transcode_encode_state_t *)){
		format = format_;
		encode_init_state =
			encode_init_state_;
		encode_sample_vector_to_snippet_vector =
			encode_sample_vector_to_snippet_vector_;
		encode_close_state =
			encode_close_state_;
	}
	tv_transcode_encode_state_t* (*encode_init_state)(tv_audio_prop_t*) = nullptr;
	std::vector<std::vector<uint8_t> > (*encode_sample_vector_to_snippet_vector)(tv_transcode_encode_state_t*, std::vector<std::vector<uint8_t> >, uint32_t, uint8_t, uint8_t) = nullptr;
	void (*encode_close_state)(tv_transcode_encode_state_t *) = nullptr;
};

struct tv_transcode_state_decode_codec_t{
private:
	uint8_t format = 0;
public:
	GET_SET(format, uint8_t);
	tv_transcode_state_decode_codec_t(
		uint8_t format_,
		tv_transcode_decode_state_t* (*decode_init_state_)(tv_audio_prop_t*),
		std::vector<std::vector<uint8_t> > (*decode_snippet_vector_to_sample_vector_)(tv_transcode_decode_state_t*, std::vector<std::vector<uint8_t> >, uint32_t*, uint8_t*, uint8_t*),
		void (*decode_close_state_)(tv_transcode_decode_state_t *)){
		format = format_;
		decode_init_state =
			decode_init_state_;
		decode_snippet_vector_to_sample_vector =
			decode_snippet_vector_to_sample_vector_;
		decode_close_state =
			decode_close_state_;
	}
	tv_transcode_decode_state_t* (*decode_init_state)(tv_audio_prop_t*) = nullptr;
	std::vector<std::vector<uint8_t> > (*decode_snippet_vector_to_sample_vector)(tv_transcode_decode_state_t*, std::vector<std::vector<uint8_t> >, uint32_t*, uint8_t*, uint8_t*) = nullptr;
	void (*decode_close_state)(tv_transcode_decode_state_t *) = nullptr;
};

/*
  As long as some linkage can exist between the frame functions, the codec
  functions, and the raw samples, we should be good. 
 */

namespace transcode{
	namespace audio{
		namespace frames{
			std::vector<id_t_> to_frames(
				std::vector<id_t_> frame_set,
				tv_audio_prop_t *output_audio_prop,
				uint64_t frame_duration_micro_s);
			std::vector<std::vector<uint8_t> > to_codec(
				std::vector<id_t_> frame_set,
				tv_audio_prop_t *output_audio_prop);
		};
		namespace codec{
			std::vector<id_t_> to_frames(
				std::vector<std::vector<uint8_t> > codec_set,
				tv_audio_prop_t *input_audio_prop,
				tv_audio_prop_t *output_audio_prop,
				uint64_t frame_duration_micro_s);
			std::vector<std::vector<uint8_t> > to_codec(
				std::vector<std::vector<uint8_t> > codec_set,
				tv_audio_prop_t *input_audio_prop,
				tv_audio_prop_t *output_audio_prop);
			std::vector<std::vector<uint8_t> > to_raw(
				std::vector<std::vector<uint8_t> > codec_set,
				tv_audio_prop_t *input_audio_prop,
				uint32_t *sampling_freq,
				uint8_t *bit_depth,
				uint8_t *channel_count);
		};
		namespace raw{
			std::vector<std::vector<uint8_t> > to_codec(
				std::vector<std::vector<uint8_t> > raw_set,
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

/*
  All encoding and decoding states are in one vector for easy garbage collecion
 */

#include "tv_transcode_wav.h"
#include "tv_transcode_opus.h"

#endif
