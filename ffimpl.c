/* Copyright 2017-2018 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by JohnChang      2017-06-12
 * Modified by 
 *
 * Copyright (c) 2017-2018 Noovo Crop.  All rights reserved
 */

#include "ffimpl.h"
#include "generic_io.h"

int setup_ffmpeg_ctx (ffmpeg_context *ctx, void *op, ff_read_packet funct)
{
	int ret = -1;
	ctx->avio_ctx_buffer_size = DEFAULT_AVIOCTX_BUF;

	ctx->avio_ctx_buffer = av_malloc(ctx->avio_ctx_buffer_size);
	if (!ctx->avio_ctx_buffer) {
		ret = AVERROR(ENOMEM);
		goto error;
	}
	ctx->avio_ctx = avio_alloc_context(ctx->avio_ctx_buffer, ctx->avio_ctx_buffer_size,
                                  0, op, funct, NULL, NULL);
	if (!ctx->avio_ctx) {
		ret = AVERROR(ENOMEM);
		goto error;
	}

	if (!(ctx->fmt_ctx = avformat_alloc_context())) {
		ret = AVERROR(ENOMEM);
		goto error;
	}
	
	ctx->fmt_ctx->pb = ctx->avio_ctx;

	/* register codecs and formats and other lavf/lavc components*/
	av_register_all();

	return 1;

error:
	/* note: the internal buffer could have changed, and be != avio_ctx_buffer */
	if (ctx->avio_ctx) {
		av_freep(&(ctx->avio_ctx->buffer));
		av_freep(&(ctx->avio_ctx));
	}

	return -1;
}

int ff_read_packet_impl (void *opaque, uint8_t *buf, int buf_size)
{
	output_param *param  = (output_param*)(opaque);
	int          read_sz = buf_size;
	int ret;

	while(1) {
		if (!(param->current_buf) || EMPTY(param->current_buf)) {
			if (param->current_buf) {
				ret = release(param->allocator, param->current_buf);
				if (ret < 0) {
					printf("[%s:%s():%d]: realease failed\n", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
			}
			threadmsg msg = {0};
			threadqueue *dataQueue = param->dataQueue;
			ret = thread_queue_get(dataQueue, NULL, &msg); /*blocking wait*/
			if (ret < 0) {
				printf("[%s:%s():%d]: thread_queue_get failed\n", __FILE__, __FUNCTION__, __LINE__);
				break;
			}			
			param->current_buf = (Allocation*)(msg.data);
		} else {
			if (AVAILABLE_DATA_SZ(param->current_buf) >= buf_size) {
				COPY_TO(param->current_buf, buf, buf_size);
				return read_sz;
			} else {
				int cp_sz = AVAILABLE_DATA_SZ(param->current_buf);
				COPY_TO(param->current_buf, buf, cp_sz);
				buf += cp_sz;
				buf_size -= cp_sz;
			}
		}
	}
    return 0;
}
