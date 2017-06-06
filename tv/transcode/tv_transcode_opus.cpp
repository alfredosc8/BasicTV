#include "tv_transcode.h"
#include "opus/opus.h"

/*
  For simplicity, raw samples don't have a tv_audio_prop_t
 */

#pragma message("assert_opus_input doesn't check beyond format flag, worth fixing?")

static bool assert_fix_opus_prop(tv_audio_prop_t *audio_prop){
	if(audio_prop == nullptr){
		print("no audio prop whatsoever, can't assert", P_ERR);
	}
	if(audio_prop->get_format() != TV_AUDIO_FORMAT_OPUS){
		print("invalid format for opus", P_ERR);
	}
	bool tainted = false;
	if((audio_prop->get_flags() & TV_AUDIO_PROP_FORMAT_ONLY) == false){
		print("TODO: actually check the other variables, don't trust it's correct Opus by format flag", P_WARN);
		if(audio_prop->get_sampling_freq() != 4000*2 ||
		   audio_prop->get_sampling_freq() != 6000*2 ||
		   audio_prop->get_sampling_freq() != 8000*2 ||
		   audio_prop->get_sampling_freq() != 12000*2 ||
		   audio_prop->get_sampling_freq() != 20000*2){
			print("output sampling freq isn't compatiable with Opus, selecting fullband", P_WARN);
			audio_prop->set_sampling_freq(
				20000*2);
			// could do some rounding, but that's not a big deal
			// right now
			tainted = true;
		}
		if(BETWEEN(6000, audio_prop->get_bit_rate(), 510000) == false){
			print("output bit rate isn't compatiable with Opus, selecting 64", P_WARN);
			audio_prop->set_bit_rate(
				64);
			tainted = true;
		}
		// uint16_t keeps GCC from complaining
		// if(BETWEEN(1, audio_prop->get_channel_count(), 255) == false){
		if(audio_prop->get_channel_count() == 0){
			print("invalid channel count for Opus (zero), selecting one", P_WARN);
			audio_prop->set_channel_count(
				1);
			tainted = true;
		}
	}else{
		print("only format is valid, inserting sane defaults", P_NOTE);
		audio_prop->set_sampling_freq(
			20000*2);
		audio_prop->set_bit_rate(
			64);
		audio_prop->set_channel_count(
			1);
		tainted = true;
	}
	return tainted;
}

static void assert_opus_prop(tv_audio_prop_t audio_prop){
	if(assert_fix_opus_prop(&audio_prop)){
		print("assertion on sane opus values failed", P_ERR);
	}
}

/*
  TODO: should make these settings, but that's for another time
 */

static tv_audio_prop_t gen_standard_opus_format(){
	tv_audio_prop_t retval;
	retval.set_sampling_freq(
		48000); // OPUS_BANDWIDTH_FULLBAND
	retval.set_bit_depth(
		16); // good enough
	retval.set_bit_rate(
		65536); // actual bits
	retval.set_channel_count(
		1);
	return retval;
}

CODEC_ENCODE_INIT_STATE(opus){
	tv_transcode_encode_state_t state;
	if(assert_fix_opus_prop(&audio_prop)){
		print("given a bad audio_prop value, using standard Opus for ease of debugging", P_NOTE);
		audio_prop = gen_standard_opus_format();
	}
	int32_t error = 0;
	OpusEncoder *encode_state =
		opus_encoder_create(
			audio_prop.get_sampling_freq(),
			audio_prop.get_channel_count(),
			OPUS_APPLICATION_AUDIO,
			&error);
	if(error != OPUS_OK){
		HANG();
		print("opus failed to create the tv_transcode_encode_t", P_ERR);
	}
	state.set_state_ptr((void*)encode_state);
	state.set_audio_prop(audio_prop);
	return state;
}

CODEC_DECODE_INIT_STATE(opus){
	tv_transcode_decode_state_t state;
	if(assert_fix_opus_prop(&audio_prop)){
		print("given a bad audio_prop value, using standard Opus for ease of debugging", P_NOTE);
		audio_prop = gen_standard_opus_format();
	}
	int32_t error = 0;
	OpusDecoder *decode_state =
		opus_decoder_create(
			audio_prop.get_sampling_freq(),
			audio_prop.get_channel_count(),
			&error);
	if(error != OPUS_OK){
		HANG();
		print("opus failed to create the tv_transcode_decode_t", P_ERR);
	}
	state.set_state_ptr((void*)decode_state);
	state.set_audio_prop(audio_prop);
	return state;
}

CODEC_DECODE_PACKET(opus){
        state_sanity_check(state);
	std::vector<std::vector<uint8_t> > retval;
	opus_int16 pcm_out[5760];
	memset(&(pcm_out[0]), 0, 5760);
	*sampling_freq = 0;
	*bit_depth = 0;
	*channel_count = 0;
	int32_t opus_retval = 0;
	for(uint64_t i = 0;i < packet.size();i++){
		opus_retval =
			opus_decode(
				(OpusDecoder*)state->get_state_ptr(),
				packet[i].data(),
				packet[i].size(),
				&(pcm_out[0]),
				5760,
				0); // FEC
		if(opus_retval < 0){
			break;
		}
		retval.push_back(
			std::vector<uint8_t>(
				&(pcm_out[0]),
				&(pcm_out[0])+opus_retval));
	}
	if(opus_retval > 0){
		print("successfuly decode opus data", P_SPAM);
		*sampling_freq = state->get_audio_prop().get_sampling_freq();
		*bit_depth = state->get_audio_prop().get_bit_depth();
		*channel_count = state->get_audio_prop().get_channel_count();
	}else if(opus_retval == 0){
		print("no opus data to decode", P_WARN);
	}else{
		HANG();
		print("opus decode failed with error code " + std::to_string(opus_retval), P_WARN);
	}
	return retval;
}

CODEC_DECODE_CLOSE_STATE(opus){
	if(state == nullptr){
		print("state is a nullptr", P_ERR);
	}
	if(state->get_state_ptr() == nullptr){
		print("state_ptr is a nullptr", P_ERR);
	}
	opus_decoder_destroy(
		(OpusDecoder*)state->get_state_ptr());
	state->set_state_ptr(
		nullptr);
}

CODEC_ENCODE_SAMPLES(opus){
	state_sanity_check(state);
	std::vector<std::vector<uint8_t> > retval;
	if(bit_depth != 16){
		// not good, should probably fix
		print("not bothering with non-16 bit bit depth", P_ERR);
	}
	P_V(channel_count, P_VAR); // not really needed right now
	const uint64_t frame_size_milli_s = 
		60; // ought to work fine
	const uint64_t inc_val =
		(frame_size_milli_s)*(sampling_freq/1000)*sizeof(opus_int16);
	P_V(inc_val, P_VAR);
	for(uint64_t i = 0;i < sample_set.size()-inc_val;i += inc_val){
		uint8_t encoded_data_tmp[65536];
		const opus_int32 encode_retval =
			opus_encode(
				(OpusEncoder*)state->get_state_ptr(),
				(const opus_int16*)&(sample_set[i]),
				(inc_val/2),
				&(encoded_data_tmp[0]),
				65536);
		if(encode_retval > 0){
			retval.push_back(
				std::vector<uint8_t>(
					&(encoded_data_tmp[0]),
					&(encoded_data_tmp[0])+encode_retval));
		}else if(encode_retval == 0){
			print("no data encoded", P_WARN);
		}else{
			HANG();
			print("opus encode failed with error " + std::to_string(encode_retval), P_ERR);
			/*
			  Opus' state is a shallow copy, so we could back up
			  the state information and re-create it later, but i'm 
			  not sure how common that would be across codecs.
			  It's useful enough for me to look into it at least.

			  I'm fine with cloning tv_transcode_*_state_t with
			  different depths for this sort of use case as
			  a generic state codec function.
			 */
		}
	}
	return retval;
}

CODEC_ENCODE_CLOSE_STATE(opus){
	state_sanity_check(state);
	opus_encoder_destroy(
		(OpusEncoder*)state->get_state_ptr());
	state->set_state_ptr(
		nullptr);
}
