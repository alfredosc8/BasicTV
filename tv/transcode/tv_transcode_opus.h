#ifndef TV_TRANSCODE_OPUS_H
#define TV_TRANSCODE_OPUS_H

#include "tv_transcode.h"

#include "opus/opus.h"
#include "opus/opusfile.h"

extern CODEC_ENCODE_INIT_STATE(opus);
extern CODEC_ENCODE_SAMPLES(opus);
extern CODEC_ENCODE_CLOSE_STATE(opus);

extern CODEC_DECODE_INIT_STATE(opus);
extern CODEC_DECODE_PACKET(opus);
extern CODEC_DECODE_CLOSE_STATE(opus);


#endif
