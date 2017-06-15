/* Copyright 2016-2017 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by William Ho  2017-03-29
 *
 * Copyright (c) 2016-2017 Noovo Crop.  All rights reserved
 */

/*------------------------------------------------------------------------------
 Includes
 ------------------------------------------------------------------------------*/
#include "tuner_dvbt.h"
/*------------------------------------------------------------------------------
 Globe Variables
 ------------------------------------------------------------------------------*/
int g_StartFreq;
int g_EndFreq;
int g_BandWidth;
int g_FoundChNum;
int g_EnableDigProg;
char *g_JSONFile;
uint8_t * g_pTSReadData;
DTV_STANDARD g_dtv;
TUNER_DRIVER_CONTROLLER_T controller;

/*------------------------------------------------------------------------------
 Argument Parser
 ------------------------------------------------------------------------------*/
static int ReadArgv(int argc, char *argv[])
{
	char* const short_options = "d:s:e:b:j:ph";
	int val = 0;
	int c;
	struct option long_options[] =
	{
	{ "dtv", 0, NULL, 'd' },
	{ "start-frequency", 0, NULL, 's' },
	{ "end-frequency", 0, NULL, 'e' },
	{ "bandwidth", 0, NULL, 'b' },
	{ "json", 0, NULL, 'j' },
	{ "program", no_argument, NULL, 'p' },
	{ "help", no_argument, NULL, 'h' },
	{ 0, 0, 0, 0 }, };
	g_StartFreq = 473000;
	g_EndFreq = 605000;
	g_BandWidth = 6;
	g_dtv = ISDB_T;
	g_EnableDigProg = 0;
	g_JSONFile = NULL;
	static char *DTV_STANDARD_STR[] =
	{ "Error", "ISDB_T" };

	while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1)
	{
		switch (c)
		{
		case 'd':
			if ((sscanf(optarg, "%d", &val) != 1) || (val < 1) || (val > 2))
			{
				printf("Warning: wrong DTV standard chosen, only 1 (ISDB-T)  \n");
			}
			else
			{
				g_dtv = val;
			}

			break;
		case 's':
			if ((sscanf(optarg, "%d", &val) != 1) || (val < 400000) || (val > 900000))
			{
				printf("Warning: wrong start frequency input, only 400000 ~ 900000\n");
				printf("Apply will apply default value: %d\n", g_StartFreq);
			}
			else
			{
				g_StartFreq = val;
			}

			break;
		case 'e':
			if ((sscanf(optarg, "%d", &val) != 1) || (val < 400000) || (val > 900000) || (val <= g_StartFreq))
			{
				printf("Warning: wrong end frequency input, only 400000 ~ 900000 and much than StartFreq \n");
				printf("Apply will apply default value: %d\n", g_EndFreq);
			}
			else
			{
				g_EndFreq = val;
			}

			break;
		case 'b':
			if ((sscanf(optarg, "%d", &val) != 1) || (val < 1) || (val > 8))
			{
				printf("Warning: wrong band width value (GHz), only 1,5,6,7,8\n");
				printf("Apply will apply default value: %d\n", g_BandWidth);
			}
			else
			{
				g_BandWidth = val;
			}
			break;
		case 'j':
			if ((sscanf(optarg, "%s", &val) != 1))
			{
				printf("No jason export \n");
			}
			else
			{
				g_JSONFile = optarg;
			}
			break;
		case 'p':
			g_EnableDigProg = 0;
			break;
		case 'h':
		default:
			printf("%s will scan TV channel from SPI bus. \nVersion: %s\n", argv[0], APP_VERSION);
			printf("Usage %s [-d DTV_standard] [-s Star_frequency_kHZ] "
					"[-s End_frequency_kHZ] [-b bandwidth_MHz] [-p] [-j export_filename ][-h] \n", argv[0]);
			printf("\tExample: %s -s 533000 -e 596000 -b 6 -j channel.json -p\n", argv[0]);
			printf("\t-s --start-frequency : Central frequency of tune.(kHz)\n");
			printf("\t-e --end-frequency : Central frequency of tune.(kHz)\n");
			printf("\t-b --bandwidth :  Bandwidth of tune.(MHz)\n");
			printf("\t-p --program :DISABLE program digging with every chanel.\n");
			printf("\t-j --jason :Export scan result into json file.\n");
			printf("\t-h --help : Show this help message\n");
			exit(0);
			break;
		}
	}
	printf("Run app as: type= %s, Star-freq= %d kHz, End-freq= %d kHz, BandWidth= %d MHz, Digging prog=%s ",
			DTV_STANDARD_STR[g_dtv], g_StartFreq, g_EndFreq, g_BandWidth, g_EnableDigProg ? "Enable" : "Disable");
	if ((NULL != g_JSONFile) && strlen(g_JSONFile) > 1)
	{
		printf(", export json to %s \n", g_JSONFile);
	}
	else
	{
		printf(", No export json\n");
	}
	return 0;
}

int main(int argc, char *argv[])
{
	ReadArgv(argc, argv);
	sony_result_t result = SONY_RESULT_OK;
	/* Driver instances. Please see tuner_common.h in detail. */
	TUNER_SCAN_PARAMETER_T param;

	/* Set SPI device parameters */
	controller.spiFreq = SPI_FREQ;
	/* Following case will open "spidev0.0" device node */
	controller.spiBusNum = SPI_BUS;
	controller.spiChipSel = SPI_DEVICE;
	controller.spiMode = SONY_SPI_MODE_0;
	g_FoundChNum = 0;

	/* Set ISDBT Parameter */
	param.dvbSytem = SYSTEM_ISDBT;
	param.bandwidth = g_BandWidth;
	param.freqStart = g_StartFreq;
	param.freqEnd = g_EndFreq;
	param.freqStep = g_BandWidth * 1000;

	/* Display driver credentials */
	printf("Driver Version : %s\n", SONY_TUNERDEMOD_DRIVER_VERSION);
	printf("Tuner scanner start\n");
	//Initial controller
	result = sony_init_spi_controller(&controller);
	if (result != SONY_RESULT_OK)
	{
		printf("Error : sony_init_spi_controller failed. (result = %s)\n", Common_Result[result]);
		return -1;
	}
	printf("sony_init_spi_controller succeeded.\n");
	result = sony_integ_Initialize(&controller.tunerDemod);
	if (result == SONY_RESULT_OK)
	{
		printf("Driver initialized, current state = SONY_TUNERDEMOD_STATE_SLEEP\n");
	}
	else
	{
		printf("Error : sony_integ_Initialize failed. (result = %s)\n", Common_Result[result]);
		return -1;
	}

	printf("------------------------------------------\n");
	printf(" Scan\n");
	printf("------------------------------------------\n");

	{

		/* Create table header for the scan results */
		printf("\nScan Results:\n");
		if (controller.tunerDemod.diverMode == SONY_TUNERDEMOD_DIVERMODE_MAIN)
		{
			printf("-----|--------|---------|---------|----------+----------|\n");
			printf("  %%  |  FREQ  | TS LOCK | SNR(dB) |  RFLEVEL    (Sub)   |\n");
			printf("-----|--------|---------|---------|----------+----------|\n");
		}
		else
		{
			printf("-----|--------|---------|---------|----------|\n");
			printf("  %%  |  FREQ  | TS LOCK | SNR(dB) |  RFLEVEL |\n");
			printf("-----|--------|---------|---------|----------|\n");
		}
		sony_integ_isdbt_scan_param_t scanParam;
		scanParam.bandwidth = g_BandWidth;
		scanParam.startFrequencyKHz = param.freqStart; /* Start frequency of scan process */
		scanParam.endFrequencyKHz = param.freqEnd; /* End frequency of scan process */
		scanParam.stepFrequencyKHz = param.freqStep; /* Step frequency between attempted tunes in scan */
		result = sony_integ_isdbt_Scan(&controller.tunerDemod, &scanParam, &isdbtScanCallbackHandler);
		printf("\n - Result         : %s\n", Common_Result[result]);
	}

	result = sony_tunerdemod_Sleep(&controller.tunerDemod);
	if (result != SONY_RESULT_OK)
	{
		printf("Error : sony_tunerdemod_Sleep failed. (result = %s)\n", Common_Result[result]);
		return -1;
	}

	/* Finalize communication. Please see tuner_common.c in detail. */
	result = sony_finish_spi_controller(&controller);
	if (result != SONY_RESULT_OK)
	{
		printf("Error : sony_finish_spi_controller failed. (result = %s)\n", Common_Result[result]);
		return -1;
	}
	//Here we dump data in Jason format.
	if ((NULL != g_JSONFile) && strlen(g_JSONFile) > 1)
	{
		exportJSON(g_JSONFile);
	}
	return 0;
}

