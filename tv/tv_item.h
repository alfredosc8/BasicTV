#include "../main.h"
#include "../util.h"
#include "../id/id.h"
#ifndef TV_ITEM_H
#define TV_ITEM_H
/*
  TV content is in "items", similar to the TV Guide

  A TV item doesn't NEED a list of all of the IDs, since
  permeating multiple different versions of the same content
  doesn't work well on live streams (there is no way to download
  a diff between two versions of an exported ID, so we either have
  to create one large set that doesn't need to be updated or use
  linked lists for live stream data to prevent (as much) redundant
  downloading (redundant downloading may still happen since a linked
  list can have variable forward and backwards lengths) ).

  Also, I am not fond of changing the ID after creation (except in
  bootstrapping cases), so we can't pre-create a list of IDs and fill
  them in properly afterwards (efficient, but complex).

  TODO: Since there is no valid reason for a non-request ID of hash
  A to refer to an ID of hash B, we can create some ID sanity checks
  to prevent piracy and nonsense problems.
 */


#define TV_ITEM_STREAMING (1 << 0)
#define TV_ITEM_AUDIO (1 << 1)
#define TV_ITEM_VIDEO (1 << 2)
#define TV_ITEM_TEXT (1 << 3)
#define TV_ITEM_NUMBER (1 << 4)

struct tv_item_t{
private:
	// Vector of ID SETS
	// can have variable lengths associated with them
	// being short just checks it against the frame linked list
	std::vector<std::vector<uint8_t> > frame_sets;
	uint64_t start_time_micro_s = 0;
	uint64_t end_time_micro_s = 0;
	// Item wallet takes precedence over tv_channel_t wallet, can
	// be pretty interesting to get donation stats on individual
	// programs vs. the channel itself
	id_t_ wallet_set_id = ID_BLANK_ID;
	id_t_ tv_channel_id = ID_BLANK_ID;
public:
	data_id_t id;
	tv_item_t();
	~tv_item_t();
	GET_SET_ID(wallet_set_id)
	GET_SET_ID(tv_channel_id)
	GET_SET(start_time_micro_s, uint64_t);
	GET_SET(end_time_micro_s, uint64_t);
	
	std::vector<std::vector<id_t_> > get_frame_id_vector();
	void add_frame_id(std::vector<id_t_> stream_id_vector_);
	void clear_frame_sets();

	// metadata from frame sets
	uint64_t count_frame_sets_of_type(type_t_ type);
};
#endif
