/* Copyright 2016-2017 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by william.ho@noovo.co 
 *
 * Copyright (c) 2016-2017 Noovo Crop.  All rights reserved
 */

#ifndef __TUNER_SPI2TS_H__
#define __TUNER_SPI2TS_H__
/*------------------------------------------------------------------------------
 Includes
 ------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include "tuner_common.h"
#include "sony_integ_dvbt_t2.h"
#include "sony_tunerdemod_dvbt_monitor.h"
#include "sony_tunerdemod_dvbt2_monitor.h"
#include "sony_tunerdemod.h"
#include "sony_tunerdemod_dvbt.h"
#include "sony_dvbt.h"

/*------------------------------------------------------------------------------
 Defines
 ------------------------------------------------------------------------------*/
/* Number of TS packets to read. */
#define SPI_FREQ 15000000  //15M Hz
#define SPI_BUS 0
#define SPI_DEVICE 0  
#define READ_TS_PACKET_MAX (100000)
#define TS_FILE_NAME "output.ts"
//#define TUNE_CFREQ_KHZ 544988 //That should be Taiwan Public TV.
#define TUNE_CFREQ_KHZ 477000 //That should be Euro DTV.

/*------------------------------------------------------------------------------
 Variables
 ------------------------------------------------------------------------------*/
static FILE * g_outputFile;
static uint8_t * g_pTSReadData;

/*------------------------------------------------------------------------------
 Functions
 ------------------------------------------------------------------------------*/
static sony_result_t InitializeTSRead();
static sony_result_t FinalizeTSRead();
static sony_result_t PrepareForChannel(TUNER_DRIVER_CONTROLLER_T *pDriver);

#endif //__TUNER_SPI2TS_H__
