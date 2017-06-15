/* Copyright 2016-2017 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by Wiliam Ho      2017-03-31 For ISDB-T recorder.
 *
 * Copyright (c) 2016-2017 Noovo Crop.  All rights reserved
 */
#include "tuner_dvbt.h"
TUNER_DRIVER_CONTROLLER_T controller;
int g_TuneFreq;

/*------------------------------------------------------------------------------
 Globe Variables
 ------------------------------------------------------------------------------*/
uint8_t * g_pTSReadData;
FILE * g_outputFile;
int g_TuneFreq;
int g_BandWidth;
int g_EnableDigProg;
DTV_STANDARD g_dtv;

/*
 * Here we read argv from command call.
 * Description: Parse frequency and band-width from command.
 * @
 * @return: Always return 0 now, or show help message only.
 *          
 */
static int ReadArgv(int argc, char *argv[])
{
	char* const short_options = "f:b:ph";
	int val = 0;
	int c;
	struct option long_options[] =
	{
	{ "freq", 0, NULL, 'f' },
	{ "bandwidth", 0, NULL, 'b' },
	{ "help", no_argument, NULL, 'h' },
	{ 0, 0 }, };
	g_TuneFreq = TUNE_CFREQ_KHZ;
	g_BandWidth = TUNE_BANDWIDTH_6MHZ;
	g_EnableDigProg = 0;
	while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1)
	{
		switch (c)
		{
		case 'f':
			if ((sscanf(optarg, "%d", &val) != 1) || (val < 400000) || (val > 900000))
			{
				printf("Warning: wrong speed value (Hz)\n");
			}
			else
			{
				g_TuneFreq = val;
			}
			break;
		case 'b':
			if ((sscanf(optarg, "%d", &val) != 1) || (val < 1) || (val > 8))
			{
				printf("Warning: wrong band width value (GHz), only 1,5,6,7,8\n");
			}
			else
			{
				g_BandWidth = val;
			}
			break;
		case 'h':
		default:
			printf("%s will read TV tuner data from SPI bus and transfer to share memory. \nVersion: %s\n", argv[0],
					APP_VERSION);
			printf("Usage %s [-d DTV_standard] [-f frequency_kHZ] [-b bandwidth_MHz] [-p] [-h] \n", argv[0]);
			printf("\tEx: %s -f 633563 -b 6 \n", argv[0]);
			;
			printf("\t-f --freq : Central frequency of tune.(kHz)\n");
			printf("\t-b --bandwidth :  Bandwidth of tune.(MHz)\n");
			printf("\t-h --help : Show this help message\n");
			exit(0);
			break;
		}
	}
	printf("Start to read ISDB-T at freq= %d kHz, bandwidth= %d MHz\n", g_TuneFreq, g_BandWidth);
	return 0;
}

static sony_result_t PrepareForControl()
{
	sony_result_t result = SONY_RESULT_OK;
	// Driver instances.
	//TUNER_DRIVER_CONTROLLER_T controller
	controller.spiFreq = SPI_FREQ;
	controller.spiBusNum = SPI_BUS;
	controller.spiChipSel = SPI_DEVICE;
	controller.spiMode = SONY_SPI_MODE_0;
	/* Display driver credentials */
	printf("Driver Version : %s\n", SONY_TUNERDEMOD_DRIVER_VERSION);
	printf("Tuner readSPIts start\n");
	result = sony_init_spi_controller(&controller);
	return result;
}

// Setup ISDBT Tuner
static sony_result_t SetupISDBTune()
{
	sony_isdbt_tune_param_t pTuneParam;
	pTuneParam.centerFreqKHz = g_TuneFreq;
	pTuneParam.bandwidth = g_BandWidth;
	pTuneParam.oneSegmentOptimize = 0;
	printf("Tune to ISDB-T signal with the following parameters:\n");
	printf(" - Center Freq          : %u kHz\n", pTuneParam.centerFreqKHz);
	printf(" - Bandwidth            : %s\n", Common_Bandwidth[pTuneParam.bandwidth]);
	printf(" - One Segment Optimize : %d\n", pTuneParam.oneSegmentOptimize);
	return sony_integ_isdbt_Tune(&controller.tunerDemod, &pTuneParam);
}

static sony_result_t ReadSPITS(TUNER_DRIVER_CONTROLLER_T *pDriver)
{
	sony_result_t result = SONY_RESULT_OK;
	printf("------------------------------------------\n");
	printf("Reading SPI TS data\n");
	printf("------------------------------------------\n");
	/* Preparation should be done at TV application start timing. */
	result = InitializeTSRead();
	if (result != SONY_RESULT_OK)
	{
		return result;
	}
	/*Preparation dynamic memery */
	g_pTSReadData = (uint8_t *) malloc(2008 * 188);
	if (!g_pTSReadData)
	{
		printf("malloc failed.\n");
		result = SONY_RESULT_ERROR_IO;
		return result;
	}
	/* Preparation should be done after every tuner/demod tuning. */
	result = PrepareForChannel(pDriver);
	if (result != SONY_RESULT_OK)
	{
		printf("PrepareForChannel failed.\n");
		FinalizeTSRead();
		return result;
	}
	printf("ISDBT %d packets will be read and stored to \"%s\" file.\n", READ_TS_PACKET_MAX, TS_FILE_NAME);
	{
		uint32_t tsCount = 0;
		int progress = 0;
		sony_tunerdemod_ts_buffer_info_t bufferInfo;
		uint8_t overflowDetected = 0;
		uint8_t underflowDetected = 0;
		printf("While loop entry....\n");
		/* TS reading loop. */
		while (tsCount < READ_TS_PACKET_MAX)
		{
			result = sony_devio_spi_ReadTSBufferInfo(&pDriver->spi, &bufferInfo);
			if (result != SONY_RESULT_OK)
			{
				printf("sony_example_spi_readts: sony_devio_spi_ReadTSBufferInfo failed\n");
				return result;
			}
			if (!overflowDetected && (bufferInfo.overflow == 1))
			{
				printf("sony_example_spi_readts: SPI buffer is overflow\n");
				overflowDetected = 1;
			}

			if (!underflowDetected && (bufferInfo.underflow == 1))
			{
				printf("sony_example_spi_readts: SPI buffer is underflow\n");
				underflowDetected = 1;
			}

			if (bufferInfo.readReady == 1)
			{
				result = sony_devio_spi_ReadTS(&pDriver->spi, g_pTSReadData, bufferInfo.packetNum);
				if (result != SONY_RESULT_OK)
				{
					printf("sony_example_spi_readts: sony_devio_spi_ReadTS failed\n");
					break;
				}
				//William : Dig channel when enable on argument, skipped after function return 1.
				//printf("Sent %d packets ", bufferInfo.packetNum);
				if (fwrite(g_pTSReadData, sizeof(uint8_t), bufferInfo.packetNum * 188, g_outputFile)
						!= bufferInfo.packetNum * 188)
				{
					printf("sony_example_spi_readts: fwrite error\n");
					result = SONY_RESULT_ERROR_OTHER;
					break;
				}
				tsCount += bufferInfo.packetNum;
				{
					int newProgress = tsCount * 100 / READ_TS_PACKET_MAX;
					if ((progress / 10) != (newProgress / 10))
					{
						printf("%d %%\n", newProgress);
					}

					progress = newProgress;
				}
			}
			//printf("Error:  bufferInfo not ready to read.\n");
		}
		printf("Channel date %d packets saved.\n", tsCount);
	}

	/* Finish TS reading. */
	FinalizeTSRead(pDriver);
	return result;
}

int main(int argc, char *argv[])
{
	ReadArgv(argc, argv);
	g_outputFile = fopen("output.ts", "wb");
	if (!g_outputFile)
	{
		printf("fopen failed.\n");
		return -1;
	}
	sony_result_t result = PrepareForControl();
	if (result != SONY_RESULT_OK)
	{
		printf("Error : Cannot prepare for control (result = %s).\n", Common_Result[result]);
		return -1;
	}
	printf("sony_init_spi_controller succeeded.\n");
	// Initialize the device./
	result = sony_integ_Initialize(&controller.tunerDemod);
	if (result == SONY_RESULT_OK)
	{
		printf("Driver initialize done.\n");
	}
	else
	{
		printf("Error : Cannot run initialize (result = %s).\n", Common_Result[result]);
		return -1;
	}
	result = SetupISDBTune();
	if ((result != SONY_RESULT_OK) && (result != SONY_RESULT_OK_CONFIRM))
	{
		printf("Error : Cannot setup ISDB tune (result = %s).\n", Common_Result[result]);
		return -1;
	}
	else
	{
		printf("Setup ISDB-T Tune OK");
	}
	//Print RF Log for debug
	int32_t rfLevel;
	result = sony_tunerdemod_monitor_RFLevel(&controller.tunerDemod, &rfLevel);
	if (result == SONY_RESULT_OK)
	{
		printf("\nRF Level ==> %d dBm\n", rfLevel / 1000);
	}
	else
	{
		printf("Error: RF Level N/A : %s\n", Common_Result[result]);
	}
	//Read TS Data
	result = ReadSPITS(&controller);
	if (result == SONY_RESULT_OK)
	{
		printf("SPI ReadTS OK\n");
	}
	else
	{
		printf("Error : SPI ReadTS Failed (result = %s)\n", Common_Result[result]);
		return -1;
	}
	if (g_outputFile)
	{
		fclose(g_outputFile);
		g_outputFile = NULL;
	}
	printf("Processor all done\n");
	return 0;
}

