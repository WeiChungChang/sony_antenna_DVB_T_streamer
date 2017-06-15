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
#include <stdatomic.h>
#include <sys/types.h>

#include "allocation.h"
#include "threadqueue.h"
#include "mmdbg.h"

#include "generic_io.h"
#include "ffimpl.h"
#include "atom_variable.h"

struct buffer_data {
    uint8_t *ptr;
    size_t size; ///< size left in the buffer
};

typedef struct noovo_buffer_data {
	uint8_t *header;
	size_t  capacity; ///< size left in the buffer
	
	uint8_t *ptr;
	size_t size; ///< size left in the buffer
} noovo_buffer_data;

typedef struct buffer_data buffer_data;

typedef struct OpBuf {
	buffer_data *data;
	threadqueue *dataQueue;
} OpBuf;

FILE *fptr = NULL;

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    struct buffer_data *bd = (struct buffer_data *)opaque;
    buf_size = FFMIN(buf_size, bd->size);

    /* copy internal buffer data to buf */
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr  += buf_size;
    bd->size -= buf_size;
	printf("[%s:%s():%d] buf_size %d\n", __FILE__, __FUNCTION__, __LINE__, buf_size);	

	fwrite(buf, buf_size, 1, fptr);	
	fflush(fptr);

    return buf_size;
}

static int read_packet_2 (void *opaque, uint8_t *buf, int buf_size)
{
    OpBuf *opBuf      = (OpBuf*)opaque;
	buffer_data *data = opBuf->data;

	int read_sz = buf_size;

	while(1) {
		printf("[%s:%s():%d] buf_size %d %ld\n", __FILE__, __FUNCTION__, __LINE__, buf_size, data->size);
		if (data->size == 0) {
			threadmsg msg = {0};
			threadqueue *dataQueue = opBuf->dataQueue;
			//printf("[%s:%s():%d] try get %p dataQueue %p\n", __FILE__, __FUNCTION__, __LINE__,  pthread_self(), dataQueue);
			thread_queue_get(dataQueue, NULL, &msg); /*blocking wait*/
			//printf("[%s:%s():%d] done!\n", __FILE__, __FUNCTION__, __LINE__);
			Allocation *allocation = (Allocation*)(msg.data);
			unsigned char *ptr = (unsigned char*)BUFFER_HEAD(allocation);
			data->ptr = ptr;
			printf("[%s:%s():%d] 0x%x 0x%x 0x%x 0x%x\n", __FILE__, __FUNCTION__, __LINE__, ptr[0], ptr[1], ptr[2], ptr[3]);
			data->size = AVAILABLE_DATA_SZ(allocation);
			printf("[%s:%s():%d] get %d %ld\n", __FILE__, __FUNCTION__, __LINE__, buf_size, data->size);
		} else {
			if (data->size >= buf_size) {
				memcpy(buf, data->ptr, buf_size);


				fwrite(buf, buf_size, 1, fptr);	
				fflush(fptr);


				data->ptr  += buf_size;
				data->size -= buf_size;
				printf("[%s:%s():%d] return %d read_sz %d\n", __FILE__, __FUNCTION__, __LINE__, buf_size, read_sz);
				return read_sz;
			} else {
				memcpy(buf, data->ptr, data->size);

				fwrite(buf, data->size, 1, fptr);	
				fflush(fptr);
				
				buf += data->size;
				buf_size -= data->size;
				data->ptr  += data->size;
				data->size = 0;
			}
		}
	}
    return 0;
}


typedef struct _test_param {
	buffer_data *buf_data;
	Allocator   *allocator;
	threadqueue *dataQueue;
} test_param;

void* myTest(void *param) {
	test_param *testParam = (test_param*)param;
	
	Allocator   *allocator = testParam->allocator;
	buffer_data *bd        = testParam->buf_data;
	threadqueue *dataQueue = testParam->dataQueue;
	int          cp_sz     = -1;
#undef printf
	printf("[%s:%s():%d]: bd->size %ld\n", __FILE__, __FUNCTION__, __LINE__, bd->size);

	while(bd->size > 0) {
		Allocation *target = allocate(allocator);
		if (!target) {
			printf("[%s:%s():%d]\n", __FILE__, __FUNCTION__, __LINE__);
			break;
		}
		cp_sz = FFMIN(FREE_SPACE_SZ(target), bd->size);

		//unsigned char* ptr = WRITE_POS(target);

		/* copy internal buffer data to buf */
		//memcpy(IO_POS(target), bd->ptr, cp_sz);
		COPY_TO(target, bd->ptr, cp_sz);
		//printf("[%s:%s():%d]: %d\n", __FILE__, __FUNCTION__, __LINE__, target->offset);

		
		bd->ptr  += cp_sz;
		bd->size -= cp_sz;

		//printf("[%s:%s():%d]: %p 0x%x 0x%x 0x%x 0x%x thread = %p\n", __FILE__, __FUNCTION__, __LINE__, ptr, ptr[0], ptr[1], ptr[2], ptr[3], pthread_self());
		thread_queue_add(dataQueue, (void*)(target), 0/*useless*/);
		long qsz = thread_queue_length(dataQueue);
		printf("[%s:%s():%d]: Q length = %ld thread %ld dataQueue %p\n", __FILE__, __FUNCTION__, __LINE__, qsz,  (long)pthread_self(), dataQueue);
	}

	//atomic_fetch_add(&endOrNot, 1);
	pthread_exit(NULL);
}


int file_map_read_data(void *op, int8_t *dist, int max)
{
	int buf_size = 0;
	noovo_buffer_data *bd = (noovo_buffer_data*)op;

	buf_size = FFMIN(max, bd->size);

	printf("[%s:%s():%d] begin %d, ramain %ld, max = %d\n", __FILE__, __FUNCTION__, __LINE__, buf_size, bd->size, max);

	/* copy internal buffer data to buf */
	memcpy(dist, bd->ptr, buf_size);
	bd->ptr  += buf_size;
	bd->size -= buf_size;	

	printf("[%s:%s():%d] read %d, ramain %ld, max = %d\n", __FILE__, __FUNCTION__, __LINE__, buf_size, bd->size, max);
	return buf_size;
}

int set_file_map_io (char *input_filename, noovo_buffer_data *bd)
{
    /* slurp file content into buffer */
	bd->header = bd->ptr = NULL;
	bd->ptr = -1;
	int ret = av_file_map(input_filename, &(bd->header), &(bd->capacity), 0, NULL);
	if (ret >= 0) { /*a non negative number in case of success*/
		bd->ptr = bd->header;
		bd->size = bd->capacity;
	}
	
	printf("[%s:%s():%d]: %p %ld\n", __FILE__, __FUNCTION__, __LINE__, bd->header, bd->capacity);
    return ret;
}

int main(int argc, char *argv[])
{
	
	pthread_t    testThread; 
	input_param  inParam  = {0};
	output_param outParam = {0};
	Allocator allocator = {0};
	threadqueue dataQueue = {0};
	noovo_buffer_data bd = { 0 };
	ffmpeg_context ctx = {0};
	int ret;

	memory_dbg_init(&stop);

	init_allocator(&allocator, 32/*INIT_ALLOCATOR_NUM*/);
	thread_queue_init(&dataQueue);
	ret = set_file_map_io(argv[1], &bd);

	inParam.read_data = file_map_read_data;
	inParam.opData    = (void*)&bd;
	inParam.allocator = &allocator;
	inParam.dataQueue = &dataQueue;
	INIT_ATOM_V(inParam);

	outParam.allocator   = &allocator;
	outParam.dataQueue   = &dataQueue;
	outParam.current_buf = NULL;

	ret = setup_ffmpeg_ctx(&ctx, (void *)(&outParam), ff_read_packet_impl);
	if (ret) {
		fprintf(stderr, "[%s:%s():%d]: setup_ffmpeg_ctx failed, ret = %d\n", __FILE__, __FUNCTION__, __LINE__, ret);
	}

	ret = pthread_create(&testThread, NULL, generic_input_thread_loop, (void *)(&inParam));
	if (ret) {
		fprintf(stderr, "[%s:%s():%d]: create thread failed, ret = %d\n", __FILE__, __FUNCTION__, __LINE__, ret);
	}


    ret = avformat_open_input(&(ctx.fmt_ctx), NULL, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open input\n");
        goto exit;
    }

#if 1

    ret = avformat_find_stream_info(ctx.fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream information\n");
        goto exit;
    }
	av_dump_format(ctx.fmt_ctx, 0, NULL, 0);

#endif

	pthread_join(testThread, NULL);


exit:


	/*do house keeping*/
	terminate_allocator(&allocator);
	av_file_unmap(bd.header, bd.capacity);


	memory_dbg_finalize();
		
	return 1;
}

int main_OK(int argc, char *argv[])
{

	fptr = fopen("origin2.ts", "w");
	if(!fptr)
		return 0;


	uint8_t *buffer       = NULL;
	size_t buffer_size    = -1;
	char *input_filename  = NULL;
	int   ret             = 0;
	struct buffer_data bd = { 0 };
	void *tret;

	Allocator      allocator;
	test_param     param       = {0}; 
	pthread_t      testThread;
	pthread_attr_t testAttr;

	threadqueue dataQueue;
	thread_queue_init(&dataQueue);

	OpBuf opBuf = {0};
	buffer_data op_buf = {0};

	AVFormatContext *fmt_ctx     = NULL;
	uint8_t *avio_ctx_buffer     = NULL;
	size_t  avio_ctx_buffer_size = 4096;
	AVIOContext *avio_ctx        = NULL;

	input_filename = argv[1];
    /* slurp file content into buffer */
    ret = av_file_map(input_filename, &buffer, &buffer_size, 0, NULL);
    if (ret < 0) {
		return -1;
	}
    /* fill opaque structure used by the AVIOContext read callback */
    bd.ptr  = buffer;
    bd.size = buffer_size;

    /* register codecs and formats and other lavf/lavc components*/
    av_register_all();


	init_allocator(&allocator, 32/*INIT_ALLOCATOR_NUM*/);
	param.allocator = &allocator;
	param.buf_data  = &bd;
	param.dataQueue = &dataQueue;
	int r = pthread_create(&testThread, NULL, myTest, (void *)(&param));

	opBuf.data      = &op_buf;
	opBuf.dataQueue = &dataQueue;

    if (!(fmt_ctx = avformat_alloc_context())) {
		return -1;
    }

    avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
		return -1;
    }
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size,
                                  0, &opBuf, &read_packet_2, NULL, NULL);
    if (!avio_ctx) {
		return -1;
    }
    fmt_ctx->pb = avio_ctx;


	ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
	if (ret < 0) {	
		fprintf(stderr, "@@@@ Could not open input ret %d\n", ret);
		pthread_join(testThread, NULL);
		//goto end;
	}

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream information\n");
        //goto end;
    }
	av_dump_format(fmt_ctx, 0, input_filename, 0);


	pthread_join(testThread, NULL);



#if 0
	threadmsg msg = {0};
	FILE *pfile = fopen("result.ts", "w");
	while(thread_queue_length(&dataQueue)) {
		thread_queue_get(&dataQueue, NULL, &msg);
		Allocation *allocation = (Allocation*)msg.data;
		unsigned char *data = (unsigned char*)(allocation->data);
		fwrite(data, AVAILABLE_DATA_SZ(allocation), 1, pfile);
		printf("[%s:%s():%d]: %p 0x%x 0x%x 0x%x 0x%x thread = %p %ld\n", __FILE__, __FUNCTION__, __LINE__, data, data[0], data[1], data[2], data[3], pthread_self(), thread_queue_length(&dataQueue));
	}
	fclose(pfile);
#endif

end:
	/*write file to check result*/
	terminate_allocator(&allocator);
	memory_dbg_finalize();
	fclose(fptr);

	return 1;
}

int main2(int argc, char *argv[])
{
	fptr = fopen("origin1.ts", "w");
	if(!fptr)
		return 0;

	//Allocator allocator;
	test_param param = {0};

    AVFormatContext *fmt_ctx = NULL;
    AVIOContext *avio_ctx = NULL;
    uint8_t *buffer = NULL, *avio_ctx_buffer = NULL;
    size_t buffer_size, avio_ctx_buffer_size = 4096;
    char *input_filename = NULL;
    int ret = 0;
    struct buffer_data bd = { 0 };

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

	param.buf_data = &bd;

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
    fmt_ctx->pb = avio_ctx;

    ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open input\n");
        goto end;
    }
/*
    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream information\n");
        goto end;
    }

    av_dump_format(fmt_ctx, 0, input_filename, 0);
*/
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

	fclose(fptr);

    return 0;
}

