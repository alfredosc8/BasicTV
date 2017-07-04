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
	FD_ZERO(&wrfds);
}

net_socket_t::~net_socket_t(){
	disconnect();
}

bool net_socket_t::is_alive(){
	if(socket_fd == -1 ||
	   socket_fd == 0){
		print("non-valid socket_fd", P_NOTE);
		return false;
	}
	struct timeval tout;
	tout.tv_sec = 0;
	tout.tv_usec = 1;
	int select_error =
		select(FD_SETSIZE, NULL, &wrfds, NULL, &tout);
	ASSERT(select_error != -1, P_ERR);
	if(FD_ISSET(get_socket_fd(), &wrfds) == false){
		return false;
	}
	int error = 0;
	socklen_t size = sizeof(error);
	if(getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &size) == -1){
		print("getsockopt failed", P_ERR);
	}
	P_V(error == 0, P_NOTE);
	if(error == 0){
		print("socket is alive", P_DEBUG);
	}else{
		print("socket is dead", P_DEBUG);
	}
	return error == 0;
}

/*
  net_socket_t::send: sends data on the current socket.
  Throws std::runtime_error on exception
 */

void net_socket_t::send(std::vector<uint8_t> data){
	local_outbound_buffer.insert(
		local_outbound_buffer.end(),
		data.begin(),
		data.end());
	if(is_alive()){
		const int64_t sent_bytes =
			::send(socket_fd,
			       (const void*)local_outbound_buffer.data(),
			       (size_t)local_outbound_buffer.size(),
			       MSG_DONTWAIT | MSG_NOSIGNAL);
		if(sent_bytes == -1){
			print("socket is broken, destroying (send failed with: " + GEN_POSIX(errno) + ")", P_DEBUG);
			disconnect();
			print("passing up", P_UNABLE);
		}
		print("sent " + std::to_string(sent_bytes) + " bytes", P_DEBUG);
	}
}

void net_socket_t::send(std::string data){
	std::vector<uint8_t> data_(data.c_str(), data.c_str()+data.size());
	send(data_);
}

std::vector<uint8_t> net_socket_t::recv(uint64_t byte_count, uint64_t flags){
	// TODO: test to see if the activity() code works
	if(is_alive()){
		uint8_t buffer[512];
		do{
			int64_t recv_retval = 0;
			while((recv_retval = ::recv(socket_fd, &(buffer[0]), 512, MSG_DONTWAIT)) > 0){
				local_inbound_buffer.insert(
					local_inbound_buffer.end(),
					&(buffer[0]),
					&(buffer[recv_retval]));
			}
			P_V(local_inbound_buffer.size(), P_NOTE);
			if(recv_retval == -1 &&
			   errno != EAGAIN &&
			   errno != EWOULDBLOCK){
				print("connection error, closing socket (recv failed with: " + GEN_POSIX(errno) + ")", P_NOTE);
			}
			if(local_inbound_buffer.size() >= byte_count){
				auto start = local_inbound_buffer.begin();
				auto end = local_inbound_buffer.begin()+byte_count;
				std::vector<uint8_t> retval =
					std::vector<uint8_t>(start, end);
				local_inbound_buffer.erase(start, end);
				return retval;
			}
		}while(!(flags & NET_SOCKET_RECV_NO_HANG));
	}
	return {};
}

std::vector<uint8_t> net_socket_t::recv_all_buffer(){
	std::vector<uint8_t> retval =
		recv(1, NET_SOCKET_RECV_NO_HANG); // runs input code
	retval.insert(
		retval.end(),
		local_inbound_buffer.begin(),
		local_inbound_buffer.end());
	local_inbound_buffer.clear();
	return retval;
}

/*
  net_socket_t::connect: connect (without SOCKS) to another client
 */

/*
  TODO: get a standardized POSIX error reporting system going (convert.h)
 */

void net_socket_t::connect(){
	if(is_alive()){
		print("already connected", P_WARN);
	}else{
		socket_fd = 
			socket(AF_INET,
			       SOCK_STREAM | SOCK_NONBLOCK,
			       0);
		FD_SET(socket_fd, &wrfds);
		if(socket_fd == -1){
			print("socket failed with error: " + GEN_POSIX(errno), P_ERR);
		}
		int yes = true;
		if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
			print("setsockopt/SO_REUSEADDR failed with error: " + GEN_POSIX(errno), P_ERR);
		}
		yes = true;
		if(setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int)) == -1){
			print("setsockopt/TCP_NODELAY failed with error: " + GEN_POSIX(errno), P_ERR);
		}
		struct sockaddr_in server;
		CLEAR(server);
		server.sin_family =
			AF_INET;
		server.sin_port =
			NBO_16(get_net_port());
		P_V_S(get_net_ip_str(), P_NOTE);
		P_V(get_net_port(), P_NOTE);
		if(get_net_ip_str() == ""){
			server.sin_addr.s_addr =
				INADDR_ANY;
			if(bind(socket_fd, 
				(struct sockaddr*)&server,
				sizeof(server)) == -1){
				print("Bind failed with error: " + GEN_POSIX(errno), P_ERR);
			}
			if(listen(socket_fd, 65536) == -1){
				print("listen failed with error: " + GEN_POSIX(errno), P_ERR);
			}
		}else{
			server.sin_addr.s_addr =
				inet_addr(get_net_ip_str().c_str());
			int32_t connect_error = 
				::connect(socket_fd, (struct sockaddr*)&server, sizeof(server));
			if(connect_error == -1){
				if(errno != EINPROGRESS){
					print("connect failed with error: " + GEN_POSIX(errno), P_ERR);
				}
			}
		}
	}
}

void net_socket_t::disconnect(){
	if(!is_alive()){
		print("already disconnected", P_WARN);
	}else{
		close(socket_fd);
		socket_fd = -1;
	}
}

void net_socket_t::reconnect(){
	disconnect();
	connect();
}

id_t_ net_socket_t::accept(){
	if(get_net_ip_str() != ""){
		print("accepting on a non-inbound socket", P_ERR);
	}
	// if(is_alive()){
		id_t_ retval = ID_BLANK_ID;
		struct sockaddr_in sockaddr_;
		CLEAR(sockaddr_);
		int new_fd = -1;
		socklen_t size_sockaddr = sizeof(sockaddr_);
		if((new_fd = ::accept(
			    get_socket_fd(),
			    (struct sockaddr*)&sockaddr_,
			    &size_sockaddr)) > 0){
			print("accepting a connection into another net_socket_t", P_NOTE);
			int flags = fcntl(new_fd, F_GETFL, 0);
			fcntl(new_fd, F_SETFL, flags & ~O_NONBLOCK);
			// that should be it on POSIX sockets jazz
			
			net_socket_t *socket_ptr =
				new net_socket_t;
			socket_ptr->set_socket_fd(new_fd);
			std::vector<uint8_t> raw_ip;
			raw_ip.insert(
				raw_ip.begin(),
				(uint8_t*)&sockaddr_.sin_addr.s_addr,
				((uint8_t*)&sockaddr_.sin_addr.s_addr)+4);
			raw_ip =
				convert::nbo::from(
					raw_ip);
			socket_ptr->set_net_ip(
				net_interface::ip::raw::to_readable(
					std::make_pair(
						raw_ip,
						NET_INTERFACE_IP_ADDRESS_TYPE_IPV4)),
				NBO_16((uint16_t)sockaddr_.sin_port));
			retval = socket_ptr->id.get_id();
		}else{
			if(errno != EAGAIN &&
			   errno != EWOULDBLOCK &&
			   errno != EINPROGRESS){
				print("accept failing with " + GEN_POSIX(errno), P_ERR);
			}
		}
		return retval;
	// }
	return ID_BLANK_ID;
}

/*
  only used on accepting incoming connections
 */

void net_socket_t::set_socket_fd(int socket_fd_){
	socket_fd = socket_fd_;
}

int net_socket_t::get_socket_fd(){
	return socket_fd;
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
