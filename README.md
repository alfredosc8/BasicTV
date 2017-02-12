Join the IRC chat at #basictv on Freenode [here](http://webchat.freenode.net/?channels=#basictv)

Pretty rough around the edges, not a working testnet yet, but it'll be working okay soon enough (2/11/17)

#BasicTV

##What is it?
BasicTV is an decentralized and anonymous Internet TV system. The entire protocol can be ran over the Tor network. Anybody can create and stream content to the network, and each node securely sends stream information to other nodes (on an opt-out basis).

##Nomenclature
An "identity" on the network is a private-public key pair that proves the content came from one person.

An ID is a pair of two numbers, the UUID (intra-identity) and the SHA-256 hash of the public key (inter-identity). Every piece of information sent over the network has an ID associated with it, through a generally accepted type for transport (data_id_t), referenced through a general purpose, type safe, and versatile "pointer" type (id_t_).

##How does it work?
The following is a checklist that a BasicTV node goes through to connect to the network and operate normally:

* Load all private and public keys from the hard disk
* Query the user for which key to use, or to create a new key
* Load my network peer information from disk (net_proto_peer_t), create it if it doesn't exist
* Connect to a node with an advertised open TCP port normally
* Begin routine requests to peers for useful information (other peers, TV channel metadata, public keys)
* Keep connecting to new peers until we are within a safe range for reliable and stable performance (using net_con_req_t as needed for TCP holepunching)

Since all information sent over the network is individual, cryptographically secure chunks, information can be hoarded and dished out to the network at will, allowing the entire network to act as a giant DVR.

##How would content be streamed?
A TV channel has to be created (tv_channel_t). A channel can have up to 256 unique streams: audio, video, or captions. These streams are responsible for maintaining different qualities of video, dubbing and subbing in different languages, as well as any future streamable types that might be implemented (Oculus Rift and other VR, 360 degree view (?), and red and blue 3D). Audio channels are handled by the encoding mechanism (Opus).

Currently, the only way to load audio is through WAV/AIFF/OGG files on disk, but live stream transcoding is in the works. There is no compression scheme for video, so broadcasting video is not an option (infrastructure is proven to work through menus and webcam tests).

###Important Information
* To prevent piracy/freebooting, the public key that is tied to your channel information can list websites that contain your public key. This is verified by the end user and detected conflicts can be solved by using this information.
* Bitcoin wallets can be tied to encryption public keys as well, to allow for donating to content creators and individual nodes (on a not yet implemented BTC/GB constant). Transactions are not handled by BasicTV directly, but instead payment request QR codes are rendered at will
* Information can be sent out with a timestamp ahead of the current timestamp, which would allow for extra permeation time
* It will be possible to allow for "pre-broadcasting" information, encrypting raw frame data with a second key, while keeping the linked list entires and metadata in tact, allowing for downloading large amounts of information ahead of a set "live" time, enforce that live time, and allow slower connections to have relatively nice quality content
* The frame information and channel information needs to match, otherwise it is assumed to be fraudulent

##How would content be watched?
All channels are downloaded and contained in the tv_window_t type. This type is responsible for syncing time throughout a channel, as well as positioning information, selecting streams, among other ideas.

There is only one tv_window_t type allowed currently (will improve to a multiplexer system), and this is interfaced with through the console currently. IR remotes will be added in the future.
