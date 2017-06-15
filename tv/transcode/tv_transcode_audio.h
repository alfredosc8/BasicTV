#ifndef TV_TRANSCODE_AUDIO_H
#define TV_TRANSCODE_AUDIO_H

extern tv_transcode_state_encode_codec_t encode_codec_lookup(uint8_t format);
extern tv_transcode_state_decode_codec_t decode_codec_lookup(uint8_t format);
extern void audio_prop_sanity_check(tv_audio_prop_t);
extern void assert_bit_depth(uint8_t bit_depth);

extern void assert_sane_audio_metadata(
	uint32_t sampling_freq,
	uint8_t bit_depth,
	uint8_t channel_count);

extern void assert_compatiable_audio_metadata(
	uint32_t sampling_freq,
	uint8_t bit_depth,
	uint8_t channel_count,
	tv_audio_prop_t state_encode_audio_prop);

extern std::vector<std::vector<uint8_t> > samples_to_chunks_of_length(
	std::vector<uint8_t> *samples,
	uint32_t chunk_size);

extern uint32_t duration_metadata_to_chunk_size(
	uint32_t duration_micro_s,
	uint32_t sampling_freq,
	uint8_t bit_depth,
	uint8_t channel_count);

#endif
