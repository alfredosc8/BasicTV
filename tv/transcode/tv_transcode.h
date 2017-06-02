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
  tv_transcode.h

  Interface for encoding and decoding functions

  Encoding functions return the encoded data, while the decoding functions
  return the raw samples, simple enough.

  The core transcoding functions don't directly work with tv_frame_*_t, since
  the applications of transcoding the stream data can often fall outside of
  those (exporting to disk, loading from disk, etc.)
*/

/*
  HERE IS HOW IT IS GOING TO WORK

  frames and raw are just two different type of containers, so we are going
  to treat them as containers and not make specific calls to different
  encoding schemes (that's taken care of internally).

  The entire AV section isn't needed right now, since BasicTV works
  internally with seperate audio and video streams running in parallel.
  AV should be written for exporting and importing.

  Any tv_frame_*_t type has associated properties with it, so there is no
  need to pass any along with it. The encoded audio may not have enough
  details as we would like, so there is an option to pass tv_*_prop_t to
  the function to allow for more information (DTX, complexity, etc).
  If we don't have tv_*_prop_t for input, that isn't a critical error,
  since the encoded data (should be) freestanding. 

  input_*_prop can be a nullptr, but output_*_prop shouldn't be a nullptr.
  output_*_prop has the valid encoded/decoded settings written to it,
  and it is an important tool for sanity checking. 
*/

/*
  Macros only do audio, but that's for simplicity at this point. All of this
  nonsense should work with video as well
 */

// C can't take < or > as preprocessor tokens, so I'm dedicating a typedef
// to this so GCC can interpret it properly
typedef std::vector<id_t_> id_vector_t;
typedef std::vector<uint8_t> byte_vector_t;
typedef std::vector<std::vector<uint8_t> > byte_vector_vector_t;

#define CODEC_FRAME_TO_FRAME(codec) id_vector_t codec##_frame_to_frame(id_vector_t frame_set, tv_audio_prop_t *output_audio_prop)
#define CODEC_FRAME_TO_RAW(codec) byte_vector_t codec##_frame_to_raw(id_vector_t frame_set, tv_audio_prop_t *output_audio_prop)

#define CODEC_RAW_TO_FRAME(codec) id_vector_t codec##_raw_to_frame(byte_vector_t raw, tv_audio_prop_t *input_audio_prop, tv_audio_prop_t *output_audio_prop)
#define CODEC_RAW_TO_RAW(codec) byte_vector_t codec##_raw_to_raw(byte_vector_t raw, tv_audio_prop_t *input_audio_prop, tv_audio_prop_t *output_audio_prop)

/*
  These functions are not strictly compliant with their parameters. Opus
  (i'm somewhat sure) cannot packetize by size, only by time. Some nitty
  gritty math is done to get the best packet duration in milliseconds that
  would give roughly the proper size.
 */

#define CODEC_REPACKETIZE_BY_TIME(codec) byte_vector_vector_t codec##_repacketize_by_time(byte_vector_t raw, uint64_t time_micro_s)
#define CODEC_REPACKETIZE_BY_SIZE(codec) byte_vector_vector_t codec##_repacketize_by_size(byte_vector_t raw, uint64_t size_bytes)


/*
  Planned audio codec support (listed by desired popularity):
  Opus
  Codec2 (excited for this one) 
  FLAC
  MP3
  AAC
  Vorbis
  Raw (intermediary only, not for storage/transmission)

  Planned video codec support (listed by desired popularity):
  VP9
  HEVC (h.265)
  h.264
 */
// raw here means encoded
namespace transcode{
	namespace audio{
		namespace frames{
			std::vector<id_t_> to_frames(
				std::vector<id_t_> frame_set,
				tv_audio_prop_t *output_audio_prop);
			std::vector<uint8_t> to_raw(
				std::vector<id_t_> frame_set,
				tv_audio_prop_t *output_audio_prop);
			
		};
		namespace raw{
			std::vector<id_t_> to_frames(
				std::vector<uint8_t> raw_set,
				tv_audio_prop_t *input_audio_prop,
				tv_audio_prop_t *output_audio_prop);
			std::vector<uint8_t> to_raw(
				std::vector<uint8_t> raw_set,
				tv_audio_prop_t *input_audio_prop,
				tv_audio_prop_t *output_audio_prop);
		};
	};
	namespace video{
		namespace frames{
			std::vector<id_t_> to_frames(
				std::vector<id_t_> frame_set,
				tv_video_prop_t *output_video_prop);
			std::vector<uint8_t> to_raw(
				std::vector<id_t_> frame_set,
				tv_video_prop_t *output_video_prop);
			
		};
		namespace raw{
			std::vector<id_t_> to_frames(
				std::vector<uint8_t> raw_set,
				tv_video_prop_t *input_video_prop,
				tv_video_prop_t *output_video_prop);
			std::vector<uint8_t> to_raw(
				std::vector<uint8_t> raw_set,
				tv_video_prop_t *input_video_prop,
				tv_video_prop_t *output_video_prop);
		};
	};
	namespace av{
		namespace frames{
			std::vector<id_t_> to_frames(
				std::vector<id_t_> audio_frame_set,
				tv_audio_prop_t *output_audio_prop,
				std::vector<id_t_> video_frame_set,
				tv_video_prop_t *output_video_prop);
			std::vector<uint8_t> to_raw(
				std::vector<id_t_> audio_frame_set,
				tv_audio_prop_t *output_audio_prop,
				std::vector<id_t_> video_frame_set,
				tv_video_prop_t *output_video_prop);
			
		};
		namespace raw{
			std::vector<id_t_> to_frames(
				std::vector<uint8_t> raw_set,
				tv_audio_prop_t *input_audio_prop,
				tv_video_prop_t *input_video_prop,
				tv_audio_prop_t *output_audio_prop,
				tv_video_prop_t *output_video_prop);
			std::vector<uint8_t> to_raw(
				std::vector<uint8_t> raw_set,
				tv_audio_prop_t *input_audio_prop,
				tv_video_prop_t *input_video_prop,
				tv_audio_prop_t *output_audio_prop,
				tv_video_prop_t *output_video_prop);
		};
	};
};

#endif
