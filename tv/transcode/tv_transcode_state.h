#ifndef TV_TRANSCODE_STATE_H
#define TV_TRANSCODE_STATE_H

#include "../../util.h"
#include "../tv_frame_audio.h"

struct tv_transcode_encode_state_t{
private:
	void *state_ptr = nullptr;
	id_t_ start_frame_id = ID_BLANK_ID;
	id_t_ last_frame_id = ID_BLANK_ID;
	uint64_t frame_depth = 0;
	// derived from start frame ID (easy lookup)
	uint8_t state_format = 0;

	tv_audio_prop_t audio_prop;
public:
	GET_SET(state_ptr, void*);
	GET_SET(audio_prop, tv_audio_prop_t);
	id_t_ get_start_frame_id();
	void set_start_frame_id(id_t_ id_);
	GET_SET_ID(last_frame_id);
	uint8_t get_state_format(){return state_format;}
};

struct tv_transcode_decode_state_t{
private:
	void *state_ptr = nullptr;
	id_t_ start_frame_id = ID_BLANK_ID;
	id_t_ last_frame_id = ID_BLANK_ID;
	uint64_t frame_depth = 0;
	// derived from start frame ID (easy lookup)
	uint8_t state_format = 0;

	tv_audio_prop_t audio_prop;
public:
	GET_SET(state_ptr, void*);
	GET_SET(audio_prop, tv_audio_prop_t);
	id_t_ get_start_frame_id();
	void set_start_frame_id(id_t_ id_);
	GET_SET_ID(last_frame_id);
	uint8_t get_state_format(){return state_format;}
};


extern std::vector<tv_transcode_encode_state_t> encode_state_vector;
extern std::vector<tv_transcode_decode_state_t> decode_state_vector;

// TODO: can optimize by checking size of first vector for a value
// less than 16 (the current search limit)

#define SEARCH_FOR_STATE(state_type)					\
	tv_transcode_##state_type##_state_t* state_type##_search_for_state( \
	        id_t_ frame_id){					\
		for(uint64_t i = 0;i < state_type##_state_vector.size();i++){ \
			std::vector<id_t_> backwards_linked_list =	\
				id_api::linked_list::list::by_distance_until_match( \
					frame_id, -16, state_type##_state_vector[i].get_last_frame_id()); \
			std::vector<id_t_> linked_list =		\
				id_api::linked_list::list::by_distance_until_match( \
					frame_id, 16, state_type##_state_vector[i].get_last_frame_id()); \
			linked_list.insert(				\
				linked_list.begin(),			\
				frame_id);				\
			linked_list.insert(				\
				linked_list.begin(),			\
				backwards_linked_list.begin(),		\
				backwards_linked_list.end());		\
			if(std::find(linked_list.begin(), linked_list.end(), state_type##_state_vector[i].get_last_frame_id()) != linked_list.end()){ \
				return &state_type##_state_vector[i];	\
			}						\
		}							\
		print("can't find transcoding state", P_ERR);		\
		return nullptr;						\
	}								\


extern void tv_transcode_state_update_to_frame(id_t_ frame_id);

#endif
