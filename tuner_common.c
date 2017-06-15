/* Copyright 2016-2017 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by JackHsu      2016-08-30
 * Modified by 
 *
 * Copyright (c) 2016-2017 Noovo Crop.  All rights reserved
 */

/*------------------------------------------------------------------------------
 Includes
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tuner_common.h"

#define DVB_SPI_FREQ 32000000  //32M Hz
#define SPI_BUS 0
#define SPI_DEVICE 0

/*------------------------------------------------------------------------------
  Variables
------------------------------------------------------------------------------*/
const char *Common_Result[13] = {"OK", "Argument Error", "I2C Error", "SW State Error", "HW State Error", "Timeout", "Unlock", "Out of Range", "No Support", "Cancelled", "Other Error", "Overflow", "OK - Confirm"};
const char *Common_System[5] = {"Unknown", "DVB-T", "DVB-T2", "ISDB-T", "Any"};
const char *Common_Bandwidth[9] = {"Unknown", "1.7MHz", "Invalid", "Invalid", "Invalid", "5MHz", "6MHz", "7MHz", "8MHz"};

/*------------------------------------------------------------------------------
  Functions for SPI case
------------------------------------------------------------------------------*/
sony_result_t sony_init_spi_controller (TUNER_DRIVER_CONTROLLER_T *pDriver)
{
	sony_result_t result = SONY_RESULT_OK;
	sony_tunerdemod_create_param_t createParam;

	/*
	 * Initialize SPI communication
	*/
	result = sony_spi_spidev_Initialize (&pDriver->spiDev, pDriver->spiBusNum, pDriver->spiChipSel, pDriver->spiMode, pDriver->spiFreq);
	if (result != SONY_RESULT_OK) {
		return result;
	}

	result = sony_spi_spidev_CreateSpi (&pDriver->spi, &pDriver->spiDev);
	if (result != SONY_RESULT_OK) {
		return result;
	}

	/* Register I/O struct instance creation */
	result = sony_regio_spi_Create (&pDriver->regio, &pDriver->spi, 0);
	if (result != SONY_RESULT_OK) {
		return result;
	}

	createParam.tsOutputIF = SONY_TUNERDEMOD_TSOUT_IF_SPI; /* SPI interface */
	createParam.enableInternalLDO = 1; /* Enable internal LDO */
	createParam.xtalShareType = SONY_TUNERDEMOD_XTAL_SHARE_NONE;
	createParam.xosc_cap = 8; /* Depend on circuit */
	createParam.xosc_i = 8; /* Depend on circuit */

	/*
	 * Initialize all member of sony_tunerdemod_t struct.
	 * Note : This API do NOT access to actual HW.
	*/
	result = sony_tunerdemod_Create (&pDriver->tunerDemod, &pDriver->regio, &createParam);
	if (result != SONY_RESULT_OK) {
		return result;
	}

	result = sony_tunermodule_Create(&pDriver->tunerDemod);
	if (result != SONY_RESULT_OK) {
		return result;
	}

	return SONY_RESULT_OK;
}

sony_result_t sony_finish_spi_controller (TUNER_DRIVER_CONTROLLER_T *pDriver)
{
	sony_result_t result = SONY_RESULT_OK;

	/* Finalize SPI communication */
	result = sony_spi_spidev_Finalize (&pDriver->spiDev);
	if (result != SONY_RESULT_OK) {
		return result;
	}

	return SONY_RESULT_OK;
}

/*handy function to initialize DVB SPI controller*/
sony_result_t initialize_DVB_SPI_controller (TUNER_DRIVER_CONTROLLER_T *pDriver) {
	sony_result_t result = SONY_RESULT_OK;
	// Driver instances.
	//TUNER_DRIVER_CONTROLLER_T controller
	pDriver->spiFreq    = DVB_SPI_FREQ;
	pDriver->spiBusNum  = SPI_BUS;
	pDriver->spiChipSel = SPI_DEVICE;
	pDriver->spiMode    = SONY_SPI_MODE_0;
	result = sony_init_spi_controller(pDriver);
	return result;
}

sony_result_t PrepareForSPIChannel(TUNER_DRIVER_CONTROLLER_T *pDriver) {
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


/*------------------------------------------------------------------------------
  Functions for SDIO case
------------------------------------------------------------------------------*/
sony_result_t sony_sdio_InitializeDriver (TUNER_DRIVER_CONTROLLER_T *pDriver)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_tunerdemod_create_param_t createParam;

    /*
     * Initialize SDIO communication
     * Note: Following case will open "cxd288x_sdio1" device node.
     */
    printf("sony_sdio_InitializeDriver entry\n");
    result = sony_sdio_cxd288x_sdio_Initialize (&pDriver->sdioDev, 1);
    if (result != SONY_RESULT_OK) {
        return result;
    }
    result = sony_sdio_cxd288x_sdio_CreateSdio (&pDriver->sdio, &pDriver->sdioDev);
    if (result != SONY_RESULT_OK) {
        return result;
    }
    /* Register I/O struct instance creation */
    result = sony_regio_sdio_Create (&pDriver->regio, &pDriver->sdio, 0);
    if (result != SONY_RESULT_OK) {
        return result;
    }
    createParam.tsOutputIF = SONY_TUNERDEMOD_TSOUT_IF_SDIO; /* SDIO interface */
    createParam.enableInternalLDO = 1; /* Enable internal LDO */
    createParam.xtalShareType = SONY_TUNERDEMOD_XTAL_SHARE_NONE;
    createParam.xosc_cap = 8; /* Depend on circuit */
    createParam.xosc_i = 8; /* Depend on circuit */
    /*
     * Initialize all member of sony_tunerdemod_t struct.
     * Note : This API do NOT access to actual HW.
     */
    result = sony_tunerdemod_Create (&pDriver->tunerDemod, &pDriver->regio, &createParam);
    if (result != SONY_RESULT_OK) {
        return result;
    }
    result = sony_tunermodule_Create(&pDriver->tunerDemod);
    if (result != SONY_RESULT_OK) {
        return result;
    }
    printf("sony_sdio_InitializeDriver exist\n");

    return SONY_RESULT_OK;
}



sony_result_t sony_sdio_FinalizeDriver (TUNER_DRIVER_CONTROLLER_T *pDriver)
{
    sony_result_t result = SONY_RESULT_OK;

    /* Finalize SDIO communication */
    result = sony_sdio_cxd288x_sdio_Finalize (&pDriver->sdioDev);
    if (result != SONY_RESULT_OK) {
        return result;
    }
    return SONY_RESULT_OK;
}


/*------------------------------------------------------------------------------
  Functions for DVB-T
------------------------------------------------------------------------------*/

// Setup DVB-T Tune
sony_result_t SetupDVBTTuneCommon(uint32_t c_freq, sony_dtv_bandwidth_t bandwidth, TUNER_DVB_SYSTEM_T dvb_sys, sony_tunerdemod_t *pTunerDemod)
{
	assert(pTunerDemod);

	sony_result_t result = SONY_RESULT_OK;
	if (dvb_sys == SYSTEM_DVBT)
	{
		sony_dvbt_tune_param_t pTuneParam;
		pTuneParam.centerFreqKHz = c_freq;
		pTuneParam.bandwidth = bandwidth;
		pTuneParam.profile = SONY_DVBT_PROFILE_HP;
		return sony_integ_dvbt_Tune(pTunerDemod, &pTuneParam);
	}
	else
	{
		sony_dvbt2_tune_param_t pTuneParam;
		pTuneParam.centerFreqKHz = c_freq;
		pTuneParam.bandwidth = bandwidth;
		pTuneParam.dataPLPID = 0;
		pTuneParam.profile = SONY_DVBT2_PROFILE_BASE;
		pTuneParam.tuneInfo = SONY_TUNERDEMOD_DVBT2_TUNE_INFO_OK;
		result = sony_integ_dvbt2_Tune(pTunerDemod, &pTuneParam);
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
			result = sony_tunerdemod_dvbt2_monitor_ActivePLP(pTunerDemod, SONY_DVBT2_PLP_DATA, &plpInfo);
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
