#include <stdbool.h>

#include "tuner_dvbt.h"
#include "libdvbpsi/dvbpsi.h"
#include "libdvbpsi/psi.h"
#include "libdvbpsi/demux.h"
#include "libdvbpsi/descriptor.h"
#include "libdvbpsi/sdt.h"
#include "libavformat/avformat.h"


pthread_mutex_t calculating = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t done = PTHREAD_COND_INITIALIZER;

typedef struct _Channel_data
{
	int32_t cFreqKHz;
	sony_result_t result;
} Channel_Data;

/*****************************************************************************
 * DVBPSI messaging callback
 *****************************************************************************/
static void message(dvbpsi_t *handle, const dvbpsi_msg_level_t level, const char* msg)
{
    switch(level)
    {
        case DVBPSI_MSG_ERROR: fprintf(stderr, "Error: "); break;
        case DVBPSI_MSG_WARN:  fprintf(stderr, "Warning: "); break;
        case DVBPSI_MSG_DEBUG: fprintf(stderr, "Debug: "); break;
        default: /* do nothing */
            return;
    }
    fprintf(stderr, "%s\n", msg);
}

/*****************************************************************************
 * DumpDescriptors
 *****************************************************************************/
static void DumpDescriptors(const char* str, dvbpsi_descriptor_t* p_descriptor)
{
  while(p_descriptor)
  {
    int i;
    printf("%s 0x%02x : \"", str, p_descriptor->i_tag);
    for(i = 0; i < p_descriptor->i_length; i++)
      printf("%c", p_descriptor->p_data[i]);
    printf("\"\n");
    p_descriptor = p_descriptor->p_next;
  }
};

/*****************************************************************************
 * DumpSDT
 *****************************************************************************/
static void DumpSDT(void* p_zero, dvbpsi_sdt_t* p_sdt)
{
  dvbpsi_sdt_service_t* p_service = p_sdt->p_first_service;
  printf(  "\n");
  printf(  "New active SDT\n");
  printf(  "  ts_id : %d\n",
         p_sdt->i_extension);
  printf(  "  version_number : %d\n",
         p_sdt->i_version);
  printf(  "  network_id        : %d\n",
         p_sdt->i_network_id);
  printf(  "    | service_id \n");
  while(p_service)
  {
    printf("    | 0x%02x \n",
           p_service->i_service_id);
    DumpDescriptors("    |  ]", p_service->p_first_descriptor);
    p_service = p_service->p_next;
  }
  dvbpsi_sdt_delete(p_sdt);
}

/*****************************************************************************
 * NewSubtable
 *****************************************************************************/
static void NewSubtable(dvbpsi_t *p_dvbpsi, uint8_t i_table_id, uint16_t i_extension,
                        void * p_zero)
{
  if(i_table_id == 0x42)
  {
     if (!dvbpsi_sdt_attach(p_dvbpsi, i_table_id, i_extension, DumpSDT, NULL))
         fprintf(stderr, "Failed to attach SDT subdecoder\n");
  }
}

inline uint8_t* getPacket(const uint8_t *buffer, int num) {
	return (buffer + 188 * num);
}

void testGetAVInputFormat(){
	AVInputFormat *in;
	in = av_iformat_next(NULL);
	while(in) {
		printf("%s\n", in->long_name);
	}
}

static void ReadPrograms(void *ptr)
{
	dvbpsi_t *p_dvbpsi;
	sony_result_t result = SONY_RESULT_OK;
	int oldtype;
	int digret = 0;
	Channel_Data *data;

	p_dvbpsi = dvbpsi_new(&message, DVBPSI_MSG_DEBUG);
	if (p_dvbpsi == NULL)
		return;

	if (!dvbpsi_AttachDemux(p_dvbpsi, NewSubtable, NULL))
		return;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
	/* type cast to a pointer to Channel_Data */
	data = (Channel_Data *) ptr;
	printf("Reading SPI TS data...\n");
	/* Preparation should be done at TV application start timing. */
	result = InitializeTSRead();
	if (result != SONY_RESULT_OK)
	{
		printf("Can't init TSReader (result = %s)\n", Common_Result[result]);
		data->result = result;
		goto ERROR_OUT;
	}
	/*Preparation Share memery */
	if (0 != Initialize_Shared_Memory())
	{
		printf("Can't init share memory (result = %s)\n", Common_Result[result]);
		data->result = SONY_RESULT_ERROR_OTHER;
		goto ERROR_OUT;
	}
	/* Preparation should be done after every tuner/demod tuning. */
	result = PrepareForChannel(&controller);
	if (result != SONY_RESULT_OK)
	{
		printf("Can't prepare For Channel(result = %s)\n", Common_Result[result]);
		goto ERROR_OUT;
	}

	{
		uint32_t tsCount = 0;
		sony_tunerdemod_ts_buffer_info_t bufferInfo;
		uint8_t overflowDetected = 0;
		uint8_t underflowDetected = 0;
		/* TS reading loop. */
		while (tsCount < 100000)
		{
			result = sony_devio_spi_ReadTSBufferInfo(&controller.spi, &bufferInfo);
			if (result != SONY_RESULT_OK)
			{
				printf("Read TSBuffer info failed (result = %s)\n", Common_Result[result]);
				data->result = result;
				return;
			}
			if (!overflowDetected && (bufferInfo.overflow == 1))
			{
				//printf("sony_example_spi_readts: SPI buffer is overflow\n");
				overflowDetected = 1;
			}
			if (!underflowDetected && (bufferInfo.underflow == 1))
			{
				//printf("sony_example_spi_readts: SPI buffer is underflow\n");
				underflowDetected = 1;
			}
			if (bufferInfo.readReady == 1)
			{
				result = sony_devio_spi_ReadTS(&controller.spi, g_pTSReadData, bufferInfo.packetNum);
				if (result != SONY_RESULT_OK)
				{
					printf("sony_devio_spi_ReadTS failed (result = %s)\n", Common_Result[result]);
					break;
				}
				for (int i = 0; i < bufferInfo.packetNum; i++) {
					uint8_t *data = getPacket(g_pTSReadData, i);
					uint16_t i_pid = ((uint16_t)(data[1] & 0x1f) << 8) + data[2];
					if(i_pid == 0x11) {
						//printf("Find sdt 0x%x 0x%x 0x%x %d %p %p\n", data[0], data[1], data[2], tsCount, p_dvbpsi, p_dvbpsi->p_decoder);
						dvbpsi_packet_push (p_dvbpsi, data); /*test the integration of libdvbpsi*/
					}
				}
				tsCount += bufferInfo.packetNum;
/*
				//William : Dig channel at beginning, skipped after channel info return data.
				if (digret == 0)
				{
					digret = DigChannelInfo(g_pTSReadData, bufferInfo.packetNum, data->cFreqKHz, g_BandWidth);
					tsCount += bufferInfo.packetNum;
					printf("\rIn digging channels with %d packets", tsCount);
				}
				else
				{
					//Here we record data to share memory
					printf("\nGot the channel info [OK], we found [%d] programs.\n\n", digret);
					data->result = result;
					FinalizeTSRead (&controller);
					pthread_cond_signal(&done);
					return;

				}
*/
			}
		}
		printf("No channel data found at %d packets, aborted.\n", tsCount);
	}

	ERROR_OUT: FinalizeTSRead (&controller);
	pthread_cond_signal(&done);
	return;
}

// Setup DVBT Tune
static sony_result_t SetupDVBTTune(uint32_t c_freq, DTV_STANDARD tvb_sys)
{
	sony_result_t result = SONY_RESULT_OK;
	if (tvb_sys == DVB_T)
	{
		sony_dvbt_tune_param_t pTuneParam;
		pTuneParam.centerFreqKHz = c_freq;
		pTuneParam.bandwidth = (sony_dtv_bandwidth_t) g_BandWidth;
		pTuneParam.profile = SONY_DVBT_PROFILE_HP;
		return sony_integ_dvbt_Tune(&controller.tunerDemod, &pTuneParam);
	}
	else
	{
		sony_dvbt2_tune_param_t pTuneParam;
		pTuneParam.centerFreqKHz = c_freq;
		pTuneParam.bandwidth = (sony_dtv_bandwidth_t) g_BandWidth;
		pTuneParam.dataPLPID = 0;
		pTuneParam.profile = SONY_DVBT2_PROFILE_BASE;
		pTuneParam.tuneInfo = SONY_TUNERDEMOD_DVBT2_TUNE_INFO_OK;
		result = sony_integ_dvbt2_Tune(&controller.tunerDemod, &pTuneParam);
		if ((result != SONY_RESULT_OK_CONFIRM))
		{
			printf("Error : sony_integ_dvbt2_Tune failed. (result = %s)\n", Common_Result[result]);
			return result;
		}
		//Confirm PLP ID
		if ((result == SONY_RESULT_OK_CONFIRM)
				&& (pTuneParam.tuneInfo == SONY_TUNERDEMOD_DVBT2_TUNE_INFO_INVALID_PLP_ID))
		{
			sony_dvbt2_plp_t plpInfo;
			printf("PLP ID error in acquisition:\n");
			result = sony_tunerdemod_dvbt2_monitor_ActivePLP(&controller.tunerDemod, SONY_DVBT2_PLP_DATA, &plpInfo);
			if (result != SONY_RESULT_OK)
			{
				printf(" Error : sony_tunerdemod_dvbt2_monitor_ActivePLP failed. (result = %s)\n",
						Common_Result[result]);
				return result;
			}
			printf(" - PLP Requested : %u\n", pTuneParam.dataPLPID);
			printf(" - PLP Acquired  : %u\n\n", plpInfo.id);
		}

	}
	return result;

}

int CallReaderWithTimeout(struct timespec *max_wait, int32_t cFreqKHz)
{
	struct timespec abs_time;
	pthread_t tid;
	int err;
	Channel_Data cdata;
	cdata.cFreqKHz = cFreqKHz;
	pthread_mutex_lock(&calculating);
	/* pthread cond_timedwait expects an absolute time to wait until */
	clock_gettime(CLOCK_REALTIME, &abs_time);
	abs_time.tv_sec += max_wait->tv_sec;
	abs_time.tv_nsec += max_wait->tv_nsec;
	pthread_create(&tid, NULL, (void *) ReadPrograms, (void *) &cdata);
	/* pthread_cond_timedwait can return spuriously: this should
	 * be in a loop for production code
	 */
	err = pthread_cond_timedwait(&done, &calculating, &abs_time);
	if (err == ETIMEDOUT)
	{
		printf("Read programs error: TIMEOUT\n");
		pthread_mutex_unlock(&calculating);
	}

	if (!err)
		pthread_mutex_unlock(&calculating);

	return err;
}

sony_result_t FindChannelsProg()
{
	sony_result_t result = SONY_RESULT_OK;

	int i;
	for (i = 0; i < g_FoundChNum; i++)
	{
		printf("Channel #%d, FREQ: %d kHz, Level: %d dB\n", i + 1, Channels[i].CentralFreqKHz, Channels[i].rfLevel);
		sony_init_spi_controller (&controller);
		sony_integ_Initialize(&controller.tunerDemod);
		result = SetupDVBTTune(Channels[i].CentralFreqKHz, g_dtv);
		if (result != SONY_RESULT_OK)
		{
			printf("Error : Find Channel's programs failed. (result = %s)\n", Common_Result[result]);
			return result;
		}
		else
		{
			printf("\tSetup new tune at %d OK\n", Channels[i].CentralFreqKHz);
		}
		{
			struct timespec reading_wait;
			memset(&reading_wait, 0, sizeof(reading_wait));
			reading_wait.tv_sec = PROGRAM_TIMEOUT;
			CallReaderWithTimeout(&reading_wait, Channels[i].CentralFreqKHz);
		}
	}
	return result;
}

void exportJSON(char * filename)
{
	//Here we dump data in Jason format.
	printf("====> Jasoon tester Start, %d channel(s),export to %s \n", g_FoundChNum, filename);
	int i = 0;
	FILE *fp;

	fp = fopen(filename, "w");
	if (fp == NULL)
	{
		printf(" Can't open file [%s] to write json result.\n", filename);
		return;
	}

	fprintf(fp, "[ \n");
	for (i = 0; i < g_FoundChNum; i++)
	{
		fprintf(fp, "{");
		fprintf(fp, "\"channel#\": %d, \"freq\": %d, \"lock\": %s ,", i, Channels[i].CentralFreqKHz,
				Channels[i].tsLock == 1 ? "true" : "false");
		fprintf(fp, " \"snr\": %d.%d, \"sqi\": %d, \"ssi\": %d, ", Channels[i].snr / 1000,
				(Channels[i].snr % 1000) / 100, Channels[i].sqi, Channels[i].ssi);
		fprintf(fp, " \"rf\": %d , \"pre_viterbi_ber\": %d, \"pre_rs_ber\": %d, \"pen\": %d , \"per\": %d ,", Channels[i].rfLevel,
				Channels[i].preViterbiBER, Channels[i].preRSBER, Channels[i].packetErrorNumber, Channels[i].PER);
		fprintf(fp, " \"post_bch_fer\": %d , \"pre_bch_ber\": %d , \"pre_ldpc_ber\": %d ", Channels[i].postBCHFER,
				Channels[i].PreBCHBER, Channels[i].preLDPCBER);
		if (i < (g_FoundChNum - 1))
		{
			fprintf(fp, " },\n");
		}
		else
		{
			fprintf(fp, " }\n");
		}
	}
	fprintf(fp, " ]\n");
	fclose(fp);
}
