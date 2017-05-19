#include "../../main.h"
#include "../../id/id_api.h"
#include "../../util.h"
#include "../net.h"
#include "../net_socket.h"
#include "net_proto_meta.h"
#include "net_proto_socket.h"
#include "../../id/id_api.h"
#include "../../encrypt/encrypt.h"
#include "../../escape.h"

/*
  New transport model:
  TCP and UDP will both use encapsulation of data provided by escape.cpp
  to handle the payload and the metadata. Each chunk of data should look
  something like this:
  [STANDARD DATA #1] [PAYLOAD #1] [STANDARD DATA #2] [PAYLOAD #2]

  No spaces nor numbers are used, they are purely visual.

  If a payload is received without a corresponding standard data block, it
  is disregarded (TODO: is this good behavior?). 
 */

/*
  TODO: copy this over to outbound too
 */

void net_proto_socket_t::add_id_to_inbound_id_set(id_t_ payload_id){
	math_number_set_t *inbound_ptr =
		PTR_DATA(inbound_id_set_id,
			 math_number_set_t);
	if(inbound_ptr == nullptr){
		print("number set is a nullptr, creating new one (probably bad GC)", P_NOTE);
		init_create_id_sets();
		inbound_ptr =
			PTR_DATA(inbound_id_set_id,
				 math_number_set_t);
	}
	inbound_ptr->add_raw_data(
	{std::vector<uint8_t>((&payload_id[0]), (&payload_id[0])+sizeof(id_t_)),
	 math::number::create(
		 get_time_microseconds(),
		 UNIT(MATH_NUMBER_USE_SI,
		      MATH_NUMBER_BASE_SECOND,
		      MATH_NUMBER_PREFIX_MICRO))});
}

void net_proto_socket_t::init_create_id_sets(){
	/*
	  1st dim: ID
	  2nd dim: timestamp
	 */
	if(PTR_ID(inbound_id_set_id, ) == nullptr){
		math_number_set_t *inbound_id_set_ptr =
			new math_number_set_t;
		inbound_id_set_ptr->id.noexp_all_data();
		inbound_id_set_ptr->set_dim_count(
			2, {MATH_NUMBER_DIM_CAT,
			    MATH_NUMBER_DIM_NUM});
		inbound_id_set_id = inbound_id_set_ptr->id.get_id();
	}
	if(PTR_ID(outbound_id_set_id, ) == nullptr){
		math_number_set_t *outbound_id_set_ptr =
			new math_number_set_t;
		outbound_id_set_ptr->id.noexp_all_data();
		outbound_id_set_ptr->set_dim_count(
			2, {MATH_NUMBER_DIM_CAT,
			    MATH_NUMBER_DIM_NUM});
		outbound_id_set_id = outbound_id_set_ptr->id.get_id();
	}
}

net_proto_socket_t::net_proto_socket_t() : id(this, TYPE_NET_PROTO_SOCKET_T){
	id.add_data_id(&socket_id, 1);
	id.add_data_id(&peer_id, 1);
	id.add_data_raw(&flags, sizeof(flags));
	id.add_data_raw(&last_recv_micro_s, sizeof(last_recv_micro_s));
	id.add_data_one_byte_vector(&working_buffer, ~0);
	id.add_data_id(&inbound_id_set_id, 1);
	id.add_data_id(&outbound_id_set_id, 1);
	init_create_id_sets();
	net_proto_standard_data_t std_data_;
	std_data_.ver_major = VERSION_MAJOR;
	std_data_.ver_minor = VERSION_MINOR;
	std_data_.ver_patch = VERSION_REVISION;
	std_data_.macros = 0;
	std_data_.unused = 0;
	std_data_.peer_id = net_proto::peer::get_self_as_peer();
	std_data =
		net_proto_write_packet_metadata(
			std_data_);
}

net_proto_socket_t::~net_proto_socket_t(){
}

void net_proto_socket_t::update_working_buffer(){
	net_socket_t *socket_ptr =
		PTR_DATA(socket_id,
			 net_socket_t);
	if(socket_ptr == nullptr){
		print("socket is a nullptr", P_ERR);
	}
	std::vector<uint8_t> buffer =
		socket_ptr->recv_all_buffer();
	if(buffer.size() != 0){
		P_V(buffer.size(), P_SPAM);
		last_recv_micro_s = get_time_microseconds();
		working_buffer.insert(
			working_buffer.end(),
			buffer.begin(),
			buffer.end());
	}
}

void net_proto_socket_t::update_block_buffer(){
	std::pair<std::vector<uint8_t>, std::vector<uint8_t> > block_data;
	while((block_data = unescape_vector(
		       working_buffer,
		       NET_PROTO_ESCAPE)).first.size() != 0){
		working_buffer = block_data.second;
		if(block_data.first.size() != 0){
			if(block_buffer.size() == 0){
				block_buffer.push_back(
					std::make_pair(
						std::vector<uint8_t>(),
						std::vector<uint8_t>()));
			}
			if(block_buffer[block_buffer.size()-1].first.size() == 0){
				block_buffer[block_buffer.size()-1].first =
					block_data.first;
			}else if(block_buffer[block_buffer.size()-1].second.size() == 0){
				block_buffer[block_buffer.size()-1].second =
					block_data.first;
			}else{
				block_buffer.push_back(
					std::make_pair(
						block_data.first,
						std::vector<uint8_t>({})));
			}
		}else{
			print("socket appears up to date (no full escaped vectors to read)", P_SPAM);
		}
	}
}

void net_proto_socket_t::send_id(id_t_ id_){
	data_id_t *id_tmp =
		PTR_ID(id_, );
	if(id_tmp == nullptr){
		print("id to send is a nullptr", P_ERR);
	}
	std::vector<uint8_t> payload =
		id_tmp->export_data(
			ID_DATA_NOEXP,
			ID_EXTRA_COMPRESS | ID_EXTRA_ENCRYPT);
	if(payload.size() == 0){
		print("exported size of ID is zero, not sending", P_NOTE);
		return;
	}
	net_socket_t *socket_ptr =
		PTR_DATA(socket_id,
			 net_socket_t);
	if(socket_ptr == nullptr){
		print("socket is a nullptr", P_ERR);
	}
	// can simplify to one vector, not done for debugging reasons
	P_V(payload.size(), P_SPAM);
	std::vector<uint8_t> std_data_postescape =
		escape_vector(std_data, NET_PROTO_ESCAPE);
	std::vector<uint8_t> payload_postescape =
		escape_vector(payload, NET_PROTO_ESCAPE);
	P_V(std_data_postescape.size(), P_SPAM);
	P_V(payload_postescape.size(), P_SPAM);
	socket_ptr->send(std_data_postescape);
	socket_ptr->send(payload_postescape);
}

void net_proto_socket_t::send_id_vector(std::vector<id_t_> id_vector){
	for(uint64_t i = 0;i < id_vector.size();i++){
		send_id(id_vector[i]);
		// TODO: should optimize this somehow...
	}
}

/*
  LOAD_BLOCKS is somehow breaking it
 */

void net_proto_socket_t::load_blocks(){
	/*
	  For now, i'm fine with loading this directly into memory and letting
	  the id_api::destroy take care of exporting it somehow, but I would
	  like to create an index where information can be stored really
	  anywhere, and have a more robust intratransport system for this
	  data (expandable to say giant tape libraries, CD/DVD/BD archvies,
	  in memory, on disk, etc). However, that is for another day.
	 */
	for(uint64_t i = 0;i < block_buffer.size();i++){
		if(block_buffer[i].first.size() != 0 &&
		   block_buffer[i].second.size() != 0){
			/*
			  They have to at least be complete blocks because of
			  the encapsulation of the data blocks being sent and
			  the automatic movement and decapsulation from
			  working_buffer

			  Since link layer encryption isn't a thing right now,
			  i'm not worried about implementing decoding code,
			  although that wouldn't be a bad idea
			 */
			print("found a complete block_buffer set", P_SPAM);
			net_proto_standard_data_t std_data;
			net_proto_read_packet_metadata(
				block_buffer[i].first,
				&std_data);
			if(std_data.peer_id != peer_id){
				print("sent peer ID and current peer ID do not"
				      " match, assume this is a bootstrap", P_NOTE);
				/*
				  Because bootstrapping is creating a new
				  peer ID for the other client, one with 
				  my hash
				 */
				net_proto_peer_t *wrong_peer_ptr =
					PTR_DATA(peer_id,
						 net_proto_peer_t);
				delete wrong_peer_ptr;
				wrong_peer_ptr = nullptr;
				peer_id = std_data.peer_id;
			}
			P_V(block_buffer[i].second.size(), P_SPAM); // temporary
			/*
			  NOTE: this doesn't call id_api::array::add_data, this
			  function calls id_api::cache::add_data, which just
			  stores the data in memory. Doesn't require the public
			  key right away, but it does when it is requested
			 */
			id_api::add_data(
				block_buffer[i].second);
			const id_t_ inbound_id =
				id_api::raw::fetch_id(
					block_buffer[i].second);
			add_id_to_inbound_id_set(
				inbound_id);
			block_buffer.erase(
				block_buffer.begin()+i);
			i--;
		}
	}
}

void net_proto_socket_t::update(){
	update_working_buffer();
	update_block_buffer();
	load_blocks();
}
