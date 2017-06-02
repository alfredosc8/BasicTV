#include "convert.h"
#include "util.h"
#include "id/id.h"

// no reason why there can't just be convert::nbo::flip with all
// of the different data types

/*
  Instead of always doing things in 8-bit blocks, try and create
  larger blocks of data (16, 32, or 64), but that isn't a concern
  now.
*/

void convert::nbo::to(uint8_t *data, uint64_t size){
#ifdef IS_LITTLE_ENDIAN
	// switch(size){
	// case 2:
	// 	*data = NBO_16(*data);
	// 	return;
	// case 4:
	// 	*data = NBO_32(*data);
	// 	return;
	// case 8:
	// 	*data = NBO_64(*data);
	// 	return;
	// }
	if(size == 1){
		return;
	}
	for(uint64_t i = 0;i < size/2;i++){
		const uint64_t first = i;
		const uint64_t second = size-i-1;
		data[first] ^= data[second];
		data[second] ^= data[first];
		data[first] ^= data[second];		
		// data[first] = second;
		// data[second] = first_data;
	}
#endif
}

void convert::nbo::from(uint8_t *data, uint64_t size){
	to(data, size);
}

std::vector<uint8_t> convert::nbo::to(std::vector<uint8_t> data){
	to(data.data(), data.size());
	return data;
}

std::vector<uint8_t> convert::nbo::from(std::vector<uint8_t> data){
	to(data.data(), data.size());
	return data;
}

std::vector<uint8_t> convert::nbo::to(std::string data){
	return to(std::vector<uint8_t>(
			  data.begin(),
			  data.end()));
}

std::vector<uint8_t> convert::nbo::from(std::string data){
	return to(std::vector<uint8_t>(
			  data.begin(),
			  data.end()));
}

std::string convert::number::to_binary(uint64_t data){
	std::string retval;
	for(uint64_t i = 0;i < 64;i++){
		if((data & 1) != 0){
			retval = "1" + retval;
		}else{
			retval = "0" + retval;
		}
		if((i+1)%8 == 0){
			retval = " " + retval;;
		}
		data >>= 1;
	}
	return retval;
}

// of course, in RGB format (fourth being the BPC, bytes per color)

// mask here is just the first color, it shifts automatically down
// the line for each color (too little control?)
uint64_t convert::color::to(std::tuple<uint64_t, uint64_t, uint64_t, uint8_t> color){
	uint64_t retval = 0;
	const uint8_t bpc = std::get<3>(color);
	retval |= std::get<0>(color);
	retval |= std::get<1>(color) << bpc;
	retval |= std::get<2>(color) << (bpc*2);
	return retval;
}

std::tuple<uint64_t, uint64_t, uint64_t, uint8_t> convert::color::from(uint64_t color, uint8_t bpc){
	std::tuple<uint64_t, uint64_t, uint64_t, uint8_t> retval;
	const uint64_t bpc_mask = (1 >> bpc)-1;
	std::get<0>(retval) = (color >> (bpc*0)) & bpc_mask;
	std::get<1>(retval) = (color >> (bpc*1)) & bpc_mask;
	std::get<2>(retval) = (color >> (bpc*2)) & bpc_mask;
	return retval;
}

std::tuple<uint64_t, uint64_t, uint64_t, uint8_t> convert::color::bpc(std::tuple<uint64_t, uint64_t, uint64_t, uint8_t> color,
								      uint8_t new_bpc){
	const uint8_t old_bpc = std::get<3>(color);
	if(old_bpc == new_bpc){
		return color;
	}
	double mul = 0;
	if(new_bpc > old_bpc && unlikely(new_bpc-old_bpc <= 31)){
		mul = (double)((uint32_t)1 << (new_bpc-old_bpc));
	}else{
		mul = (double)pow(2, new_bpc-old_bpc);
	}
	std::get<0>(color) *= mul;
	std::get<1>(color) *= mul;
	std::get<2>(color) *= mul;
	return color;
}

const static int8_t hex_map[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

static std::string byte_to_hex(uint8_t byte){
	std::string retval;
	retval += std::string(1, hex_map[(byte&0xF0)>>4]);
	retval += std::string(1, hex_map[(byte&0x0F)]);
	return retval;
}

static uint8_t to_byte_char(int8_t hex){
	switch(hex){
	case '0':
		return 0;
	case '1':
		return 1;
	case '2':
		return 2;
	case '3':
		return 3;
	case '4':
		return 4;
	case '5':
		return 5;
	case '6':
		return 6;
	case '7':
		return 7;
	case '8':
		return 8;
	case '9':
		return 9;
	case 'a':
		return 10;
	case 'b':
		return 11;
	case 'c':
		return 12;
	case 'd':
		return 13;
	case 'e':
		return 14;
	case 'f':
		return 15;
	default:
		break;
	}
	print("invalid hex character " + std::string(1, hex), P_ERR);
	return 0;
}

// TODO: make cross endian

static uint8_t hex_to_byte(std::string hex){
	return (to_byte_char(hex[0]) << 4) | (to_byte_char(hex[1]));
}

/*
  All hexadecimal is in big endian
*/

std::vector<uint8_t> convert::number::from_hex(std::string hex){
	std::vector<uint8_t> retval;
	while(hex[0] == ' '){
		hex = hex.substr(1, hex.size());
	}
	for(uint64_t i = 0;i < (hex.size()/2);i++){
		retval.push_back(
			hex_to_byte(
				hex.substr(
					i*2, 2)));
	}
	retval = convert::nbo::to(retval);
	return retval;
}

std::string convert::number::to_hex(std::vector<uint8_t> byte){
	std::string retval;
	byte = convert::nbo::to(byte);
	for(uint64_t i = 0;i < byte.size();i++){
		retval +=
			byte_to_hex(byte[i]);
	}
	return retval;
}
	
std::string convert::array::id::to_hex(id_t_ id_){
	std::array<uint8_t, 32> hash_array =
		get_id_hash(id_);
	uint64_t uuid_num =
		get_id_uuid(id_);
	std::vector<uint8_t> uuid_vector(
		(uint8_t*)&uuid_num,
		((uint8_t*)&uuid_num)+8);
	if(uuid_vector.size() != 8){
		P_V(uuid_vector.size(), P_WARN);
		print("invalid size for UUID", P_ERR);
	}
	std::string retval =
		convert::number::to_hex(
			uuid_vector) +
		"-" +
		convert::number::to_hex(
			std::vector<uint8_t>(
				hash_array.begin(),
				hash_array.end())) +
		"-" +
		convert::number::to_hex(
			std::vector<uint8_t>({get_id_type(id_)}));
	return retval;
}

#pragma message("convert::array::id::from_hex doesn't sanitize inputs, probably should")

id_t_ convert::array::id::from_hex(std::string id_){
	if(unlikely(id_.size() != 41*2+(2*1))){ // 2*1 for seperators
		P_V(id_.size(), P_WARN);
		print("invalid length for ID", P_ERR);
	}
	id_t_ retval = ID_BLANK_ID;
	std::string uuid_substr =
		id_.substr(0, (8*2));
	std::vector<uint8_t> uuid_raw =
		convert::number::from_hex(uuid_substr);
	if(uuid_raw.size() != 8){
		P_V_S(uuid_substr, P_WARN);
		print("invalid size for UUID", P_ERR);
	}
	set_id_uuid(
		&retval,
		*((uint64_t*)(&(uuid_raw[0]))));
	std::string hash_substr =
		id_.substr(
			16+1,
			(2*32));
	std::vector<uint8_t> hash =
		convert::number::from_hex(
			hash_substr);
	std::array<uint8_t, 32> hash_array;
	memcpy(&(hash_array[0]), &(hash[0]), 32);
	set_id_hash(
		&retval,
		hash_array);
	
	uint8_t tmp_type =
		convert::number::from_hex(
			id_.substr(id_.size()-2, id_.size())).at(0);
	set_id_type(
		&retval,
		tmp_type);
	return retval;
}

#define CONV_CHECK_TYPE(a, b) if(type == a){return b;}

uint8_t convert::type::to(std::string type){
	CONV_CHECK_TYPE("ir_remote_t", TYPE_IR_REMOTE_T);
	CONV_CHECK_TYPE("encrypt_priv_key_t", TYPE_ENCRYPT_PRIV_KEY_T);
	CONV_CHECK_TYPE("encrypt_pub_key_t", TYPE_ENCRYPT_PUB_KEY_T);
	CONV_CHECK_TYPE("console_t", TYPE_CONSOLE_T);
	CONV_CHECK_TYPE("wallet_set_t", TYPE_WALLET_SET_T);
	CONV_CHECK_TYPE("net_proxy_t", TYPE_NET_PROXY_T);
	CONV_CHECK_TYPE("net_proto_peer_t", TYPE_NET_PROTO_PEER_T);
	CONV_CHECK_TYPE("net_proto_socket_t", TYPE_NET_PROTO_SOCKET_T);
	CONV_CHECK_TYPE("net_proto_con_req_t", TYPE_NET_PROTO_CON_REQ_T);
	CONV_CHECK_TYPE("net_proto_linked_list_request_t", TYPE_NET_PROTO_LINKED_LIST_REQUEST_T);
	CONV_CHECK_TYPE("net_proto_id_request_t", TYPE_NET_PROTO_ID_REQUEST_T);
	CONV_CHECK_TYPE("net_proto_type_request_t", TYPE_NET_PROTO_TYPE_REQUEST_T);
	CONV_CHECK_TYPE("net_socket_t", TYPE_NET_SOCKET_T);
	CONV_CHECK_TYPE("net_proxy_t", TYPE_NET_PROXY_T);
	CONV_CHECK_TYPE("tv_channel_t", TYPE_TV_CHANNEL_T);
	CONV_CHECK_TYPE("tv_window_t", TYPE_TV_WINDOW_T);
	CONV_CHECK_TYPE("tv_menu_entry_t", TYPE_TV_MENU_ENTRY_T);
	CONV_CHECK_TYPE("tv_menu_t", TYPE_TV_MENU_T);
	CONV_CHECK_TYPE("tv_dev_audio_t", TYPE_TV_DEV_AUDIO_T);
	CONV_CHECK_TYPE("tv_dev_video_t", TYPE_TV_DEV_VIDEO_T);
	CONV_CHECK_TYPE("tv_frame_audio_t", TYPE_TV_FRAME_AUDIO_T);
	CONV_CHECK_TYPE("tv_frame_video_t", TYPE_TV_FRAME_VIDEO_T);
	CONV_CHECK_TYPE("tv_frame_caption_t", TYPE_TV_FRAME_CAPTION_T);
	CONV_CHECK_TYPE("input_dev_standard_t", TYPE_INPUT_DEV_STANDARD_T);
	CONV_CHECK_TYPE("id_disk_index_t", TYPE_ID_DISK_INDEX_T);
	CONV_CHECK_TYPE("math_number_set_t", TYPE_MATH_NUMBER_SET_T);
	CONV_CHECK_TYPE("tv_item_t", TYPE_TV_ITEM_T);
	print("unknown type has been passed, returning zero", P_CRIT);
	return 0;
}

std::string convert::type::from(uint8_t type){
	switch(type){
	case TYPE_IR_REMOTE_T:
		return "ir_remote_t";
	case TYPE_ENCRYPT_PRIV_KEY_T:
		return "encrypt_priv_key_t";
	case TYPE_ENCRYPT_PUB_KEY_T:
		return "encrypt_pub_key_t";
	case TYPE_CONSOLE_T:
		return "console_t";
	case TYPE_WALLET_SET_T:
		return "wallet_set_t";
	case TYPE_NET_PROXY_T:
		return "net_proxy_t";
	case TYPE_NET_PROTO_SOCKET_T:
		return "net_proto_socket_t";
	case TYPE_NET_PROTO_PEER_T:
		return "net_proto_peer_t";
	case TYPE_NET_PROTO_CON_REQ_T:
		return "net_proto_con_req_t";
	case TYPE_NET_PROTO_LINKED_LIST_REQUEST_T:
		return "net_proto_linked_list_request_t";
	case TYPE_NET_PROTO_ID_REQUEST_T:
		return "net_proto_id_request_t";
	case TYPE_NET_PROTO_TYPE_REQUEST_T:
		return "net_proto_type_request_t";
	case TYPE_NET_SOCKET_T:
		return "net_socket_t";
	case TYPE_TV_CHANNEL_T:
		return "tv_channel_t";
	case TYPE_TV_WINDOW_T:
		return "tv_window_t";
	case TYPE_TV_MENU_ENTRY_T:
		return "tv_menu_entry_t";
	case TYPE_TV_MENU_T:
		return "tv_menu_t";
	case TYPE_TV_DEV_AUDIO_T:
		return "tv_dev_audio_t";
	case TYPE_TV_DEV_VIDEO_T:
		return "tv_dev_video_t";
	case TYPE_TV_FRAME_AUDIO_T:
		return "tv_frame_audio_t";
	case TYPE_TV_FRAME_VIDEO_T:
		return "tv_frame_video_t";
	case TYPE_TV_FRAME_CAPTION_T:
		return "tv_frame_caption_t";
	case TYPE_INPUT_DEV_STANDARD_T:
		return "input_dev_standard_t";
	case TYPE_ID_DISK_INDEX_T:
		return "id_disk_index_t";
	case TYPE_MATH_NUMBER_SET_T:
		return "math_number_set_t";
	case TYPE_TV_ITEM_T:
		return "tv_item_t";
	case 0:
		print("zero type, something went wrong earlier", P_WARN);
		return "";
	default:
		P_V(type, P_WARN);
		print("invalid type, probably malicious (not zero)", P_ERR);
		return "";
	}
}

std::string convert::net::ip::to_string(std::string ip, uint16_t port){
	return ip + ":" + std::to_string(port);
}

std::vector<uint8_t> convert::string::to_bytes(std::string data){
	if(unlikely(data.size() == 0)){
		return {};
	}
	return std::vector<uint8_t>(
		(uint8_t*)data.data(),
		((uint8_t*)data.data())+data.size());
}

std::string convert::string::from_bytes(std::vector<uint8_t> data){
	return std::string((char*)data.data(), data.size());
}
