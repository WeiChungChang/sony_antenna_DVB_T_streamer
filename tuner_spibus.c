/* Copyright 2016-2017 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by William Ho      2016-09-15
 * Modified by 
 *
 * Copyright (c) 2016-2017 Noovo Crop.  All rights reserved
 */
#include "tuner_dvbt.h"
sony_result_t FinalizeTSRead() {
	/*
	 * This function Finalize TS reading related things.
	 * In normal, this initialization should be done at the end of user's TV application.
	 * Do not need to do before each tuning to suspend TS thread while tuning.
	 */
	if (g_pTSReadData) {
		free (g_pTSReadData);
		g_pTSReadData = NULL;
	}
	return SONY_RESULT_OK;
}

sony_result_t InitializeTSRead() {
	/*
	 * This function initializes TS reading related things.
	 * In normal, this initialization should be done at the start of user's TV application.
	 * Do not need to do after each tuning.
	 */
	sony_result_t result = SONY_RESULT_OK;
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

sony_result_t PrepareForChannel(TUNER_DRIVER_CONTROLLER_T *pDriver) {
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
	//Jack Hsu mark this line, because we have to receive packet and send to Qualdrill real-time
	//readReadyTSPacket /= 2;
	readReadyTSPacket = 1;

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
