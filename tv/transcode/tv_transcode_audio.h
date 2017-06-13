#ifndef TV_TRANSCODE_AUDIO_H
#define TV_TRANSCODE_AUDIO_H

extern tv_transcode_state_encode_codec_t encode_codec_lookup(uint8_t format);
extern tv_transcode_state_decode_codec_t decode_codec_lookup(uint8_t format);

extern void audio_prop_sanity_check(tv_audio_prop_t);

#endif
