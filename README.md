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
BasicTV has integration with cryptocurrencies, and planned integration with the Bitcoin blockchain directly. Peers on the network, as well as television channels, can have any number of cryptocurrency addresses associated with them, all stored and referred to in a cryptocurrency wallet set. BasicTV does not currently, or plan to ever, create or handle Bitcoin private keys. Wallets must be imported from another source. 


## Tor and I2P Integration
One major highlight of BasicTV is the capability of using Tor and I2P efficiently. Tor and I2P typically work by creating one connection to the network to load one page. Since BasicTV doesn't follow this monolithic connection system, it can spread the load across multiple circuits and interfaces more efficiently.

## Packet Radio
In a decentralized system, unintentional centralization is a major problem. Operating entirely on the Internet, although having a decentralized topology, can be unreliable and centralized in control. Networking abstractions have been put into place to allow for packet radio modulation schemes such as AFSK 1200 baud and G3RUH 9600 baud, as well as encapsulation schemes for the payload such as AX.25 and FX.25. These speeds, although low, can carry live transmissions. Check your local broadcasting restrictions before use. The 900MHz ISM band is the target frequency band.

| Audio Bitrate (kbps) | Codecs | Minimum Broadcast Delay (seconds)    | Live Packet Radio Mediums |
| -------------------- | ------ | ------------------------------------ | ------------------------  |
| 0.7                  | Codec2 | 0.062                                | AFSK-1200                 |
| 1.3                  | Codec2 | 0.066                                | G3RUH-9600                |
| 2.4                  | Codec2 | 0.077                                | G3RUH-9600                |
| 3.2                  | Codec2 | 0.083                                | G3RUH-9600                |
| 6.0                  | Opus   | 0.153                                | G3RUH-9600                |

The minimum broadcast delay is the theoretical smallest audio segment that can be sent out to the network (including protocol metadata). Codec-specific settings may increase the minimum broadcast delay.

## Storage & Network Model
Everything networked and stored has an ID associated with it, currently a 41-byte identifier (prone to change). Containing 8-bytes of UUID, the 32-byte SHA-256 hash of the RSA public key of the owner, and 1 byte for identifying the type of data. 

Everything but public keys are encrypted and compressed when sent using a hybrid RSA-AES encryption scheme. Once sent out to the network, the nodes behave similarly to torrents, multicasting data out to people who ask for it (as well as more complex network functions).

A tiered storage model is used, which allows for archiving old and unpopular data onto lower "tiers" of storage, ranked by ease of access and reading/writing restrictions. Examples of tiered storage is exporting to tape libraries, optical media, and network attached storage (NAS). Most users won't be using tiered storage, but it remains an integral part.

This means, as long as the ID is known to exist, it can be downloaded. You can also take streams off of the network and export them to local files. The age of the file may be taken into account for donations.


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
Nobody asked any questions yet, but feel free to contact me through Tox, GitHub, or Matrix

TOX: 2604D78720F30EC169CDC951655F9C5577B1CBC317E918D95A693D898316524287D77EDB7ADD
Matrix: @Dako300:matrix.org