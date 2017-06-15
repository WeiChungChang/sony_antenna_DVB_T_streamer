/* Copyright 2016-2017 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by William Ho      2016-09-10
 * Modified by 
 *
 * Copyright (c) 2016-2017 Noovo Crop.  All rights reserved
 */
#include "tuner_spi2ts.h"

static sony_result_t InitializeTSRead() {
	/*
	 * This function initializes TS reading related things.
	 * In normal, this initialization should be done at the start of user's TV application.
	 * Do not need to do after each tuning.
	 */
	sony_result_t result = SONY_RESULT_OK;
	/* Open "output.ts" at current directory. */
	g_outputFile = fopen(TS_FILE_NAME, "wb");
	if (!g_outputFile) {
		printf("fopen failed.\n");
		return SONY_RESULT_ERROR_OTHER;
	}

	/*
	 * Allocate buffer to store TS data.
	 * 2008 * 188 is the max buffer size of CXD2880. (DVB-T2, single PLP case)
	 */
	g_pTSReadData = (uint8_t *) malloc(2008 * 188);
	if (!g_pTSReadData) {
		printf("malloc failed.\n");
		result = SONY_RESULT_ERROR_IO;
		goto ERROR_EXIT;
	}
	return result;

	ERROR_EXIT: FinalizeTSRead();

	return result;
}

static sony_result_t FinalizeTSRead() {
	/*
	 * This function Finalize TS reading related things.
	 * In normal, this initialization should be done at the end of user's TV application.
	 * Do not need to do before each tuning to suspend TS thread while tuning.
	 */

	if (g_pTSReadData) {
		free (g_pTSReadData);
		g_pTSReadData = NULL;
	}

	if (g_outputFile) {
		fclose (g_outputFile);
		g_outputFile = NULL;
	}

	return SONY_RESULT_OK;
}

static sony_result_t PrepareForChannel(TUNER_DRIVER_CONTROLLER_T *pDriver) {
	/*
	 * This function prepares TS reading for each channel.
	 * Depend on user's application, these settings should be done after each tuning
	 * and before resuming TS read.
	 */

	sony_result_t result = SONY_RESULT_OK;
	uint32_t readReadyTSPacket = 0;

	/* Decide TS size to read */
	switch (pDriver->tunerDemod.system) {
	case SONY_DTV_SYSTEM_DVBT:
	case SONY_DTV_SYSTEM_ISDBT:
		/* For DVB-T and ISDB-T, TS buffer max is 1114 */
		readReadyTSPacket = 1114;
		break;
	case SONY_DTV_SYSTEM_DVBT2: {
		sony_dvbt2_l1post_t l1Post;
		result = sony_tunerdemod_dvbt2_monitor_L1Post(&pDriver->tunerDemod, &l1Post);
		if (result != SONY_RESULT_OK) {
			/* Use smaller value. */
			readReadyTSPacket = 837;
			break;
		}

		if (l1Post.numPLPs > 1) {
			readReadyTSPacket = 837;
		} else {
			readReadyTSPacket = 2008;
		}
	}
		break;
	default:
		printf("Invalid system (%d)\n", pDriver->tunerDemod.system);
		return SONY_RESULT_ERROR_OTHER;
	}

	/*
	 * Deside TS read ready thresold.
	 * It can be changed by the user's system.
	 * Here, read ready threshold is 50% of max buffer size.
	 */
	readReadyTSPacket /= 2;

	/* Set TS buffer read ready threshold. */
	result = sony_tunerdemod_SetConfig(&pDriver->tunerDemod, SONY_TUNERDEMOD_CONFIG_TS_BUFFER_RRDY_THRESHOLD,
			readReadyTSPacket);
	if (result != SONY_RESULT_OK) {
		printf("sony_tunerdemod_SetConfig failed. (%s)\n", Common_Result[result]);
		return result;
	}

	/*
	 * Set PID filter.
	 * It can be changed by PID that target TS includes.
	 * Here, set PID filter to exclude NULL packets. (PID = 0x1FFF)
	 */
	{
		sony_tunerdemod_pid_filter_config_t config;
		memset(&config, 0, sizeof(config));

		config.isNegative = 1; /* To exclude specified PIDs. */
		config.pidConfig[0].isEnable = 1;
		config.pidConfig[0].pid = 0x1FFF;  //We need all PID

		result = sony_tunerdemod_SetPIDFilter(&pDriver->tunerDemod, &config);
		if (result != SONY_RESULT_OK) {
			printf("sony_tunerdemod_SetPIDFilter failed. (%s)\n", Common_Result[result]);
			return result;
		}
	}

	/* Clear TS buffer */
	result = sony_devio_spi_ClearTSBuffer(&pDriver->spi);
	if (result != SONY_RESULT_OK) {
		printf("sony_devio_spi_ClearTSBuffer failed. (%s)\n", Common_Result[result]);
		return result;
	}

	return result;
}

sony_result_t sony_example_spi_ReadTS(TUNER_DRIVER_CONTROLLER_T *pDriver) {
	sony_result_t result = SONY_RESULT_OK;

	printf("------------------------------------------\n");
	printf(" TS reading (SPI)\n");
	printf("------------------------------------------\n");
	printf("%d packets will be read and stored to \"%s\" file.\n", READ_TS_PACKET_MAX, TS_FILE_NAME);

	/* Preparation should be done at TV application start timing. */
	result = InitializeTSRead();
	if (result != SONY_RESULT_OK) {
		return result;
	}

	/* Preparation should be done after every tuner/demod tuning. */
	result = PrepareForChannel(pDriver);
	if (result != SONY_RESULT_OK) {
		FinalizeTSRead();
		return result;
	}

	{
		uint32_t tsCount = 0;
		int progress = 0;
		sony_tunerdemod_ts_buffer_info_t bufferInfo;
		uint8_t overflowDetected = 0;
		uint8_t underflowDetected = 0;
		printf("While loop entry....\n");
		/* TS reading loop. */
		while (tsCount < READ_TS_PACKET_MAX) {
			result = sony_devio_spi_ReadTSBufferInfo(&pDriver->spi, &bufferInfo);
			if (result != SONY_RESULT_OK) {
				printf("sony_example_spi_readts: sony_devio_spi_ReadTSBufferInfo failed\n");
				return result;
			}
			if (!overflowDetected && (bufferInfo.overflow == 1)) {
				printf("sony_example_spi_readts: SPI buffer is overflow\n");
				overflowDetected = 1;
			}
			if (!underflowDetected && (bufferInfo.underflow == 1)) {
				printf("sony_example_spi_readts: SPI buffer is underflow\n");
				underflowDetected = 1;
			}
			if (bufferInfo.readReady == 1) {
				result = sony_devio_spi_ReadTS(&pDriver->spi, g_pTSReadData, bufferInfo.packetNum);
				if (result != SONY_RESULT_OK) {
					printf("sony_example_spi_readts: sony_devio_spi_ReadTS failed\n");
					break;
				}
				if (fwrite(g_pTSReadData, sizeof(uint8_t), bufferInfo.packetNum * 188, g_outputFile)
						!= bufferInfo.packetNum * 188) {
					printf("sony_example_spi_readts: fwrite error\n");
					result = SONY_RESULT_ERROR_OTHER;
					break;
				}

				tsCount += bufferInfo.packetNum;
			}
			{
				int newProgress = tsCount * 100 / READ_TS_PACKET_MAX;
				if ((progress / 10) != (newProgress / 10)) {
					printf("%d %%\n", newProgress);
				}
				progress = newProgress;
			}
		}
	}

	/* Finish TS reading. */
	FinalizeTSRead(pDriver);

	return result;
}

/*
 First version write testing code to scan channels and receiver TS to file
 */
int main(int argc, char *argv[]) {
	//Scan TS channels
	sony_result_t result = SONY_RESULT_OK;
	// Driver instances.
	TUNER_DRIVER_CONTROLLER_T controller;
	//sony_integ_dvbt2_Tune tuneP;
	controller.spiFreq = SPI_FREQ;
	controller.spiBusNum = SPI_BUS;
	controller.spiChipSel = SPI_DEVICE;
	controller.spiMode = SONY_SPI_MODE_0;
	/* Display driver credentials */
	printf("Driver Version : %s\n", SONY_TUNERDEMOD_DRIVER_VERSION);
	printf("Tuner readSPIts start\n");

	result = sony_init_spi_controller(&controller);
	if (result != SONY_RESULT_OK) {
		printf("Error : sony_init_spi_controller failed. (result = %s)\n", Common_Result[result]);
		return -1;
	}
	printf("sony_init_spi_controller succeeded.\n");
	// Initialize the device./
	result = sony_integ_Initialize(&controller.tunerDemod);
	if (result == SONY_RESULT_OK) {
		printf("Driver initialized, current state = SONY_TUNERDEMOD_STATE_SLEEP\n");
	} else {
		printf("Error : sony_integ_Initialize failed. (result = %s)\n", Common_Result[result]);
		return -1;
	}
	// Setup DVBT Tune 
	sony_dvbt_tune_param_t pTuneParam;
	pTuneParam.centerFreqKHz = TUNE_CFREQ_KHZ;
	pTuneParam.bandwidth = SONY_DTV_BW_6_MHZ;
	pTuneParam.profile = SONY_DVBT_PROFILE_HP;
	result = sony_integ_dvbt_Tune(&controller.tunerDemod, &pTuneParam);
	if (result == SONY_RESULT_OK) {
		printf("The DVBT setup OK\n");
	} else {
		printf("Error : The DVBT setup Failed (result = %s)\n", Common_Result[result]);
		return -1;
	}
	//Print RF Log for debug
	int32_t rfLevel;
	result = sony_tunerdemod_monitor_RFLevel(&controller.tunerDemod, &rfLevel);
	if (result == SONY_RESULT_OK) {
		printf("RF Level ==> %d x 10^-3 dBm\n", rfLevel);
	} else {
		printf("Error: RF Level N/A : %s\n", Common_Result[result]);
	}
	result = sony_example_spi_ReadTS(&controller);
	if (result == SONY_RESULT_OK) {
		printf("SPI ReadTS OK\n");
	} else {
		printf("Error : SPI ReadTS Failed (result = %s)\n", Common_Result[result]);
		return -1;
	}
	printf("Processor all done\n");
	return 0;
}

