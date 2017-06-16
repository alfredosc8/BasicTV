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
  RULES FOR TRANSCODERS:
  
  Sampling frequency, bit depth, and channel count are defined at the
  state initialization, all raw samples coming from this function
  HAVE to match with the tv_audio_prop_t enough for a coupled
  tv_frame_audio_t to work

  Snippets are encoded frames (not saying frames to avoid confusion with
  tv_frame_*_t), and are referred to internally as double byte vectors
  (std::vector<std::vector<uint8_t> >), but these can collapse down
  as needed. Snippets are NOT internally referred to as 1D vectors,
  because it is far easier to segment packets into tv_frame_*_t
  through a 2D byte vector of independent information than a 1D
  byte vector with a (currently) hard to compute length per entry.

  Raw samples are referred to internally as std::vector<uint8_t>, passed
  as pointers to the encoder, wherein the encoded samples are deleted from
  the std::vector<uint8_t> (frame size and sample size discrepencies would
  force us to either drop samples or fill in with nonsense)
 */

/*
  STANDARDS:
  All raw audio samples coming from any decoding function are in unsigned
  system byte order
 */

struct tv_transcode_state_encode_codec_t{
private:
	uint8_t format = 0;
public:
	GET_SET(format, uint8_t);
	tv_transcode_state_encode_codec_t(
		uint8_t format_,
		tv_transcode_encode_state_t* (*encode_init_state_)(tv_audio_prop_t*),
		std::vector<std::vector<uint8_t> >  (*encode_samples_to_snippets_)(tv_transcode_encode_state_t*, std::vector<uint8_t>*, uint32_t, uint8_t, uint8_t),
		void (*encode_close_state_)(tv_transcode_encode_state_t *)){
		format = format_;
		encode_init_state =
			encode_init_state_;
		encode_samples_to_snippets =
			encode_samples_to_snippets_;
		encode_close_state =
			encode_close_state_;
	}

	tv_transcode_encode_state_t* (*encode_init_state)(tv_audio_prop_t*) = nullptr;
	std::vector<std::vector<uint8_t> > (*encode_samples_to_snippets)(tv_transcode_encode_state_t*, std::vector<uint8_t>*, uint32_t, uint8_t, uint8_t) = nullptr;
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
		std::vector<uint8_t> (*decode_snippets_to_samples_)(tv_transcode_decode_state_t*, std::vector<std::vector<uint8_t> >*, uint32_t*, uint8_t*, uint8_t*),
		void (*decode_close_state_)(tv_transcode_decode_state_t *)){
		format = format_;
		decode_init_state =
			decode_init_state_;
		decode_snippets_to_samples =
			decode_snippets_to_samples_;
		decode_close_state =
			decode_close_state_;
	}
	tv_transcode_decode_state_t* (*decode_init_state)(tv_audio_prop_t*) = nullptr;
	std::vector<uint8_t> (*decode_snippets_to_samples)(tv_transcode_decode_state_t*, std::vector<std::vector<uint8_t> >*, uint32_t*, uint8_t*, uint8_t*) = nullptr;
	void (*decode_close_state)(tv_transcode_decode_state_t *) = nullptr;
};

/*
  As long as some linkage can exist between the frame functions, the codec
  functions, and the raw samples, we should be good. 
 */

namespace transcode{
	namespace audio{
		/*
		  We can safely assume that all frames are independent of each
		  other, and they can all be decoded (since they should all
		  be loaded with transcode::audio/video::codec::to_frames or
		  equivalent)
		 */
		namespace frames{
			std::vector<id_t_> to_frames(
				std::vector<id_t_> frame_set,
				tv_audio_prop_t *output_audio_prop,
				uint64_t frame_duration_micro_s);
			std::vector<std::vector<uint8_t> > to_codec(
				std::vector<id_t_> frame_set,
				tv_audio_prop_t *output_audio_prop);
			std::vector<uint8_t> to_raw(
				std::vector<id_t_> frame_set,
				uint32_t *sampling_freq,
				uint8_t *bit_depth,
				uint8_t *channel_count);
		};
		namespace codec{
			std::vector<id_t_> to_frames(
				std::vector<std::vector<uint8_t> > *codec_set,
				tv_audio_prop_t *input_audio_prop,
				tv_audio_prop_t *output_audio_prop,
				uint64_t frame_duration_micro_s);
			std::vector<std::vector<uint8_t> > to_codec(
				std::vector<std::vector<uint8_t> > *codec_set,
				tv_audio_prop_t *input_audio_prop,
				tv_audio_prop_t *output_audio_prop);
			std::vector<uint8_t> to_raw(
				std::vector<std::vector<uint8_t> > *codec_set,
				tv_audio_prop_t *input_audio_prop,
				uint32_t *sampling_freq,
				uint8_t *bit_depth,
				uint8_t *channel_count);
		};
		namespace raw{
			std::vector<std::vector<uint8_t> > to_codec(
				std::vector<uint8_t> *raw_set,
				uint32_t sampling_freq,
				uint8_t bit_depth,
				uint8_t channel_count,
				tv_audio_prop_t *output_audio_prop);
			/*
			  When repacketizing is popular among all codecs, there
			  should be minimal overheads in just calling that from
			  an external function
			 */
			std::vector<uint8_t> signed_to_unsigned(
				std::vector<uint8_t> signed_payload,
				uint8_t bit_depth);
			std::vector<uint8_t> unsigned_to_signed(
				std::vector<uint8_t> unsigned_payload,
				uint8_t bit_depth);
		};
		
	};
};

/*
  All encoding and decoding states are in one vector for easy garbage collecion
 */

#include "tv_transcode_wav.h"
#include "tv_transcode_opus.h"
#include "tv_transcode_audio.h"
#endif
