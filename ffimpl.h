#ifndef _FFIMPL_H_
#define _FFIMPL_H 1

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>

#define DEFAULT_AVIOCTX_BUF 32*1024

//#define DEFAULT_AVIOCTX_BUF 128*1024

typedef int(*ff_read_packet)(void *op, uint8_t *data, int size);

typedef struct ffmpeg_context {
	AVFormatContext *fmt_ctx;
	AVIOContext *avio_ctx;

	uint8_t *avio_ctx_buffer;
	size_t	avio_ctx_buffer_size;
} ffmpeg_context;

int setup_ffmpeg_ctx (ffmpeg_context *ctx, void *op, ff_read_packet funct);

int ff_read_packet_impl (void *opaque, uint8_t *buf, int buf_size);


#endif
