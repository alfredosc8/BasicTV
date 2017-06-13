#ifndef TV_FRAME_AUDIO_H
#define TV_FRAME_AUDIO_H
#include "tv_audio.h"
#include "tv_frame_standard.h"
#include <opus/opus.h>

#define TV_FRAME_AUDIO_DEFAULT_SAMPLING_RATE 48000
#define TV_FRAME_AUDIO_DEFAULT_BIT_DEPTH 24

#define TV_FRAME_AUDIO_DATA_SIZE (TV_FRAME_AUDIO_DEFAULT_SAMPLING_RATE*(TV_FRAME_AUDIO_DEFAULT_BIT_DEPTH/8))

#define TV_FRAME_AUDIO_DEFAULT_FORMAT TV_AUDIO_FORMAT_OPUS

// first 2-bits are reserved for the raw format

/*
  tv_frame_audio_t: snippet of audio

  Since compression is going to be king, then data is just the raw data (unless
  raw is used, then it is samples).

  Sound streams are going to be the first streams on to the network, since
  having uncompressed versions isn't going to saturate the nodes' bandwidth as
  much as uncompressed video will. However, compression is still a very high
  priority.

  I have no plan of using MP3 on the network as a widely supported protocol,
  mainly because of restrictive licensing. Opus is a great alternative (which
  ties in nicely with the planned VP9 compression). I can't imagine switching
  from MP3 to Opus will cause any major loss of quality (if you are concerned
  about quality, convert to Opus from FLAC, if that is an option).
*/

/*
  tv_frame_audio_t should always be unsigned with system byte order. Bit depth
  is the only variable that can change (16 is where SDL2 taps out)
*/

class tv_frame_audio_t : public tv_frame_standard_t{
private:
	std::vector<std::vector<uint8_t> > packet_set;
	tv_audio_prop_t audio_prop;
public:
	data_id_t id;
	tv_frame_audio_t();
	~tv_frame_audio_t();
	GET_SET(audio_prop, tv_audio_prop_t);
	GET_SET(packet_set, std::vector<std::vector<uint8_t> >);
};

#endif
