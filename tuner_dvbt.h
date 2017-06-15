/* Copyright 2016-2017 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by william.ho@noovo.co 
 *
 * Copyright (c) 2016-2017 Noovo Crop.  All rights reserved
 */

#ifndef __TUNER_DVBT_H__
#define __TUNER_DVBT_H__
/*------------------------------------------------------------------------------
 Includes
 ------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include "tuner_common.h"
#include "sony_integ_dvbt_t2.h"
#include "sony_tunerdemod_dvbt_monitor.h"
#include "sony_tunerdemod_dvbt2_monitor.h"
#include "sony_tunerdemod_isdbt_monitor.h"
#include "sony_integ_isdbt.h"
#include "sony_tunerdemod_monitor.h"
#include "sony_tunerdemod.h"
#include "sony_tunerdemod_dvbt.h"
#include "sony_dvbt.h"
#include "noovo_common.h"
#include "noovo_mesgdef.h"
#include "libtsmgr/libtsmgr_tsfmt.h"
#include "libtsmgr/libtsmgr_shmdef.h"
#include "libtsmgr/libtsmgr_api.h"

/*------------------------------------------------------------------------------
 Defines
 ------------------------------------------------------------------------------*/
#define APP_VERSION "2017.0330"  //Version string
#define SPI_FREQ 32000000  //32M Hz
#define SPI_BUS 0
#define SPI_DEVICE 0  
#define READ_TS_PACKET_MAX (100000)
#define TS_FILE_NAME "output.ts"
#define TUNE_CFREQ_KHZ 477000 //For DVB-T2 testing.
#define TUNE_BANDWIDTH_6MHZ 6 //That should be Taiwan DVB-T default.
#define TUNE_BANDWIDTH_8MHZ 8 //That should be Euro DVB-T2 default.
#define PROGRAM_TIMEOUT 5  //Reading programs time out: 5 seconds

/*------------------------------------------------------------------------------
 Defines
 ------------------------------------------------------------------------------*/
typedef enum digital_tv_standards
{
	DVB_T = 1, DVB_T2 = 2, ISDB_T = 3
} DTV_STANDARD;

/*------------------------------------------------------------------------------
 Globe Variables
 ------------------------------------------------------------------------------*/
extern int g_TuneFreq;
extern int g_StartFreq;
extern int g_EndFreq;
extern int g_BandWidth;
extern int g_FoundChNum;
extern int g_EnableDigProg;
extern uint8_t * g_pTSReadData;
extern DTV_STANDARD g_dtv;
extern TUNER_DRIVER_CONTROLLER_T controller;
//Here we setup channel data as an array, 32 channel should be enough.
struct ChannelArr
{
	int32_t CentralFreqKHz;
	int32_t rfLevel;
	int8_t tsLock;
	int32_t snr;
	uint8_t sqi;
	uint8_t ssi;
	uint8_t ssi_sub;
	uint32_t preViterbiBER;
	uint32_t preRSBER;
	uint32_t packetErrorNumber;
	uint32_t postBCHFER;
	uint32_t PreBCHBER;
	uint32_t preLDPCBER;
	uint32_t PER;
} Channels[32];

/*------------------------------------------------------------------------------
 Functions
 ------------------------------------------------------------------------------*/

int Initialize_Shared_Memory();
int DigChannelInfo(uint8_t * pData, uint32_t packetNum, int tune_freq, int band_width);
uint32_t RecordTSData(uint8_t * pData, uint32_t packetNum, int tune_freq, int band_width);

void dvbt_t2ScanCallbackHandler(sony_tunerdemod_t * pTunerDemod, sony_integ_dvbt_t2_scan_result_t * pResult,
		sony_integ_dvbt_t2_scan_param_t * pScanParam);
void isdbtScanCallbackHandler (sony_tunerdemod_t * pTunerDemod, sony_integ_isdbt_scan_result_t * pResult, 
		sony_integ_isdbt_scan_param_t * pScanParam);
int Initialize_Shared_Memory();
void exportJSON(char * filename);
sony_result_t FinalizeTSRead();
sony_result_t InitializeTSRead();
sony_result_t PrepareForChannel(TUNER_DRIVER_CONTROLLER_T *pDriver);
sony_result_t FindChannelsProg();
#endif //__TUNER_DVBT_H__
