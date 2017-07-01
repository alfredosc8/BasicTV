#include "../main.h"
#include "../util.h"
#include "net.h"
#include "net_socket.h"
#include "../id/id_api.h"
#include "../math/math.h"

net_socket_t::net_socket_t() : id(this, TYPE_NET_SOCKET_T){
	id.add_data_raw(&status, sizeof(status));
	ID_MAKE_TMP(id.get_id());
	id.set_lowest_global_flag_level(
		ID_DATA_NETWORK_RULE_NEVER,
		ID_DATA_EXPORT_RULE_NEVER,
		ID_DATA_RULE_UNDEF);
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
			const int32_t recv_retval = 
				SDLNet_TCP_Recv(socket, &(buffer[0]), 512);
			if(recv_retval <= 0){
				print("SDLNet_TCP_Recv failed, closing socket: " + (std::string)SDL_GetError(), P_SPAM);
				disconnect();
				break;
			}else{
				local_buffer.insert(
					local_buffer.end(),
					&(buffer[0]),
					&(buffer[recv_retval]));
				data_received += recv_retval;
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
		res_host_retval = SDLNet_ResolveHost(
			&tmp_ip,
			nullptr,
			get_net_port());
	}else{
		print("opening a standard socket to " +
		      get_net_ip_str() + ":" + std::to_string(get_net_port()), P_NOTE);
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
		P_V(get_net_port(), P_WARN);
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
	int activity_ = SDLNet_CheckSockets(socket_set, 0) > 0;
	if(activity_ == -1){
		print("SDLNet_CheckSockets failed:" + (std::string)SDL_GetError(), P_ERR);
	}
	return SDLNet_SocketReady(socket) != 0;
}

id_t_ net_socket_t::get_proxy_id(){
	return proxy_id;
}

void net_socket_t::register_inbound_data(
	uint32_t bytes,
	uint64_t start_time_micro_s,
	uint64_t end_time_micro_s){
	net::stats::add_throughput_datum(
		bytes,
		start_time_micro_s,
		end_time_micro_s,
		id.get_id(),
		proxy_id);
}
