Join the IRC chat at #basictv on Freenode [here](http://webchat.freenode.net/?channels=#basictv)

Pretty rough around the edges, not a working testnet yet, but it'll be working okay soon enough (4/30/17)

# BasicTV

## What is it?
BasicTV is an decentralized and anonymous Internet TV system. The entire protocol can be ran over the Tor network. Anybody can create and stream content to the network, and each node securely sends stream information to other nodes (on an opt-out basis).

BasicTV channels can have multiple audio, video, text, and numerical streams.

## Uses of multiple streams
### Video
* Different quality video feeds (source, multiple resolutions and bitrates)
* Live feeds from multiple cameras
* 3D video streams (red and blue)
* Commentary video feeds

### Audio
* Dubbing in different languages
* Seperate channel for commentary
* Different audio bitrates
* Multiple channel streaming

### Text
* Subbing in different languages
* "Exploiting the medium" for jokes
* The deaf

### Numerical
Numerical streams are streams of data samples, mostly from sensors, that are sent out on the network. Numerical streams can have units associated with them and are grouped by device, giving BasicTV the flexibility to do seamless conversions and statistical functions. Visualizations of the data can be generated on the fly and viewed as a local, non-networked, channel.
* Live feeds from sensors from satellites and probes
* Integration with different APIs (GPS)
* Units associatable with number streams
* Auto-deriving missing yet calculatable data
* Math functions (log scale, derivations, integrals, statistical tests)
* Exporting from the network for more advanced math and archiving

## Bitcoin and other Cryptocurrencies
BasicTV has integration with cryptocurrencies, and planned integration with the Bitcoin blockchain directly. Peers on the network, as well as television channels, can have Bitcoin addresses associated with them. Wallets are stored in cryptocurrency wallet sets, allowing for multiple addresses from multiple different cryptocurrencies to be used.

## Storage & Network Model
Everything networked and stored has an ID associated with it, a 41-byte identifier. Containing 8-bytes of UUID, the 32-byte SHA-256 hash of the RSA public key of the owner, and 1 byte for identifying the type of data. 

Everything but public keys are encrypted and compressed when sent using a hybrid RSA-AES compression scheme. Once sent out to the network, the nodes behave similarly to torrents, multicasting data out to people who ask for it (as well as more complex network functions).

A tiered storage model is used, which allows for archiving old and unpopular data onto lower "tiers" of storage, ranked by ease of access and reading/writing restrictions. Most users won't be using tiered storage.

This means, as long as the ID is known to exist, you can rewind indefinitely. You can also take streams off of the network and export them to local files. The age of the file may be taken into account for donations.

## Tor integration
One major highlight of BasicTV is the capability of using Tor efficiently. Tor conventionally creates one "circuit", or connection to the internet, since multiplexing simple webpages isn't efficient.

BasicTV allows for creating multiple Tor circuits and effectively using them for all peer connections. Tor can allow for more connections than just a clearnet connection, as routers tend to drop TCP connections beyond a certain limit (routers don't see connections made inside of a Tor circuit).

## FAQ
Nobody asked any questions yet, but feel free to contact me through GitHub or Tox

BasicTV Tox: 531A523A6A13A721E3973E39FA92DC0E799758410AD8A9597260B3C3B6FC9C3C496E611F7665
