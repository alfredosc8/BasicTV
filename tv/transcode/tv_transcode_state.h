#ifndef TV_TRANSCODE_STATE_H
#define TV_TRANSCODE_STATE_H

#include "../../util.h"
#include "../tv_frame_audio.h"

struct tv_transcode_encode_state_t{
private:
	void *state_ptr = nullptr;
	id_t_ start_frame_id = ID_BLANK_ID;
	uint64_t frame_depth = 0;
	// derived from start frame ID (easy lookup)
	uint8_t state_format = 0;
	uint64_t codec_state_ref = 0;
	
	tv_audio_prop_t audio_prop;
public:
	GET_SET(state_ptr, void*);
	GET_SET(audio_prop, tv_audio_prop_t);
	id_t_ get_start_frame_id();
	void set_start_frame_id(id_t_ id_);
	void inc_frame_depth(){frame_depth++;}
	uint64_t get_frame_depth(){return frame_depth;}
	uint8_t get_state_format(){return state_format;}
	uint64_t get_codec_state_ref(){return codec_state_ref;}
};

struct tv_transcode_decode_state_t{
private:
	void *state_ptr = nullptr;
	id_t_ start_frame_id = ID_BLANK_ID;
	uint64_t frame_depth = 0;
	// derived from start frame ID (easy lookup)
	uint8_t state_format = 0;
	uint64_t codec_state_ref = 0;

	tv_audio_prop_t audio_prop;
public:
	GET_SET(state_ptr, void*);
	GET_SET(audio_prop, tv_audio_prop_t);
	id_t_ get_start_frame_id();
	void set_start_frame_id(id_t_ id_);
	void inc_frame_depth(){frame_depth++;}
	uint64_t get_frame_depth(){return frame_depth;}
	uint8_t get_state_format(){return state_format;}
	uint64_t get_codec_state_ref(){return codec_state_ref;}
};


extern std::vector<tv_transcode_encode_state_t> encode_state_vector;
extern std::vector<tv_transcode_decode_state_t> decode_state_vector;

#define SEARCH_FOR_STATE(state_type)					\
	tv_transcode_##state_type##_state_t* state_type##_search_for_state( \
		uint8_t state_format_,					\
		uint64_t codec_state_ref_){				\
		for(uint64_t i = 0;i < state_type##_state_vector.size();i++){ \
			if(state_type##_state_vector[i].get_codec_state_ref() == codec_state_ref_ && \
			   state_type##_state_vector[i].get_state_format() == state_format_){ \
				return &state_type##_state_vector[i];	\
			}						\
		}							\
		print("can't find transcoding state", P_ERR);		\
		return nullptr;						\
	}								\


extern void tv_transcode_state_update_to_frame(id_t_ frame_id);

#endif
