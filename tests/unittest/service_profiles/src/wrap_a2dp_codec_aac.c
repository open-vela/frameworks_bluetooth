/* Compile a2dp_codec_aac.c with static functions exposed for testing.
 * Block heavy headers that are not needed. */

#define static
#define __A2DP_CODEC_H__
#define __A2DP_SOURCE_AUDIO_H__
#include "../../../../service/profiles/a2dp/codec/a2dp_codec_aac.c"
