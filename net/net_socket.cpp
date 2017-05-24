#include "../main.h"
#include "../util.h"
#include "net.h"
#include "net_socket.h"
#include "../id/id_api.h"
#include "../math/math.h"

void net_socket_t::init_create_data_sets(){
	if(PTR_ID(inbound_stat_sample_set_id, ) == nullptr){
		math_number_set_t *inbound_ptr =
			new math_number_set_t;
		inbound_ptr->id.noexp_all_data();
		inbound_ptr->set_dim_count(
			2, {MATH_NUMBER_DIM_NUM, MATH_NUMBER_DIM_NUM});
		inbound_stat_sample_set_id = inbound_ptr->id.get_id();
	}
	if(PTR_ID(outbound_stat_sample_set_id, ) == nullptr){
		math_number_set_t *outbound_ptr =
			new math_number_set_t;
		outbound_ptr->id.noexp_all_data();
		outbound_ptr->set_dim_count(
			2, {MATH_NUMBER_DIM_NUM, MATH_NUMBER_DIM_NUM});
		outbound_stat_sample_set_id = outbound_ptr->id.get_id();
	}
}

net_socket_t::net_socket_t() : id(this, TYPE_NET_SOCKET_T){
	id.add_data_raw(&status, sizeof(status));
	id.noexp_all_data();
	id.nonet_all_data();
	math_number_set_t *outbound_stat_sample_set_ptr =
		new math_number_set_t;
	outbound_stat_sample_set_ptr->id.noexp_all_data();
	math_number_set_t *inbound_stat_sample_set_ptr =
		new math_number_set_t;
	inbound_stat_sample_set_ptr->id.noexp_all_data();
	// data over time, so both numerical
	outbound_stat_sample_set_ptr->set_dim_count(
		2, {MATH_NUMBER_DIM_NUM, MATH_NUMBER_DIM_NUM});
	inbound_stat_sample_set_ptr->set_dim_count(
		2, {MATH_NUMBER_DIM_NUM, MATH_NUMBER_DIM_NUM});
	outbound_stat_sample_set_id = outbound_stat_sample_set_ptr->id.get_id();;
	inbound_stat_sample_set_id = inbound_stat_sample_set_ptr->id.get_id();;
}

net_socket_t::~net_socket_t(){}

/*
  net_socket_t::socket_check: throws if the socket is null. 
  This is the only private function for net_socket_t
 */

void net_socket_t::socket_check(){
	if(socket == nullptr){
		print("socket is null", P_ERR);
		/*
		  Not only because of IP and port data, but also because
		  of guaranteed anonymity.
		 */
	}
}

uint8_t net_socket_t::get_status(){
	socket_check(); // should this be here?
	return status;
}

/*
  net_socket_t::is_alive: returns the status of the socket
 */

bool net_socket_t::is_alive(){
	return socket != nullptr;
}

/*
  net_socket_t::send: sends data on the current socket.
  Throws std::runtime_error on exception
 */

void net_socket_t::send(std::vector<uint8_t> data){
	socket_check();
	const int64_t sent_bytes =
		SDLNet_TCP_Send(socket,
				data.data(),
				data.size());
	if(sent_bytes == -1){
		print("server port mismatch", P_ERR);
	}else if(sent_bytes > 0 && sent_bytes != (int64_t)data.size()){
		print("socket has closed", P_SPAM);
		disconnect();
	}
	print("sent " + std::to_string(sent_bytes) + " bytes", P_DEBUG);
	// std::pair<uint64_t, uint64_t> data_point =
	// 	std::make_pair(
	// 		get_time_microseconds(),
	// 		sent_bytes);
	// math_stat_sample_set_t *outbound_sample_set =
	// 	PTR_DATA(outbound_stat_sample_set_id,
	// 		 math_stat_sample_set_t);
	// if(outbound_sample_set != nullptr){
	// 	outbound_sample_set->add(
	// 	{std::vector<uint8_t>(&data_point.first, &data_point.first+1),
	// 	 std::vector<uint8_t>(&data_point.first, &data_point.first+1)});
	// }
}

void net_socket_t::send(std::string data){
	std::vector<uint8_t> data_(data.c_str(), data.c_str()+data.size());
	send(data_);
}

/*
  net_socket_t::recv: reads byte_count amount of data. Flags can be passed
  NET_SOCKET_RECV_NO_HANG: check for activity on the socket

  This is guaranteed to return maxlen bytes, but the SDLNet_TCP_Recv function
  doesn't share that same behavior, so store all of the information in a local
  buffer and just read from that when recv is called.
 */

static void net_socket_recv_posix_error_checking(int32_t error){
#ifdef __linux
	if(error < 0){
		switch(error){
		case -ENOTCONN:
			print("not connected to socket", P_ERR);
			break;
#if EAGAIN == EWOULDBLOCK
		case -EAGAIN:
#else		       
		case -EAGAIN:
		case -EWOULDBLOCK:
#endif
			print("something something blocking", P_ERR);
			break;
		case -EPERM: // non-blocking socket and no data is received
			break;
		default:
			P_V(error, P_DEBUG);
		}
	}
#endif
}

std::vector<uint8_t> net_socket_t::recv(uint64_t byte_count, uint64_t flags){
	// TODO: test to see if the activity() code works
	uint8_t buffer[512];
	do{
		uint64_t data_received = 0;
		while(activity()){
			int32_t recv_retval = 0;
			if((recv_retval = SDLNet_TCP_Recv(socket, &(buffer[0]), 512)) > 0){
				if(recv_retval <= 0){
					disconnect();
					break;
				}else{
					local_buffer.insert(
						local_buffer.end(),
						&(buffer[0]),
						&(buffer[recv_retval]));
					data_received += recv_retval;
				}
			}else{
				net_socket_recv_posix_error_checking(recv_retval);
			}
		}
		if(data_received != 0){
			math_number_set_t *inbound_stat_sample_set =
				PTR_DATA(inbound_stat_sample_set_id,
					 math_number_set_t);
			uint64_t time_microseconds = get_time_microseconds();
			if(inbound_stat_sample_set != nullptr){
				inbound_stat_sample_set->add_raw_data(
				{math::number::create(
						time_microseconds,
						UNIT(MATH_NUMBER_USE_SI,
						     MATH_NUMBER_BASE_SECOND,
						     MATH_NUMBER_PREFIX_MICRO)),
				 math::number::create(
					 data_received,
					 UNIT(MATH_NUMBER_USE_SI,
					      MATH_NUMBER_BASE_BYTE,
					      0))});
			}
		}
		if(local_buffer.size() >= byte_count){
			auto start = local_buffer.begin();
			auto end = local_buffer.begin()+byte_count;
			std::vector<uint8_t> retval =
				std::vector<uint8_t>(start, end);
			local_buffer.erase(start, end);
			return retval;
		}
	}while(!(flags & NET_SOCKET_RECV_NO_HANG));
	return {};
}

std::vector<uint8_t> net_socket_t::recv_all_buffer(){
	std::vector<uint8_t> retval =
		recv(1, NET_SOCKET_RECV_NO_HANG); // runs input code
	retval.insert(
		retval.end(),
		local_buffer.begin(),
		local_buffer.end());
	local_buffer.clear();
	return retval;
}

/*
  net_socket_t::connect: connect (without SOCKS) to another client
 */

void net_socket_t::connect(){
	IPaddress tmp_ip;
	int16_t res_host_retval = 0;
	if(get_net_ip_str() == ""){
		print("opening a listening socket", P_NOTE);
		res_host_retval = SDLNet_ResolveHost(&tmp_ip,
						     nullptr,
						     get_net_port());
	}else{
		print("opening a standard socket to " +
		      convert::net::ip::to_string(
			      get_net_ip_str(),
			      get_net_port()), P_NOTE);
		res_host_retval =
			SDLNet_ResolveHost(
				&tmp_ip,
				get_net_ip_str().c_str(),
				get_net_port());
	}
	if(res_host_retval == -1){
		print((std::string)"cannot resolve host:"+SDL_GetError(),
		      P_ERR);
	}
	socket = SDLNet_TCP_Open(&tmp_ip);
	if(socket == nullptr){
		print((std::string)"cannot open socket (" + std::to_string(errno) + "):"+SDL_GetError(),
		      P_WARN);
	}else{
		print("opened socket", P_NOTE);
	}
	update_socket_set();
}

void net_socket_t::update_socket_set(){
	socket_set = SDLNet_AllocSocketSet(1);
	SDLNet_TCP_AddSocket(socket_set, socket);
}

void net_socket_t::disconnect(){
	SDLNet_TCP_Close(socket);
	socket = nullptr;
	SDLNet_FreeSocketSet(socket_set);
	socket_set = nullptr;
}

void net_socket_t::reconnect(){
	disconnect();
	connect();
}

/*
  only used on accepting incoming connections
 */

void net_socket_t::set_tcp_socket(TCPsocket socket_){
	socket = socket_;
	IPaddress tmp_ip;
	tmp_ip = *SDLNet_TCP_GetPeerAddress(socket);
	const char *ip_addr_tmp = SDLNet_ResolveIP(&tmp_ip);
	if(ip_addr_tmp == nullptr){
		print("cannot read IP", P_ERR);
		return;
	}
	set_net_ip(ip_addr_tmp,
		   NBO_16(tmp_ip.port));
	update_socket_set();
}

TCPsocket net_socket_t::get_tcp_socket(){
	return socket;
}

bool net_socket_t::activity(){
	if(socket == nullptr){
		print("socket is nullptr", P_WARN);
		return false;
	}
	return SDLNet_SocketReady(socket) != 0;
	//return SDLNet_CheckSockets(socket_set, 0) > 0;
}

void net_socket_t::set_inbound_stat_sample_set_id(id_t_ inbound_stat_sample_set_id_){
	inbound_stat_sample_set_id = inbound_stat_sample_set_id_;
}

id_t_ net_socket_t::get_inbound_stat_sample_set_id(){
	return inbound_stat_sample_set_id;
}

void net_socket_t::set_outbound_stat_sample_set_id(id_t_ outbound_stat_sample_set_id_){
	outbound_stat_sample_set_id = outbound_stat_sample_set_id_;
}

id_t_ net_socket_t::get_outbound_stat_sample_set_id(){
	return outbound_stat_sample_set_id;
}

id_t_ net_socket_t::get_proxy_id(){
	return proxy_id;
}

/*
  Honestly, this shouldn't be a function. Just use net_proto_socket_t, create
  a new socket, and somehow pass proxy information to the connection function
 */

void net_socket_t::set_proxy_id(id_t_ proxy_id_){
	if(proxy_id_ != proxy_id){
		// nothing we can do
		disconnect();
	}
	proxy_id = proxy_id_;
}

/*
  Following two functions add received data to socket inbound data and
  proxy inbound data (if using one), as well as a global throughput stat
  set (doesn't currently exist, probably shouldn't either).
 */

void net_socket_t::register_outbound_data(uint32_t bytes){
	uint64_t time_micro_s = get_time_microseconds();
	const std::vector<std::vector<uint8_t> > stat_sample =
		{math::number::create(time_micro_s, MATH_NUMBER_BASE_SECOND),
		 math::number::create((uint64_t)bytes, MATH_NUMBER_BASE_BYTE)};
	try{
		math_number_set_t *outbound =
			PTR_DATA(outbound_stat_sample_set_id,
				 math_number_set_t);
		if(outbound == nullptr){
			print("there is no outbound stat sample set, creating one", P_WARN);
			init_create_data_sets();
		}
		outbound->add_raw_data(stat_sample);
	}catch(...){
		print("failed to add stat sample to socket outbound stat set", P_WARN);
	}
	try{
		net_proxy_t *proxy_ptr =
			PTR_DATA(get_proxy_id(),
				 net_proxy_t);
		if(proxy_ptr == nullptr){
			print("proxy is a nullptr, assuming one is not used", P_NOTE);
			throw std::runtime_error("proxy stat sample set is a nullptr");
		}
		math_number_set_t *proxy =
			PTR_DATA(proxy_ptr->get_outbound_stat_sample_set_id(),
				 math_number_set_t);
		if(proxy == nullptr){
			print("no stat sample set for proxy, creating one", P_NOTE);
			proxy = new math_number_set_t;
			proxy->set_dim_count(
				2, {MATH_NUMBER_DIM_NUM, MATH_NUMBER_DIM_NUM});
			proxy_ptr->set_outbound_stat_sample_set_id(
				proxy->id.get_id());
		}
		proxy->add_raw_data(stat_sample);
	}catch(...){
		print("failed to add stat sample to proxy outbound stat set", P_NOTE);
	}
}

void net_socket_t::register_inbound_data(uint32_t bytes){
}
