#ifndef TV_TRANSCODE_OPUS_H
#define TV_TRANSCODE_OPUS_H

#include "tv_transcode.h"

extern CODEC_FRAME_TO_FRAME(opus);
extern CODEC_FRAME_TO_RAW(opus);
extern CODEC_RAW_TO_FRAME(opus);
extern CODEC_RAW_TO_RAW(opus);
extern CODEC_REPACKETIZE_BY_TIME(opus);
extern CODEC_REPACKETIZE_BY_SIZE(opus);


#endif
