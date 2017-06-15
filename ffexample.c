/*
 * Copyright (c) 2014 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * libavformat AVIOContext API example.
 *
 * Make libavformat demuxer access media content through a custom
 * AVIOContext read callback.
 * @example avio_reading.c
 */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>

struct buffer_data {
    uint8_t *ptr;
    size_t size; ///< size left in the buffer
};


static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
	printf("[%s %d] opaque %p\n", __FUNCTION__, __LINE__, opaque, buf_size);

    struct buffer_data *bd = (struct buffer_data *)opaque;
    buf_size = FFMIN(buf_size, bd->size);

    //printf("ptr:%p size:%zu\n", bd->ptr, bd->size);

    /* copy internal buffer data to buf */
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr  += buf_size;
    bd->size -= buf_size;

    return buf_size;
}


static int file_map_ead_packet(void *opaque, uint8_t *buf, int buf_size)
{
	
	struct buffer_data *bd = (struct buffer_data *)opaque;
	buf_size = FFMIN(buf_size, bd->size);

	/* copy internal buffer data to buf */
	memcpy(buf, bd->ptr, buf_size);
	bd->ptr  += buf_size;
	bd->size -= buf_size;

	return buf_size;
}

AVIOContext* CreateFileMapAVIOContext(char *input_filename, void **opaque)
{
	uint8_t *buffer              = NULL;
	uint8_t *avio_ctx_buffer     = NULL;
	size_t  buffer_size          = 0;
	size_t  avio_ctx_buffer_size = 4096;
	AVIOContext *avio_ctx        = NULL;
	struct buffer_data *bd       = NULL;
	int ret                      = 0;

	printf("[%s %d]\n", __FUNCTION__, __LINE__);

	bd = av_mallocz(sizeof(struct buffer_data));
	if (!bd)
		return NULL;

	/* slurp file content into buffer */
	ret = av_file_map(input_filename, &buffer, &buffer_size, 0, NULL);
	if (ret < 0) {
		av_freep(bd);
		return NULL;
	}

	printf("[%s %d] buffer_size %d\n", __FUNCTION__, __LINE__, buffer_size);

	avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
		av_freep(bd);
		av_file_unmap(buffer, buffer_size);
		return NULL;
	}

	/* fill opaque structure used by the AVIOContext read callback */
	bd->ptr  = buffer;
	bd->size = buffer_size;
	
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size,
                                  0, bd, &file_map_ead_packet, NULL, NULL);
    if (!avio_ctx) {
		av_freep(bd);
		av_freep(avio_ctx_buffer);
		av_file_unmap(buffer, buffer_size);
		return NULL;
    }

	*opaque = (void*)(bd);
	return avio_ctx;
}


void ReleaseFileMapAVIOContext(AVIOContext *avio_ctx, void *opaque)
{
	struct buffer_data *bd = (struct buffer_data*)(opaque);

	if (avio_ctx) {
		av_freep(&avio_ctx->buffer);
		av_freep(&avio_ctx);
	}
	av_file_unmap(bd->ptr, bd->size);
}



int main(int argc, char *argv[])
{
    AVFormatContext *fmt_ctx = NULL;
    AVIOContext *avio_ctx = NULL;
    uint8_t *buffer = NULL, *avio_ctx_buffer = NULL;
    size_t buffer_size, avio_ctx_buffer_size = 4096;
    char *input_filename = NULL;
    int ret = 0;
    struct buffer_data bd = { 0 };
	void *opaque;

    if (argc != 2) {
        fprintf(stderr, "usage: %s input_file\n"
                "API example program to show how to read from a custom buffer "
                "accessed through AVIOContext.\n", argv[0]);
        return 1;
    }
    input_filename = argv[1];

    /* register codecs and formats and other lavf/lavc components*/
    av_register_all();

    /* slurp file content into buffer */
    ret = av_file_map(input_filename, &buffer, &buffer_size, 0, NULL);
    if (ret < 0)
        goto end;

    /* fill opaque structure used by the AVIOContext read callback */
    bd.ptr  = buffer;
    bd.size = buffer_size;

	printf("[%s %d] bd.size %d %s\n", __FUNCTION__, __LINE__, bd.size, input_filename);

    if (!(fmt_ctx = avformat_alloc_context())) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size,
                                  0, &bd, &read_packet, NULL, NULL);
    if (!avio_ctx) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    
    avio_ctx = CreateFileMapAVIOContext ("/usr/local/bin/DVB_T_output.ts", &opaque);
    printf("[%s %d] %p\n", __FUNCTION__, __LINE__, avio_ctx);
	if (!avio_ctx) {
		return 0;
	}
    
    fmt_ctx->pb = avio_ctx;

    ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open input\n");
        goto end;
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream information\n");
        goto end;
    }

    av_dump_format(fmt_ctx, 0, input_filename, 0);

end:
    avformat_close_input(&fmt_ctx);
    /* note: the internal buffer could have changed, and be != avio_ctx_buffer */
    if (avio_ctx) {
        av_freep(&avio_ctx->buffer);
        av_freep(&avio_ctx);
    }
    av_file_unmap(buffer, buffer_size);

    if (ret < 0) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}

