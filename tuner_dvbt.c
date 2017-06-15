/* Copyright 2016-2017 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by William Ho      2016-09-10
 * Modified by Wiliam Ho      2016-09-23 For DVB-T & DVBT-2 recorder.
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
static int ReadArgv(int argc, char *argv[]) {
	char* const short_options = "d:f:b:ph";
	int val = 0;
	int c;
	struct option long_options[] =
			{ { "dtv", 0, NULL, 'd' },
			  { "freq", 0, NULL, 'f' },
			  { "bandwidth", 0, NULL, 'b' },
			  { "program", no_argument, NULL, 'p' },
			  { "help", no_argument, NULL, 'h' },
			  { 0, 0, 0 }, };
	g_TuneFreq = TUNE_CFREQ_KHZ;
	g_BandWidth = TUNE_BANDWIDTH_8MHZ;
	g_dtv = DVB_T2;
	g_EnableDigProg = 0;
	static char *DTV_STANDARD_STR[] = { "Error", "DVB_T", "DVB_T2" };
	while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		switch (c) {
		case 'd':
			if ((sscanf(optarg, "%d", &val) != 1) || (val < 1) || (val > 2)) {
				printf("Warning: wrong DTV standard chosen, only 1 (DVB-T), 2 (DVB-T2)  \n");
			} else {
				g_dtv = val;
			}

			break;
		case 'f':
			if ((sscanf(optarg, "%d", &val) != 1) || (val < 400000) || (val > 900000)) {
				printf("Warning: wrong speed value (Hz)\n");
			} else {
				g_TuneFreq = val;
			}

			break;
		case 'b':
			if ((sscanf(optarg, "%d", &val) != 1) || (val < 1) || (val > 8)) {
				printf("Warning: wrong band width value (GHz), only 1,5,6,7,8\n");
			} else {
				g_BandWidth = val;
			}
			break;
		case 'p':
					g_EnableDigProg = 1;
					break;
		case 'h':
		default:
			printf("%s will read TV tuner data from SPI bus and transfer to share memory. \nVersion: %s\n",argv[0],APP_VERSION);
			printf("Usage %s [-d DTV_standard] [-f frequency_kHZ] [-b bandwidth_MHz] [-p] [-h] \n", argv[0]);
			printf("\tEx: %s -d 1 -f 633563 -b 6 \n", argv[0]);
			printf("\t-d --dtv : DTV standard, 1: DVB-T 2: DVB-T2\n");
			printf("\t-f --freq : Central frequency of tune.(kHz)\n");
			printf("\t-b --bandwidth :  Bandwidth of tune.(MHz)\n");
			printf("\t-p --program :  ENABLE program digging, default is disabled.\n");
			printf("\t-h --help : Show this help message\n");
			exit(0);
			break;
		}
	}
	printf("Type= %s, Freq= %d kHz, BandWidth= %d MHz, Digging prog=%s\n",DTV_STANDARD_STR[g_dtv], g_TuneFreq, g_BandWidth, g_EnableDigProg?"Enable":"Disable");
	return 0;
}

static sony_result_t PrepareForControl() {
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

// Setup DVBT Tune
static sony_result_t SetupDVBTTune() {
	sony_dvbt_tune_param_t pTuneParam;
	pTuneParam.centerFreqKHz = g_TuneFreq;
	pTuneParam.bandwidth = g_BandWidth;
	pTuneParam.profile = SONY_DVBT_PROFILE_HP;
	return sony_integ_dvbt_Tune(&controller.tunerDemod, &pTuneParam);
}

// Setup DVBT2 Tune 
static sony_result_t SetupDVBT2Tune() {
	//Data PLP ID where multiple PLP's are available.
	//const char *profile_str[] = { "T2-base", "T2-Lite", "Any" };
	sony_result_t result = SONY_RESULT_OK;
	sony_dvbt2_tune_param_t pTuneParam;
	pTuneParam.centerFreqKHz = g_TuneFreq;
	pTuneParam.bandwidth = g_BandWidth;
	pTuneParam.dataPLPID = 0;
	pTuneParam.profile = SONY_DVBT2_PROFILE_BASE;
	pTuneParam.tuneInfo = SONY_TUNERDEMOD_DVBT2_TUNE_INFO_OK;
	/*
	printf("WW: Tune to DVB-T2 signal with the following parameters:\n");
	printf(" - Center Freq    : %u kHz\n", pTuneParam.centerFreqKHz);
	printf(" - Bandwidth      : %s\n", Common_Bandwidth[pTuneParam.bandwidth]);
	printf(" - PLP ID         : %u\n", pTuneParam.dataPLPID);
	printf(" - Profile        : %s\n", profile_str[pTuneParam.profile]);
	*/
	result = sony_integ_dvbt2_Tune(&controller.tunerDemod, &pTuneParam);
	if ((result != SONY_RESULT_OK_CONFIRM)) {
		printf("Error : sony_integ_dvbt2_Tune failed. (result = %s)\n", Common_Result[result]);
		return result;
	}
	//Confirm PLP ID
	if ((result == SONY_RESULT_OK_CONFIRM) && (pTuneParam.tuneInfo == SONY_TUNERDEMOD_DVBT2_TUNE_INFO_INVALID_PLP_ID)) {
		sony_dvbt2_plp_t plpInfo;
		printf("PLP ID error in acquisition:\n");
		result = sony_tunerdemod_dvbt2_monitor_ActivePLP(&controller.tunerDemod, SONY_DVBT2_PLP_DATA, &plpInfo);
		if (result != SONY_RESULT_OK) {
			printf(" Error : sony_tunerdemod_dvbt2_monitor_ActivePLP failed. (result = %s)\n", Common_Result[result]);
			return result;
		}
		printf(" - PLP Requested : %u\n", pTuneParam.dataPLPID);
		printf(" - PLP Acquired  : %u\n\n", plpInfo.id);
	}
	return SONY_RESULT_OK;
}

static sony_result_t ReadSPITS(TUNER_DRIVER_CONTROLLER_T *pDriver) {
	sony_result_t result = SONY_RESULT_OK;
	int digret = 0;
	printf("------------------------------------------\n");
	printf("Reading SPI TS data\n");
	printf("------------------------------------------\n");
	/* Preparation should be done at TV application start timing. */
	result = InitializeTSRead();
	if (result != SONY_RESULT_OK) {
		return result;
	}
	/*Preparation Share memery */
	if (0 != Initialize_Shared_Memory()) {
		printf("Can't init share memory.\n");
		return SONY_RESULT_ERROR_OTHER;
	}
	/* Preparation should be done after every tuner/demod tuning. */
	result = PrepareForChannel(pDriver);
	if (result != SONY_RESULT_OK) {
		FinalizeTSRead();
		return result;
	}

	{
		uint32_t tsCount = 0;
		
		uint32_t nextTsCount = 50001;

		sony_tunerdemod_ts_buffer_info_t bufferInfo;
		uint8_t overflowDetected = 0;
		uint8_t underflowDetected = 0;
		printf("While loop entry....\n");
		/* TS reading loop. */
		while (1) {
			result = sony_devio_spi_ReadTSBufferInfo(&pDriver->spi, &bufferInfo);
			if (result != SONY_RESULT_OK) {
				printf("sony_example_spi_readts: sony_devio_spi_ReadTSBufferInfo failed\n");
				return result;
			}
			if ((bufferInfo.overflow == 1)) {
				printf("[%s %d] %d %d\n", __FUNCTION__, __LINE__, tsCount, bufferInfo.packetNum);
				printf("sony_example_spi_readts: SPI buffer is overflow\n");
				overflowDetected = 1;
			}
			if ((bufferInfo.underflow == 1)) {
				printf("[%s %d] %d %d\n", __FUNCTION__, __LINE__, tsCount, bufferInfo.packetNum);
				printf("sony_example_spi_readts: SPI buffer is underflow\n");
				underflowDetected = 1;
			}
			if (bufferInfo.readReady == 1) {
				result = sony_devio_spi_ReadTS(&pDriver->spi, g_pTSReadData, bufferInfo.packetNum);
				if (result != SONY_RESULT_OK) {
					printf("sony_example_spi_readts: sony_devio_spi_ReadTS failed\n");
					break;
				}
				//printf("[%s %d] %d %d\n", __FUNCTION__, __LINE__, tsCount, bufferInfo.packetNum);
				tsCount += bufferInfo.packetNum;
				if (tsCount >= nextTsCount) {
					printf("[%s %d] %d %d %d\n", __FUNCTION__, __LINE__, tsCount, bufferInfo.packetNum, nextTsCount);
					nextTsCount += 50001;
				}
				
#if 0
				//William : Dig channel when enable on argument, skipped after function return 1.
				//printf("Sent %d packets ", bufferInfo.packetNum);
				if ((digret == 0) && (g_EnableDigProg == 1)) {
					digret = DigChannelInfo(g_pTSReadData, bufferInfo.packetNum, g_TuneFreq, g_BandWidth);
					tsCount += bufferInfo.packetNum;
					printf("for digging channels....\n");
				} else {
					//Here we record data to share memory
					RecordTSData(g_pTSReadData, bufferInfo.packetNum, g_TuneFreq, g_BandWidth);
					//printf("for shared memory....\n");
				}
#endif
			}
		}
		printf("No channel data found at %d packets, aborted.\n", tsCount);
	}

	/* Finish TS reading. */
	FinalizeTSRead(pDriver);
	return result;
}

int main(int argc, char *argv[]) {
	ReadArgv(argc, argv);
	sony_result_t result = initialize_DVB_SPI_controller(&controller);
	if (result != SONY_RESULT_OK) {
		printf("Error : Cannot prepare for control (result = %s).\n", Common_Result[result]);
		return -1;
	}
	printf("sony_init_spi_controller succeeded.\n");
	// Initialize the device./
	result = sony_integ_Initialize(&controller.tunerDemod);
	if (result == SONY_RESULT_OK) {
		printf("Driver initialize done.\n");
	} else {
		printf("Error : Cannot run initialize (result = %s).\n", Common_Result[result]);
		return -1;
	}
	if (g_dtv == DVB_T2){
		result = SetupDVBT2Tune();
	}else{
		result = SetupDVBTTune();
	}
	if ((result != SONY_RESULT_OK) && (result != SONY_RESULT_OK_CONFIRM)) {
		printf("Error : Cannot setup tune (result = %s).\n", Common_Result[result]);
		return -1;
	} else {
		printf("SetupDVBTTune OK");
	}
	//Print RF Log for debug
	int32_t rfLevel;
	result = sony_tunerdemod_monitor_RFLevel(&controller.tunerDemod, &rfLevel);
	if (result == SONY_RESULT_OK) {
		printf("RF Level ==> %d x 10^-3 dBm\n", rfLevel);
	} else {
		printf("Error: RF Level N/A : %s\n", Common_Result[result]);
	}
	//Read TS Data
	result = ReadSPITS(&controller);
	if (result == SONY_RESULT_OK) {
		printf("SPI ReadTS OK\n");
	} else {
		printf("Error : SPI ReadTS Failed (result = %s)\n", Common_Result[result]);
		return -1;
	}
	printf("Processor all done\n");
	return 0;
}

