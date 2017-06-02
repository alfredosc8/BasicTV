#include "tv_transcode.h"

#include "../tv_frame_audio.h"
#include "../tv_frame_video.h"

#include "tv_transcode_opus.h"
#include "tv_transcode_wav.h"

std::vector<id_t_> transcode::audio::frames::to_frames(
	std::vector<id_t_> frame_set,
	tv_audio_prop_t *output_audio_prop){
	std::vector<id_t_> retval;
	if(frame_set.size() == 0){
		print("frame_set is empty, can't encode a blank set", P_WARN);
		return retval;
	}
	if(output_audio_prop == nullptr){
		print("not converting mediums and no output_audio_prop, nothing to do", P_WARN);
		return frame_set;
	}
	switch(output_audio_prop->get_format()){
	case TV_AUDIO_FORMAT_OPUS:
		// retval =
		// 	opus_frame_to_frame(
		// 		frame_set,
		// 		output_audio_prop);
		break;
	case TV_AUDIO_FORMAT_UNDEFINED:
	default:
		print("unknown format", P_ERR);
	}
	return retval;
}

std::vector<uint8_t> transcode::audio::frames::to_raw(
	std::vector<id_t_> frame_set,
	tv_audio_prop_t *output_audio_prop){
	std::vector<uint8_t> retval;
	if(frame_set.size() == 0){
		print("frame_set is empty, can't encode a blank set", P_WARN);
		return retval;
	}
	switch(output_audio_prop->get_format()){
	case TV_AUDIO_FORMAT_OPUS:
		// retval = opus_frame_to_raw(
		// 	frame_set,
		// 	output_audio_prop);
		// break;
	case TV_AUDIO_FORMAT_UNDEFINED:
	default:
		print("unknown format", P_ERR);
	}
	return retval;
}

std::vector<id_t_> transcode::audio::raw::to_frames(
	std::vector<uint8_t> raw_set,
	tv_audio_prop_t *input_audio_prop,
	tv_audio_prop_t *output_audio_prop){
	std::vector<id_t_> retval;
	if(raw_set.size() == 0){
		print("raw_set is empty, can't encode a blank set", P_WARN);
		return retval;
	}
	if(input_audio_prop == nullptr){
		print("input audio prop is a nullptr, this is fine", P_SPAM);
	}
	if(output_audio_prop == nullptr){
		print("output audio prop is a nullptr, this is fine", P_WARN);
	}
	switch(output_audio_prop->get_format()){
	case TV_AUDIO_FORMAT_OPUS:
		// retval = opus_raw_to_frame(
		// 	raw_set,
		// 	input_audio_prop,
		// 	output_audio_prop);
		// break;
	case TV_AUDIO_FORMAT_UNDEFINED:
	default:
		print("unknown format", P_ERR);
	}
	return retval;
}

std::vector<uint8_t> transcode::audio::raw::to_raw(
	std::vector<uint8_t> raw_set,
	tv_audio_prop_t *input_audio_prop,
	tv_audio_prop_t *output_audio_prop){
	std::vector<uint8_t> retval;
	if(raw_set.size() == 0){
		print("raw_set is empty, can't encode a blank set", P_WARN);
		return retval;
	}
	if(input_audio_prop == nullptr){
		print("input audio prop is a nullptr, this isn't fine", P_ERR);
	}
	if(output_audio_prop == nullptr){
		print("output audio prop is a nullptr, this isn't fine", P_ERR);
	}
	switch(output_audio_prop->get_format()){
	case TV_AUDIO_FORMAT_OPUS:
   		// retval = opus_raw_to_raw(
		// 	raw_set,
		// 	input_audio_prop,
		// 	output_audio_prop);
		// break;
	case TV_AUDIO_FORMAT_UNDEFINED:
	default:
		print("unknown format", P_ERR);
	}
	return retval;
}
