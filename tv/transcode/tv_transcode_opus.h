#ifndef TV_TRANSCODE_OPUS_H
#define TV_TRANSCODE_OPUS_H

#include "tv_transcode.h"
#include "tv_transcode_state.h"

#include "opus/opus.h"
#include "opus/opusfile.h"

extern tv_transcode_encode_state_t *opus_encode_init_state(tv_audio_prop_t*);
extern std::vector<std::vector<uint8_t> > opus_encode_samples_to_snippets(tv_transcode_encode_state_t *, std::vector<uint8_t>*, uint32_t, uint8_t, uint8_t);
extern void opus_encode_close_state(tv_transcode_encode_state_t*);

extern tv_transcode_decode_state_t *opus_decode_init_state(tv_audio_prop_t*);
extern std::vector<uint8_t> opus_decode_snippets_to_samples(tv_transcode_decode_state_t *, std::vector<std::vector<uint8_t> >*, uint32_t*, uint8_t*, uint8_t*);
extern void opus_decode_close_state(tv_transcode_decode_state_t*);

// TODO: standardize mins and maxes between all codecs as well?

#define OPUS_MAX_PACKET_SIZE (120*48*255)

#endif
