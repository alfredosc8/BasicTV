#ifndef STREAM_H
#define STREAM_H
/*
  stream_main.h: somewhat nice attempt to standardize information
  coming to and from the node.

  STANDARD PARAMETERS
 */

// streams out to the internet in some fashion
#define STREAM_MEDIUM_TYPE_INTERNET 1

// streams out to the radio in some fashion
#define STREAM_MEDIUM_TYPE_RADIO 2

#define STREAM_PAYLOAD_AUDIO (1 << 0)
#define STREAM_PAYLOAD_VIDEO (1 << 1)
#define STREAM_PAYLOAD_TEXT (1 << 2)
#define STREAM_PAYLOAD_NUMBER (1 << 3)

/*
  TODO: since this is just a variation of net_proto_interface_state_t, complete
  that first, then reference that as an ID here. 
 */

struct stream_type_t{
private:
	uint8_t medium_type = 0;
	uint8_t allowed_payload = 0;
	uint8_t encoding_type = 0;
public:
};

#endif
