#include "tv_transcode.h"
#include "opus/opus.h"

/*
  Go over this in another commit, I just want to get back on the tree
 */

// static struct tv_transcode_opus_settings_t{
// public:
// 	uint8_t application = 0;
// 	uint8_t complexity = 0;
// 	uint8_t signal = 0;
// 	uint8_t dtx = 0;
// 	uint8_t vbr = 0;
// 	uint8_t vbr_constrained = 0;
// 	uint8_t loss_perc = 0;
// 	uint8_t force_channels = 0;
// 	uint8_t inband_fec = 0;
// 	uint8_t max_bandwidth = 0;
// 	uint32_t bitrate = 0;
// 	uint32_t sampling_freq = 0;
// 	uint8_t channel_count = 0;
// 	uint8_t bit_depth = 0;

// 	tv_audio_prop_t audio_prop_tmp;
	
// 	void read_from_prop(
// 		tv_audio_prop_t audio_prop);
// 	tv_audio_prop_t write_to_prop();
// };


// void tv_transcode_opus_settings_t::read_from_prop(tv_audio_prop_t audio_prop){
// 	const uint64_t flags =
// 		audio_prop.get_flags();
// 	application =
// 		TV_AUDIO_FLAGS_OPUS_APPLICATION(flags);
// 	complexity =
// 		TV_AUDIO_FLAGS_OPUS_COMPLEXITY(flags);
// 	signal =
// 		TV_AUDIO_FLAGS_OPUS_SIGNAL(flags);
// 	dtx =
// 		TV_AUDIO_FLAGS_OPUS_DTX(flags);
// 	vbr =
// 		TV_AUDIO_FLAGS_OPUS_VBR(flags);
// 	vbr_constrained =
// 		TV_AUDIO_FLAGS_OPUS_VBR_CONSTRAINED(flags);
// 	loss_perc =
// 		TV_AUDIO_FLAGS_OPUS_LOSS_PERC(flags);
// 	force_channels =
// 		TV_AUDIO_FLAGS_OPUS_FORCE_CHANNELS(flags);
// 	inband_fec =
// 		TV_AUDIO_FLAGS_OPUS_INBAND_FEC(flags);
// 	bitrate =
// 		audio_prop->get_bit_rate();
// 	bit_depth =
// 		audio_prop->get_bit_depth();
// 	max_bandwidth =
// 		opus_verify_fix_sampling_freq(
// 			audio_prop.get_sampling_freq());
// 	audio_prop_tmp = audio_prop;
// }

// tv_audio_prop_t tv_transcode_opus_settings_t::write_to_prop(){
	
// }

// static void opus_input_audio_prop_sanity_check(tv_audio_prop_t audio_prop){
// 	if(BETWEEN(8, audio_prop.get_sampling_freq(), 48) == false){
// 		print("invalid sampling rate for opus", P_ERR);
// 		// TODO: when raw conversion functions are made, just
// 		// cut the sampling rate in half until we have a freq
// 		// that is in the bounds
// 	}
// 	if(BETWEEN(1, audio_prop.get_channel_count(), 255) == false){
// 		print("invalid number of channels for opus", P_ERR);
// 	}
// }

// static void opus_output_audio_prop_sanity_check(tv_audio_prop_t audio_prop){
// 	opus_output_audio_prop_sanity_check(audio_prop);
// 	if(!(audio_prop->get_flags() & TV_AUDIO_FLAG_OPUS_VBR ||
// 	     BETWEEN(6000, audio_prop->get_bit_rate(), 510000))){
// 		print("invalid bit rate for opus", P_ERR);
// 	}
// }
	
// #pragma message("opus encoder has not been written yet")

// static uint8_t opus_verify_fix_sampling_freq(uint32_t sampling_freq){
// 	switch(sampling_freq){
// 	case 8000:
// 		return OPUS_BANDWIDTH_NARROWBAND;
// 	case 12000:
// 		return OPUS_BANDWIDTH_MEDIUMBAND;
// 	case 16000:
// 		return OPUS_BANDWIDTH_WIDEBAND;
// 	case 24000:
// 		return OPUS_BANDWIDTH_SUPERWIDEBAND;
// 	case 40000:
// 		return OPUS_BANDWIDTH_FULLBAND;
// 	default:
// 		print("invalid opus sampling_freq, using standard", P_WARN);
// 		return OPUS_BANDWIDTH_FULLBAND;
// 	}
// }

// // std::vector<uint8_t> raw
// // tv_audio_prop_t *input_audio_prop (can be null)
// // tv_audio_prop_t *output_audio_prop (can't be null)

// CODEC_RAW_TO_RAW(opus){
// 	std::vector<uint8_t> retval;
// 	if(output_audio_prop == nullptr){
// 		print("having no output_audio_prop only works on frame sets (repacketize)", P_ERR);
// 	}
// 	if(
// 	opus_input_audio_prop_sanity_check();
// 	opus_output_audio_prop_sanity_check();
// 	return retval;
// } 

// #pragme message("opus decoder has not been tested to work yet")

// /*
//   TODO: since frequency, bitrate, and other jargon is stored on a packet
//   by packet basis, just fetch those packets, convert them to raw, do
//   whatever conversions are needed with transcode::convert (only with raw).
//   Convert up lower quality samples (or come to a default output, either
//   is fine).

//   Opus, being a stateful codec, must have all data encoded and decoded
//   at the same time.
//  */

// std::vector<uint8_t> transcode::decode::opus(std::vector<uint8_t> raw_data,
// 					     tv_audio_prop_t *audio_prop){
// 	std::vector<uint8_t> samples;
// 	OpusDecoder *decoder = nullptr;
// 	try{
// 		opus_decoder_create(
// 			TV_AUDIO_DEFAULT_SAMPLING_RATE,
// 			1,
// 			NULL);
// 		if(decoder == nullptr){
// 			print("couldn't create opus decoder", P_ERR);
// 		}
// 		const int32_t frame_size =
// 			opus_decoder_get_nb_samples(
// 				decoder,
// 				raw_data.data(),
// 				raw_data.size());
// 		if(frame_size_retval == OPUS_BAD_ARG){
// 			print("bad arguments for opus_decoder_get_nb_samples", P_ERR);
// 		}else if(frame_size_retval == OPUS_INVALID_PACKET){
// 			print("corrupted opus payload for opus_decoder_get_nb_samples", P_ERR);
// 		}
// 		std::vector<uint8_t> sample_vector(
// 			frame_size, 0);
// 		const int64_t decode_retval =
// 			opus_decode(
// 				decoder,
// 				raw_data.data(),
// 				raw_data.size(),
// 				(opus_int16*)sample_vector.data(),
// 				sample_vector.size(),
// 				0);
// 		if(decode_retval < 0){
// 			print("couldn't decode opus data", P_ERR);
// 		}
// 	}catch(...){
// 		print("opus decoding failed", P_ERR);
// 	}
// 	if(decoder != nullptr){
// 		opus_decoder_destroy(decoder);
// 		decoder = nullptr;
// 	}
// 	return samples;
// }


// // std::vector<uint8_t> raw
// // uint64_t size_bytes

// #pragma message("Opus only segments data chunks into their smallest parts (1276 bytes)")

// CODEC_REPACKETIZE_BY_SIZE(opus){
// 	std::vector<std::vector<uint8_t> > retval;
// 	if(raw.size() == 0){
// 		print("Opus cannot repacketize an empty set", P_ERR);
// 	}
// 	if(size_bytes != 1276){
// 		P_V(size_bytes, P_SPAM);
// 		print("current implementation only supports segmenting in 1276 chunks", P_ERR);
// 	}
// 	OpusRepacketizer *repacketizer =
// 		(OpusRepacketizer*) new uint8_t[opus_repacketizer_get_size());
// 	if(opus_repacketizer_cat(
// 		   repacketizer,
// 		   raw.data(),
// 		   raw.size()) != OPUS_OK){
// 		print("Opus couldn't add data to repacketize", P_ERR);
// 	}else{
// 		const uint64_t frame_count =
// 			opus_repacketizer_get_nb_frames(
// 				repacketizer);
// 		for(uint64_t i = 0;i < frame_count;i++){
// 			const opus_int32 out_len =
// 				opus_repacketizer_out_range(
// 					repacketizer,
// 					i,
// 					i+1,
// 					&tmp_out,
// 					sizeof(tmp_out));
// 			retval.push_back(
// 				std::vector<uint8_t>());
// 			retval[retval.size()-1] =
// 				std::vector<uint8_t>(
// 					&tmp_out,
// 					&tmp_out + out_len);
// 		}
// 	}
// 	return retval;
// }


// CODEC_FRAME_TO_FRAME(opus){
// 	return {};
// }
// CODEC_FRAME_TO_RAW(opus){
// 	return {};
// }
// CODEC_RAW_TO_FRAME(opus){
// 	return {};
// }
// CODEC_RAW_TO_RAW(opus){
// 	return {};
// }
