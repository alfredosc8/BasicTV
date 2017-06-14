#ifndef TV_TRANSCODE_WAV_H
#define TV_TRANSCODE_WAV_H

extern tv_transcode_encode_state_t *wave_encode_init_state(tv_audio_prop_t*);
extern std::vector<std::vector<uint8_t> > wave_encode_samples_to_snippets(tv_transcode_encode_state_t *, std::vector<uint8_t>*, uint32_t, uint8_t, uint8_t);
extern void wave_encode_close_state(tv_transcode_encode_state_t*);

extern tv_transcode_decode_state_t *wave_decode_init_state(tv_audio_prop_t*);
extern std::vector<uint8_t> wave_decode_snippets_to_samples(tv_transcode_decode_state_t *, std::vector<std::vector<uint8_t> >*, uint32_t*, uint8_t*, uint8_t*);
extern void wave_decode_close_state(tv_transcode_decode_state_t*);

#endif
