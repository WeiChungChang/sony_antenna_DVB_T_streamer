#include <stdio.h>
#include <assert.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>

#include "tuner_dvbt.h"
#include "tuner_common.h"

#include <stdatomic.h>
#include <sys/types.h>

#include "allocation.h"
#include "threadqueue.h"
#include "mmdbg.h"

#include "generic_io.h"
#include "ffimpl.h"

#include "file_io.h"

#define DEBUG 1
//#define DEBUG 0

#define DUMP_PKT 0
//#define DUMP_PKT 0

#define DEFAULT_RING_BUF_SZ     1024*1024*10
#define AVIO_CTX_BUF_SZ         32*1024
#define MAX_PKT_NUM_PER_RECEIVE 256
#define TS_PKT_SZ               188
#define GET_TS_PKT_SZ(n)        (TS_PKT_SZ * (n))
#define DUMP_INTERVAL           50000

/*test settings*/
#define  TEST_FREQ        592985       
#define  TEST_BANDWIDTH   6
#define  TEST_DVB_SYSTEM  0

#define  ERROR_NONE       0
#define  ERROR_GENERAL   -1 
#define  ERROR_UNDERFLOW -2 
#define  ERROR_OVERFLOW  -3 
#define  ERROR_TIMEOUT   -4

#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)
#define dump_pkt(buf, sz, file) \
					do { if (DUMP_PKT) fwrite((buf), (sz), 1, (file)); fflush((file));} while (0)

#define CHECK_SYNC_BYTE(buf) (((buf)[0]) == (0x47))


void CheckAntennaReceive(TUNER_DRIVER_CONTROLLER_T *pController, int pktNumber)
{
	sony_result_t result;
	int tsCount = 0;
	sony_tunerdemod_ts_buffer_info_t bufferInfo;
	uint8_t *output = (uint8_t*) malloc_dbg(MAX_PKT_NUM_PER_RECEIVE * TS_PKT_SZ);
	if (!output) {
		return;
	}

	uint32_t nextTsCount = DUMP_INTERVAL;
	while(tsCount < pktNumber) {
		result = sony_devio_spi_ReadTSBufferInfo(&(pController->spi), &bufferInfo);
		if (result != SONY_RESULT_OK) {
			printf("sony_example_spi_readts: sony_devio_spi_ReadTSBufferInfo failed\n");
			goto exit;
		}
	
		if (bufferInfo.overflow == 1) {
			printf("sony_example_spi_readts: SPI buffer is overflow\n");
		}
		if (bufferInfo.underflow == 1) {
			printf("sony_example_spi_readts: SPI buffer is underflow\n");
		}
		if (bufferInfo.readReady == 1) {
			result = sony_devio_spi_ReadTS(&(pController->spi), output, bufferInfo.packetNum);
			if (result != SONY_RESULT_OK) {
				printf("sony_example_spi_readts: sony_devio_spi_ReadTS failed, reason = %d\n", result);
				break;
			}
			tsCount += bufferInfo.packetNum;
			
			if (tsCount >= nextTsCount) {
				debug_print("Have received #pkt = %d\n", tsCount);
				nextTsCount += DUMP_INTERVAL;
			}
		}
	}
exit:
	free_dbg(output);
}

#define MAX_PKT_SZ 1024*256
static int total_pkt = 0;

static int ReceiveSonyTSPacket(TUNER_DRIVER_CONTROLLER_T *pController, int8_t *dist, int max)
{
	assert (pController != NULL);
	assert (dist != NULL);

	sony_tunerdemod_ts_buffer_info_t bufferInfo;
	sony_result_t                    result;
	int max_pkt = (max / TS_PKT_SZ);

	while(1) {
		result = sony_devio_spi_ReadTSBufferInfo(&(pController->spi), &bufferInfo);
		if (result != SONY_RESULT_OK) {
			printf("sony_example_spi_readts: sony_devio_spi_ReadTSBufferInfo failed\n");
			return ERROR_GENERAL;
		}
			
		if (bufferInfo.overflow == 1) {
			printf("sony_example_spi_readts: bufferInfo.overflow\n");
			return ERROR_OVERFLOW;
		}
		if (bufferInfo.underflow == 1) {
			printf("sony_example_spi_readts: bufferInfo.underflow\n");
			return ERROR_UNDERFLOW;
		}
		if (bufferInfo.readReady == 1) {
			max_pkt = ((max_pkt > bufferInfo.packetNum) ? (bufferInfo.packetNum) : (max_pkt));
			result = sony_devio_spi_ReadTS(&(pController->spi), dist, max_pkt);
			if (result != SONY_RESULT_OK) {
				printf("sony_example_spi_readts: sony_devio_spi_ReadTS failed, reason = %d\n", result);
				return ERROR_GENERAL;
			} else {
				int offset = 0;
				while(offset < GET_TS_PKT_SZ(max_pkt)) {
					if (!CHECK_SYNC_BYTE((dist + offset))) {
						printf("[%s:%s():%d] unsync!!!\n", __FILE__, __FUNCTION__, __LINE__);
						return ERROR_GENERAL;
					} else {
						offset += TS_PKT_SZ;
					}
				}
				total_pkt += max_pkt;
#ifdef MAX_PKT_SZ
				if (total_pkt >= MAX_PKT_SZ) {
					printf("[%s:%s():%d] have read the max pkt!\n", __FILE__, __FUNCTION__, __LINE__);
					return 0;
				}
#endif
				break;
			}
		}
	}
	return GET_TS_PKT_SZ(max_pkt);
}

static int ReceiveSonyTSPacketWrapper(void *op, int8_t *dist, int max)
{
	TUNER_DRIVER_CONTROLLER_T *pController = (TUNER_DRIVER_CONTROLLER_T*)op;
	int r = ReceiveSonyTSPacket(pController, dist, max);
	return r;
}


int SetupDVBTunner(TUNER_DRIVER_CONTROLLER_T *controller, uint32_t freq, TUNER_DVB_BANDWIDTH_T bandwidth, TUNER_DVB_SYSTEM_T system)
{
	sony_result_t result = initialize_DVB_SPI_controller(controller);
	if (result != SONY_RESULT_OK) {
		printf("Error : Cannot prepare for control (result = %s).\n", Common_Result[result]);
		return -1;
	}
	printf("sony_init_spi_controller succeeded.\n");

	// Initialize the device./
	result = sony_integ_Initialize(&controller->tunerDemod);
	if (result == SONY_RESULT_OK) {
		printf("Driver initialize done.\n");
	} else {
		printf("Error : Cannot run initialize (result = %s).\n", Common_Result[result]);
		return -1;
	}

	result = SetupDVBTTuneCommon(freq, bandwidth, system, &(controller->tunerDemod));
	if ((result != SONY_RESULT_OK) && (result != SONY_RESULT_OK_CONFIRM)) {
		printf("Error : Cannot setup tune (result = %s).\n", Common_Result[result]);
		return -1;
	} else {
		printf("SetupDVBTTune OK\n");
	}

	//Print RF Log for debug
	int32_t rfLevel;
	result = sony_tunerdemod_monitor_RFLevel(&(controller->tunerDemod), &rfLevel);
	if (result == SONY_RESULT_OK) {
		printf("RF Level ==> %d x 10^-3 dBm\n", rfLevel);
	} else {
		printf("Error: RF Level N/A : %s\n", Common_Result[result]);
	}

	result = PrepareForSPIChannel(controller);
	if (result != SONY_RESULT_OK) {
		printf("PrepareForSPIChannel failed\n");
		return -1;
	}

	total_pkt = 0;
	return 0;
}

int testDVBTOverflow(uint32_t freq, TUNER_DVB_BANDWIDTH_T bandwidth, TUNER_DVB_SYSTEM_T system)
{
#define MAX_PKT_PER_READ 256
	TUNER_DRIVER_CONTROLLER_T controller = {0};
	uint8_t *buffer = malloc_dbg(GET_TS_PKT_SZ(MAX_PKT_PER_READ));

	int ret = ERROR_NONE;
	int overflow = 0;
	int receivedTSPkt = 0;

	if(SetupDVBTunner(&controller, freq, bandwidth, system) < 0) {
		ret = ERROR_GENERAL;
		goto error;
	}
	
	uint32_t nextTsCount = DUMP_INTERVAL;
	while(1) {
		ret = ReceiveSonyTSPacket(&controller, buffer, -1);
		if (ret == ERROR_OVERFLOW) {
			overflow++;
			printf("[%s:%s():%d] overflow; #overflow = %d\n", __FILE__, __FUNCTION__, __LINE__, overflow);
		} else if (ret < 0) {
			printf("[%s:%s():%d] err = %d, #overflow = %d\n", __FILE__, __FUNCTION__, __LINE__, ret, overflow);
			goto error;
		} else {
			receivedTSPkt += ret;
			if (receivedTSPkt >= nextTsCount) {
				printf("[%s:%s():%d] received #%d pkts, #overflow = %d\n", __FILE__, __FUNCTION__, __LINE__, receivedTSPkt, overflow);
				nextTsCount += DUMP_INTERVAL;
				break;
			}
		}	
	}
error:
	if(buffer)
		free_dbg(buffer);
	return ret;
}

int overflow_test()
{
	int ret = testDVBTOverflow(TEST_FREQ, TEST_BANDWIDTH, TEST_DVB_SYSTEM);
	return ret;
}

int main2(int argc, char *argv[])
{
	memory_dbg_init();
	overflow_test();
	memory_dbg_finalize();
}

int main_free2Air(int argc, char *argv[])
{
	pthread_t    testThread              = {0}; 
	input_param  inParam                 = {0};
	output_param outParam                = {0};
	Allocator allocator                  = {0};
	threadqueue dataQueue                = {0};
	TUNER_DRIVER_CONTROLLER_T controller = {0};

	ffmpeg_context ctx = {0};
	int ret;

	memory_dbg_init();

	/*stap 1: init allocator*/
	init_allocator(&allocator, 32/*INIT_ALLOCATOR_NUM*/);

	/*step 2: init buffer queue*/
	thread_queue_init(&dataQueue);

	/*step 3: init input param*/
	inParam.read_data = ReceiveSonyTSPacketWrapper;
	inParam.opData    = (void*)(&controller); /*not ready yet...*/
	inParam.allocator = &allocator;
	inParam.dataQueue = &dataQueue;
	//inParam.stop      = 0;
	//pthread_mutex_init(&(inParam.mutex), NULL);
	INIT_ATOM_V(&(inParam.stop), 0);

	/*step 4: init output param*/
	outParam.allocator   = &allocator;
	outParam.dataQueue   = &dataQueue;
	outParam.current_buf = NULL;

	/*step 5: setup ffmpeg param; expected sideeffect is a prepared ffmpeg_context*/
	ret = setup_ffmpeg_ctx(&ctx, (void *)(&outParam), ff_read_packet_impl);
	if (ret < 0) {
		fprintf(stderr, "[%s:%s():%d]: setup_ffmpeg_ctx failed, ret = %d\n", __FILE__, __FUNCTION__, __LINE__, ret);
	}

	/*step 6: prepare tuner input*/
	if(SetupDVBTunner(&controller, TEST_FREQ, TEST_BANDWIDTH, TEST_DVB_SYSTEM) < 0) {
		printf("[%s:%s():%d] SetupDVBTunner failed!\n", __FILE__, __FUNCTION__, __LINE__);
		ret = ERROR_GENERAL;
		goto exit;
	}

	/*step 7: launch reading thread*/
	ret = pthread_create(&testThread, NULL, generic_input_thread_loop, (void *)(&inParam));
	if (ret) {
		fprintf(stderr, "[%s:%s():%d]: create thread failed, ret = %d\n", __FILE__, __FUNCTION__, __LINE__, ret);
	}

	/*step 8: test the result*/
    ret = avformat_open_input(&(ctx.fmt_ctx), NULL, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open input\n");
        goto exit;
    }
	printf("[%s:%s():%d] avformat_open_input done\n", __FILE__, __FUNCTION__, __LINE__);
    
	ret = avformat_find_stream_info(ctx.fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream information\n");
        goto exit;
	}
	printf("[%s:%s():%d] avformat_find_stream_info done\n", __FILE__, __FUNCTION__, __LINE__);
    
	av_dump_format(ctx.fmt_ctx, 0, NULL, 0);

	printf("[%s:%s():%d]: stop reading thread!!!!!!\n", __FILE__, __FUNCTION__, __LINE__);
	SET(&(inParam.stop), 1);

	/*wait for complete*/
	pthread_join(testThread, NULL);
exit:

	/*do house keeping*/
	terminate_allocator(&allocator);
	memory_dbg_finalize();
		
	return ret;
}

int main(int argc, char *argv[])
{
	pthread_t    testThread              = {0}; 
	input_param  inParam                 = {0};
	Allocator    allocator               = {0};
	threadqueue  dataQueue               = {0};
	TUNER_DRIVER_CONTROLLER_T controller = {0};
	int ret;

	if (argc < 2) {
		printf("[%s:%s():%d] #param is less than requirement!\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	memory_dbg_init();

	/*stap 1: init allocator*/
	init_allocator(&allocator, 32/*INIT_ALLOCATOR_NUM*/);

	/*step 2: init buffer queue*/
	thread_queue_init(&dataQueue);

	/*step 3: init input param*/
	inParam.read_data = ReceiveSonyTSPacketWrapper;
	inParam.opData    = (void*)(&controller); /*not ready yet...*/
	inParam.allocator = &allocator;
	inParam.dataQueue = &dataQueue;
	INIT_ATOM_V(&(inParam.stop), 0);

	/*step 6: prepare tuner input*/
	if(SetupDVBTunner(&controller, TEST_FREQ, TEST_BANDWIDTH, TEST_DVB_SYSTEM) < 0) {
		printf("[%s:%s():%d] SetupDVBTunner failed!\n", __FILE__, __FUNCTION__, __LINE__);
		ret = ERROR_GENERAL;
		goto exit;
	}

	/*step 7: launch reading thread*/
	ret = pthread_create(&testThread, NULL, generic_input_thread_loop, (void *)(&inParam));
	if (ret) {
		fprintf(stderr, "[%s:%s():%d]: create thread failed, ret = %d\n", __FILE__, __FUNCTION__, __LINE__, ret);
	}

	/*step 8: test the result*/
	int r = file_store_loop (argv[1], &allocator, &dataQueue);

	printf("[%s:%s():%d]: stop reading thread!!!!!!\n", __FILE__, __FUNCTION__, __LINE__);
	SET(&(inParam.stop), 1);

	/*wait for complete*/
	pthread_join(testThread, NULL);
exit:

	/*do house keeping*/
	terminate_allocator(&allocator);
	memory_dbg_finalize();
		
	return ret;
}



