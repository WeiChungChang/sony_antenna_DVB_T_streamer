/* Copyright 2016-2017 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by JackHsu      2016-08-30
 * Modified by 
 *
 * Copyright (c) 2016-2017 Noovo Crop.  All rights reserved
 */

#ifndef __TUNER_COMMON_H__
#define __TUNER_COMMON_H__

/*------------------------------------------------------------------------------
  Includes
------------------------------------------------------------------------------*/
#include "sony_tunerdemod.h"
#include "sony_tunerdemod_dvbt.h"
#include "sony_tunerdemod_dvbt2.h"
#include "sony_tunermodule.h"
#include "sony_devio_i2c.h"
#include "sony_devio_sdio.h"
#include "sony_devio_spi.h"

#if defined(S900_SDIO)
#include "sony_sdio_cxd288x_sdio.h"
#elif defined(LINUX_SDIO)
#include "sony_sdio_csdio.h"
#endif
#include "sony_spi_spidev.h"

#include "sony_tunerdemod_driver_version.h"

/*------------------------------------------------------------------------------
  Enumerations
------------------------------------------------------------------------------*/
typedef enum {
	SYSTEM_DVBT,                  /*DVB-T system */
	SYSTEM_DVBT2,                 /*DVB-T2 system */
	SYSTEM_ISDBT,                 /*ISDBT system */
} TUNER_DVB_SYSTEM_T;

typedef enum {
	BANDWIDTH_1_7_MHZ = 1,        /*1.7 MHZ bandwidth */
	BANDWIDTH_5_MHZ = 5,          /*5 MHZ bandwidth */
	BANDWIDTH_6_MHZ = 6,          /*6 MHZ bandwidth */
	BANDWIDTH_7_MHZ = 7,          /*7 MHZ bandwidth */
	BANDWIDTH_8_MHZ = 8,          /*8 MHZ bandwidth */
} TUNER_DVB_BANDWIDTH_T;

/*------------------------------------------------------------------------------
  Structs
------------------------------------------------------------------------------*/
typedef struct TUNER_DRIVER_CONTROLLER {
	sony_tunerdemod_t        tunerDemod;    /**< Main IC driver struct instance */
	sony_regio_t             regio;         /**< Register I/O struct instance */
	sony_tunerdemod_t        tunerDemodSub;    /**< Main IC driver struct instance */
	sony_regio_t             regioSub;         /**< Register I/O struct instance */


	/* Necessary if I2C is used */
	sony_i2c_t               i2c;           /**< I2C struct (necessary if I2C is used) */

	/* Necessary if SDIO is used */
	sony_sdio_t              sdio;          /**< SDIO struct (necessary if SDIO is used) */
#ifdef S900_SDIO
	sony_sdio_cxd288x_sdio_t sdioDev;       /**< "cxd288x_sdio" kernel driver dependent information */
#endif

	/* Necessary if SPI is used */
	sony_spi_t               spi;           /**< SPI struct (necessary if SPI is used) */
	sony_spi_spidev_t        spiDev;        /**< "spidev" kernel driver dependent information */
	int                      spiFreq;       /**< SPI clock frequence */
	int                      spiBusNum;     /**< SPI Bus number */
	int                      spiChipSel;    /**< SPI Chip select */
	sony_spi_mode_t          spiMode;       /**< SPI mode */
} TUNER_DRIVER_CONTROLLER_T;

typedef struct TUNER_SCAN_PARAMETER {
	TUNER_DVB_SYSTEM_T       dvbSytem;      /* Set DVB-T or DVB-T2 system */
	TUNER_DVB_BANDWIDTH_T    bandwidth;     /* Set bandwidth */
	int                      freqStart;     /* Set start frequence */
	int                      freqEnd;       /* Set end frequence */
	int                      freqStep;      /* Set step frequence */
} TUNER_SCAN_PARAMETER_T;


/*------------------------------------------------------------------------------
  Variables
------------------------------------------------------------------------------*/
extern const char *Common_Result[13];      /**< Table to convert sony_result_t value to character */
extern const char *Common_System[5];       /**< Table to convert sony_dtv_system_t value to character */
extern const char *Common_Bandwidth[9];    /**< Table to convert sony_dtv_bandwidth_t value to character */

/*------------------------------------------------------------------------------
  Functions
------------------------------------------------------------------------------*/
extern sony_result_t sony_init_spi_controller (TUNER_DRIVER_CONTROLLER_T *pDriver);
extern sony_result_t sony_finish_spi_controller (TUNER_DRIVER_CONTROLLER_T *pDriver);
extern sony_result_t initialize_DVB_SPI_controller (TUNER_DRIVER_CONTROLLER_T *pDriver);
extern sony_result_t PrepareForSPIChannel(TUNER_DRIVER_CONTROLLER_T *pDriver);

extern sony_result_t sony_sdio_InitializeDriver (TUNER_DRIVER_CONTROLLER_T *pDriver);
extern sony_result_t sony_sdio_diver_InitializeDriver (TUNER_DRIVER_CONTROLLER_T *pDriver);
extern sony_result_t sony_sdio_FinalizeDriver (TUNER_DRIVER_CONTROLLER_T *pDriver);

extern sony_result_t SetupDVBTTuneCommon(uint32_t c_freq, sony_dtv_bandwidth_t bandwidth, TUNER_DVB_SYSTEM_T dvb_sys, sony_tunerdemod_t *pTunerDemod);

#endif //__TUNER_COMMON_H__

