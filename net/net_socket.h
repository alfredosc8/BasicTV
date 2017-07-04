#include "../id/id.h"
#include "../math/math.h"
#include "net_const.h"
#include "net_ip.h"
#include "net_proxy.h"
#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <SDL2/SDL_net.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <fcntl.h>

/*
  net_socket_t: Manages network sockets. Socket is stored inside of this file. 
  Using torsocks should work with this program, as there are no plans for UDP
  packets whatsoever, and this program isn't doing anything too technical.

  TCP sockets should be able to have multiple chunks of information sent over
  them, but vandals and malicious nodes that mis-represent the data should be
  detected and punished somehow (perhaps have a sanity checker for all values
  built into the ID API, so invalid numbers can be detected?).
  
  TODO: implement multiple Tor circuits at one time. It would be great because
  the decentralized nature works better when multiple TCP streams are being used
  at the same time (to spread information, receiving information won't really
  matter after the ID index is used efficiently).
 */

/*
  IP addresses and port numbers shouldn't exist on their own, but instead in
  std::pair entries
 */

struct net_socket_t : public net_ip_t{
private:
	uint8_t status = 0;
	std::vector<uint8_t> local_inbound_buffer;
	std::vector<uint8_t> local_outbound_buffer;
	// raw socket for SDL
	int socket_fd = 0;
	id_t_ proxy_id = ID_BLANK_ID;

	fd_set wrfds;
	
	bool conn_wait = false;
	
	/*
	  inbound is throughput
	  outbound isn't going to be created, instead we are going to name that
	  latency and use ping/pong system
	 */
	void register_inbound_data(
		uint32_t bytes,
		uint64_t start_time_micro_s,
		uint64_t end_time_micro_s);
	void update_socket_set();
public:
	data_id_t id;
	net_socket_t();
	~net_socket_t();

	// general socket stuff
	bool is_alive();
	void connect();
	void disconnect();
	void reconnect();

	id_t_ accept(); // returns a net_socket_t
	
	// send and recv functions
	void send(std::vector<uint8_t> data);
	void send(std::string);
	std::vector<uint8_t> recv(uint64_t byte_count = 0, uint64_t flags = 0);
	std::vector<uint8_t> recv_all_buffer();
	bool activity();

	void set_proxy_id(id_t_ proxy_id_);
	id_t_ get_proxy_id();

	// hacky stuff that should be streamlined and abstracted
	void set_socket_fd(int socket_fd);
	int get_socket_fd();
};
#endif
