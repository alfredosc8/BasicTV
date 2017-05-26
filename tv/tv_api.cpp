#include "tv_api.h"
#include "tv_channel.h"
#include "tv_frame_standard.h"
#include "tv_frame_video.h"
#include "tv_frame_audio.h"
#include "tv_window.h"
#include "tv_menu.h"
/*
  tv::chan: channel functions. Does not directly interface with the channels
  themselves, but does operations on the static list (count, next, prev, rand).

  flags:
  TV_CHAN_STREAMING: only use channels that are currently streaming. 
  Streaming is a very broad definition, but should be an independent
  variable inside of the tv_channel_t type
  TV_CHAN_NO_AUDIO: only video only streams
  TV_CHAN_NO_VIDEO: only audio only streams
*/

/*
  tv::chan::count: returns a channel count of all channels. Channels that are
  not currently streaming are co
 */

/*
  We can't be specific about data types since the actual data is inside of
  anothter type. 
 */

id_t_ tv::chan::next_id(id_t_ id, uint64_t flags){
	std::vector<id_t_> all_channels =
		id_api::cache::get("tv_channel_t");
	P_V_S(convert::array::id::to_hex(id), P_VAR);
	return rand_id(flags);
	// id_t_ retval = ID_BLANK_ID;
	// bool swapped_pos = true;
	// while(swapped_pos){
	// 	swapped_pos = false;
	// 	for(uint64_t i = 0;i < all_channels.size();i++){
	// 	}
	// }
}

id_t_ tv::chan::prev_id(id_t_ id, uint64_t flags){
	std::vector<id_t_> all_channels =
		id_api::cache::get("tv_channel_t");
	P_V_S(convert::array::id::to_hex(id), P_VAR);
	return rand_id(flags);
	//return id;
}

id_t_ tv::chan::rand_id(uint64_t flags){
	std::vector<id_t_> channel_id =
		id_api::cache::get("tv_channel_t");
	P_V_B(flags, P_VAR);
	if(channel_id.size() == 0){
		return ID_BLANK_ID;
	}
	uint64_t id_from_start =
		true_rand(0, channel_id.size());
	return channel_id.at(id_from_start);
}

