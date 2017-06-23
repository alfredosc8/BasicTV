Join the IRC chat at #basictv on Freenode [here](http://webchat.freenode.net/?channels=#basictv) or the Matrix group chat at #BasicTV:matrix.org

# BasicTV

## What is it?
BasicTV is an decentralized and anonymous Internet TV system. The entire protocol can be ran over the Tor network. Anybody can create and stream content to the network, and each node fowards data in a torrenting-like system.

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
* Legitimate documents

### Numerical
Numerical streams are streams of data samples, mostly from sensors, that are sent out on the network. Numerical streams can have units associated with them and are grouped by device, giving BasicTV the flexibility to do seamless conversions and statistical functions. Visualizations of the data can be generated on the fly and viewed as a local, non-networked, channel.
* Live feeds from sensors from satellites and probes
* Integration with different APIs (GPS and Google Maps)
* Units associatable with number streams
* Auto-deriving missing yet calculatable data
* Math functions (log scale, derivations, integrals, statistical tests)
* Exporting from the network for more advanced math and archiving

## Bitcoin and other Cryptocurrencies
BasicTV has integration with cryptocurrencies, and planned integration with the Bitcoin blockchain directly. Peers on the network, as well as television channels, can have Bitcoin addresses associated with them. Wallets are stored in cryptocurrency wallet sets, allowing for multiple addresses from multiple different cryptocurrencies to be used. BasicTV does not currently, or plan to ever, create or handle Bitcoin private keys. Wallets must be imported from another source. 

## Storage & Network Model
Everything networked and stored has an ID associated with it, currently a 41-byte identifier (prone to change). Containing 8-bytes of UUID, the 32-byte SHA-256 hash of the RSA public key of the owner, and 1 byte for identifying the type of data. 

Everything but public keys are encrypted and compressed when sent using a hybrid RSA-AES encryption scheme. Once sent out to the network, the nodes behave similarly to torrents, multicasting data out to people who ask for it (as well as more complex network functions).

A tiered storage model is used, which allows for archiving old and unpopular data onto lower "tiers" of storage, ranked by ease of access and reading/writing restrictions. Examples of tiered storage is exporting to tape libraries, optical media, and network attached storage (NAS). Most users won't be using tiered storage, but it remains an integral part.

This means, as long as the ID is known to exist, it can be downloaded. You can also take streams off of the network and export them to local files. The age of the file may be taken into account for donations.

## Tor and I2P Integration
One major highlight of BasicTV is the capability of using Tor and I2P efficiently. Tor and I2P typically work by creating one connection to the network to load one page. Since BasicTV doesn't follow this monolithic connection system, it can spread the load across multiple circuits and interfaces more efficiently.

## Dependencies
* SDL2
* SDL2_net
* SDL2_mixer
* libcurl
* libvpx
* libopus and libopusfile
* libzstd
* libcrypto

## FAQ
Nobody asked any questions yet, but feel free to contact me through GitHub or Tox

TOX: 2604D78720F30EC169CDC951655F9C5577B1CBC317E918D95A693D898316524287D77EDB7ADD
