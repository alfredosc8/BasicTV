#include "../../main.h" // VERSION_*
#include "../../util.h"
#include "net_proto.h"
#include "net_proto_meta.h"

#define WRITE_DATA_META(data_) retval.insert(retval.end(), &data_, &data_+1)

std::vector<uint8_t> net_proto_write_packet_metadata(
        net_proto_standard_data_t data){
	std::vector<uint8_t> retval;
	convert::nbo::to(
		data.peer_id.data(), data.peer_id.size());
	retval.insert(
		retval.end(),
		data.peer_id.begin(),
		data.peer_id.end());
	WRITE_DATA_META(data.ver_major);
	WRITE_DATA_META(data.ver_minor);
	WRITE_DATA_META(data.ver_patch);
	WRITE_DATA_META(data.macros);
	data.unused = 0; // defined as always true
	WRITE_DATA_META(data.unused);
	return retval;
}


#define READ_DATA_META(ptr)						\
	if(ptr != nullptr){						\
		P_V(sizeof(*ptr), P_NOTE);				\
		P_V(data.size(), P_NOTE);				\
		if(sizeof(*ptr) > data.size()){				\
			print("metadata is too short", P_ERR);		\
		}else{							\
			memcpy(ptr, data.data(), sizeof(*ptr));		\
			data.erase(data.begin(),			\
				   data.begin()+sizeof(*ptr));		\
		}							\
	}								\

/*
  New system is using a general escape to encapsulate the standard data and
  the payload independently, so the stripping of control characters is done
  automatically and we can read it fine starting from zero
 */

void net_proto_read_packet_metadata(std::vector<uint8_t> data,
				    net_proto_standard_data_t *standard_data){
	READ_DATA_META(&(standard_data->peer_id));
	convert::nbo::to(
		&(standard_data->peer_id[0]), sizeof(standard_data->peer_id));
	READ_DATA_META(&(standard_data->ver_major));
	READ_DATA_META(&(standard_data->ver_minor));
	READ_DATA_META(&(standard_data->ver_patch));
	READ_DATA_META(&(standard_data->macros));
	READ_DATA_META(&(standard_data->unused));
	if(standard_data->ver_major != VERSION_MAJOR ||
	   standard_data->ver_minor != VERSION_MINOR ||
	   standard_data->ver_patch != VERSION_REVISION){
		// worst case scenario is the unused data isn't used
		// sane defaults should be chosen for a zero
		print("decoding metadata from a different version", P_WARN);
		P_V(standard_data->ver_major, P_WARN);
		P_V(standard_data->ver_minor, P_WARN);
		P_V(standard_data->ver_patch, P_WARN);
		P_V(VERSION_MAJOR, P_WARN);
		P_V(VERSION_MINOR, P_WARN);
		P_V(VERSION_REVISION, P_WARN);
	}
	if(standard_data->unused != 0){
		print("unused space is being used, check for a new version", P_ERR);
	}
}
