#include "tuner_dvbt.h"

int32_t carrierOffset = 0;
int32_t snr = 0;
uint8_t sqi = 0;
uint8_t ssi = 0;
uint8_t ssi_sub = 0;
int32_t rfLevel = 0;
uint32_t preViterbiBER = 0;
uint32_t preRSBER = 0;
uint32_t packetErrorNumber = 0;
uint32_t preLDPCBER = 0;
uint32_t postBCHFER = 0;
uint32_t preBCHBER = 0;
uint32_t PER = 0;

void dvbt_t2ScanCallbackHandler(sony_tunerdemod_t * pTunerDemod, sony_integ_dvbt_t2_scan_result_t * pResult,
		sony_integ_dvbt_t2_scan_param_t * pScanParam)
{
	sony_result_t result = SONY_RESULT_OK;
	if (pResult->tuneResult == SONY_RESULT_OK)
	{
		char * tsLock;
		/* Callback contains channel information, so output channel information to table. */
		printf("[%s %d] pResult->system %d\n", __FUNCTION__, __LINE__, pResult->system);
		if (pResult->system == SONY_DTV_SYSTEM_DVBT)
		{
			result = sony_integ_dvbt_WaitTSLock(pTunerDemod);
			tsLock = (result == SONY_RESULT_OK) ? "Yes" : " No";
			Channels[g_FoundChNum].tsLock = (result == SONY_RESULT_OK) ? 1 : 0;

			/* Allow the monitors time to settle */
			SONY_SLEEP(1000);

			/* Get carrier offset to compensate frequency. */
			result = sony_tunerdemod_dvbt_monitor_CarrierOffset(pTunerDemod, &carrierOffset);
			if (result != SONY_RESULT_OK)
			{
				carrierOffset = 0;

				if (pTunerDemod->diverMode == SONY_TUNERDEMOD_DIVERMODE_MAIN)
				{
					/* Use sub IC value. */
					result = sony_tunerdemod_dvbt_monitor_CarrierOffset_sub(pTunerDemod, &carrierOffset);
					if (result != SONY_RESULT_OK)
					{
						carrierOffset = 0;
					}
				}
			}

			/* Hz to kHz */
			if (carrierOffset > 0)
			{
				carrierOffset += 500;
			}
			else
			{
				carrierOffset -= 500;
			}
			carrierOffset /= 1000;
			//Here save CentralFreqKHz into channel array.
			Channels[g_FoundChNum].CentralFreqKHz = pResult->dvbtTuneParam.centerFreqKHz + carrierOffset;
			//William: For redmine #256, added some parament:
			result = sony_tunerdemod_dvbt_monitor_PreViterbiBER(pTunerDemod, &preViterbiBER);
			if (result != SONY_RESULT_OK)
			{
				preViterbiBER = 0;
				printf("sony_tunerdemod_dvbt_monitor_PreViterbiBER Error\n");
			}
			Channels[g_FoundChNum].preViterbiBER = preViterbiBER;

			result = sony_tunerdemod_dvbt_monitor_PreRSBER(pTunerDemod, &preRSBER);
			if (result != SONY_RESULT_OK)
			{
				preRSBER = 0;
				printf("sony_tunerdemod_dvbt_monitor_PreRSBER Error\n");
			}
			Channels[g_FoundChNum].preRSBER = preRSBER;

			result = sony_tunerdemod_dvbt_monitor_PacketErrorNumber(pTunerDemod, &packetErrorNumber);
			if (result != SONY_RESULT_OK)
			{
				packetErrorNumber = 0;
				printf("sony_tunerdemod_dvbt_monitor_PacketErrorNumber Error\n");
			}
			Channels[g_FoundChNum].packetErrorNumber = packetErrorNumber;

			result = sony_tunerdemod_dvbt_monitor_PER(pTunerDemod, &PER);
			if (result != SONY_RESULT_OK)
			{
				printf("sony_tunerdemod_dvbt_monitor_PER Error\n");
				PER = 0;
			}
			Channels[g_FoundChNum].PER = PER;

			result = sony_tunerdemod_dvbt_monitor_SNR(pTunerDemod, &snr);
			if (result != SONY_RESULT_OK)
			{
				snr = 0;
			}
			Channels[g_FoundChNum].snr = snr;

			result = sony_tunerdemod_dvbt_monitor_Quality(pTunerDemod, &sqi);
			if (result != SONY_RESULT_OK)
			{
				sqi = 0;
			}
			Channels[g_FoundChNum].sqi = sqi;

			result = sony_tunerdemod_monitor_RFLevel(pTunerDemod, &rfLevel);
			if (result != SONY_RESULT_OK)
			{
				rfLevel = 0;
			}
			Channels[g_FoundChNum].rfLevel = (rfLevel / 1000);

			result = sony_tunerdemod_dvbt_monitor_SSI(pTunerDemod, &ssi);
			if (result != SONY_RESULT_OK)
			{
				ssi = 0;
			}
			Channels[g_FoundChNum].ssi = ssi;

			if (pTunerDemod->diverMode == SONY_TUNERDEMOD_DIVERMODE_MAIN)
			{
				result = sony_tunerdemod_dvbt_monitor_SSI_sub(pTunerDemod, &ssi_sub);
				if (result != SONY_RESULT_OK)
				{
					ssi_sub = 0;
				}
				Channels[g_FoundChNum].ssi_sub = ssi_sub;
				printf(" %3u | DVB-T  | %6u | --- | ------- |     %s |  %2u.%03u | %3u | %3u | %3u | %d  |\n",
						((pResult->centerFreqKHz - pScanParam->startFrequencyKHz) * 100)
								/ (pScanParam->endFrequencyKHz - pScanParam->startFrequencyKHz),
						Channels[g_FoundChNum].CentralFreqKHz, /* Compensated frequency */
						tsLock, snr / 1000, snr % 1000, sqi, ssi, ssi_sub, rfLevel / 1000);
			}
			else
			{
				printf(" %3u | DVB-T  | %6u | --- | ------- |     %s |  %2u.%03u | %3u | %3u | %d  |\n",
						((pResult->centerFreqKHz - pScanParam->startFrequencyKHz) * 100)
								/ (pScanParam->endFrequencyKHz - pScanParam->startFrequencyKHz),
						Channels[g_FoundChNum].CentralFreqKHz, /* Compensated frequency */
						tsLock, snr / 1000, snr % 1000, sqi, ssi, rfLevel / 1000);
			}
			//Here save CentralFreqKHz into channel array.
			g_FoundChNum++;
		}
		else if (pResult->system == SONY_DTV_SYSTEM_DVBT2)
		{
			result = sony_integ_dvbt2_WaitTSLock(pTunerDemod, pResult->dvbt2TuneParam.profile);
			tsLock = (result == SONY_RESULT_OK) ? "Yes" : " No";
			Channels[g_FoundChNum].tsLock = (result == SONY_RESULT_OK) ? 1 : 0;

			/* Allow the monitors time to settle */
			SONY_SLEEP(1000);

			/* Get carrier offset to compensate frequency. */
			result = sony_tunerdemod_dvbt2_monitor_CarrierOffset(pTunerDemod, &carrierOffset);
			if (result != SONY_RESULT_OK)
			{
				carrierOffset = 0;

				if (pTunerDemod->diverMode == SONY_TUNERDEMOD_DIVERMODE_MAIN)
				{
					/* Use sub IC value. */
					result = sony_tunerdemod_dvbt2_monitor_CarrierOffset_sub(pTunerDemod, &carrierOffset);
					if (result != SONY_RESULT_OK)
					{
						carrierOffset = 0;
					}
				}
			}

			/* Hz to kHz */
			if (carrierOffset > 0)
			{
				carrierOffset += 500;
			}
			else
			{
				carrierOffset -= 500;
			}
			carrierOffset /= 1000;
			//Here save CentralFreqKHz into channel array.
			Channels[g_FoundChNum].CentralFreqKHz = pResult->dvbt2TuneParam.centerFreqKHz + carrierOffset;

			//William: For redmine #256, added some parament:
			result = sony_tunerdemod_dvbt2_monitor_PreLDPCBER(pTunerDemod, &preLDPCBER);
			if (result != SONY_RESULT_OK)
			{
				printf("sony_tunerdemod_dvbt2_monitor_PreLDPCBER Error\n");
				preLDPCBER = 0;
			}
			Channels[g_FoundChNum].preLDPCBER = preLDPCBER;

			result = sony_tunerdemod_dvbt2_monitor_PostBCHFER(pTunerDemod, &postBCHFER);
			if (result != SONY_RESULT_OK)
			{
				printf("sony_tunerdemod_dvbt2_monitor_PostBCHFER Error\n");
				postBCHFER = 0;
			}
			Channels[g_FoundChNum].postBCHFER = postBCHFER;
			result = sony_tunerdemod_dvbt2_monitor_PreBCHBER(pTunerDemod, &preBCHBER);
			if (result != SONY_RESULT_OK)
			{
				printf("sony_tunerdemod_dvbt2_monitor_PreBCHBER Error\n");
				preBCHBER = 0;
			}
			Channels[g_FoundChNum].PreBCHBER = preBCHBER;

			result = sony_tunerdemod_dvbt2_monitor_PacketErrorNumber(pTunerDemod, &packetErrorNumber);
			if (result != SONY_RESULT_OK)
			{
				printf("sony_tunerdemod_dvbt2_monitor_PacketErrorNumber Error\n");
				packetErrorNumber = 0;
			}
			Channels[g_FoundChNum].packetErrorNumber = packetErrorNumber;

			result = sony_tunerdemod_dvbt2_monitor_PER(pTunerDemod, &PER);
			if (result != SONY_RESULT_OK)
			{
				printf("sony_tunerdemod_dvbt2_monitor_PER Error\n");
				PER = 0;
			}
			Channels[g_FoundChNum].PER = PER;

			result = sony_tunerdemod_dvbt2_monitor_SNR(pTunerDemod, &snr);
			if (result != SONY_RESULT_OK)
			{
				snr = 0;
			}
			Channels[g_FoundChNum].snr = snr;

			result = sony_tunerdemod_dvbt2_monitor_SNR(pTunerDemod, &snr);
			if (result != SONY_RESULT_OK)
			{
				snr = 0;
			}
			Channels[g_FoundChNum].snr = snr;

			result = sony_tunerdemod_dvbt2_monitor_Quality(pTunerDemod, &sqi);
			if (result != SONY_RESULT_OK)
			{
				sqi = 0;
			}
			Channels[g_FoundChNum].sqi = sqi;

			result = sony_tunerdemod_monitor_RFLevel(pTunerDemod, &rfLevel);
			if (result != SONY_RESULT_OK)
			{
				rfLevel = 0;
			}
			Channels[g_FoundChNum].rfLevel = (rfLevel / 1000);

			result = sony_tunerdemod_dvbt2_monitor_SSI(pTunerDemod, &ssi);
			if (result != SONY_RESULT_OK)
			{
				ssi = 0;
			}
			Channels[g_FoundChNum].ssi = ssi;

			if (pTunerDemod->diverMode == SONY_TUNERDEMOD_DIVERMODE_MAIN)
			{
				result = sony_tunerdemod_dvbt2_monitor_SSI_sub(pTunerDemod, &ssi_sub);
				if (result != SONY_RESULT_OK)
				{
					ssi_sub = 0;
				}
				Channels[g_FoundChNum].ssi_sub = ssi_sub;

				printf(" %3u | DVB-T2 | %6u | %3u |    %s |     %s |  %2u.%03u | %3u | %3u | %3u | %d  |\n",
						((pResult->centerFreqKHz - pScanParam->startFrequencyKHz) * 100)
								/ (pScanParam->endFrequencyKHz - pScanParam->startFrequencyKHz),
						Channels[g_FoundChNum].CentralFreqKHz, /* Compensated frequency */
						pResult->dvbt2TuneParam.dataPLPID,
						pResult->dvbt2TuneParam.profile == SONY_DVBT2_PROFILE_LITE ? "Lite" : "Base", tsLock,
						snr / 1000, snr % 1000, sqi, ssi, ssi_sub, rfLevel / 1000);
			}
			else
			{
				printf(" %3u | DVB-T2 | %6u | %3u |    %s |     %s |  %2u.%03u | %3u | %3u | %d  |\n",
						((pResult->centerFreqKHz - pScanParam->startFrequencyKHz) * 100)
								/ (pScanParam->endFrequencyKHz - pScanParam->startFrequencyKHz),
						Channels[g_FoundChNum].CentralFreqKHz, /* Compensated frequency */
						pResult->dvbt2TuneParam.dataPLPID,
						pResult->dvbt2TuneParam.profile == SONY_DVBT2_PROFILE_LITE ? "Lite" : "Base", tsLock,
						snr / 1000, snr % 1000, sqi, ssi, rfLevel / 1000);
			}
			//Here save data into channel array.
			g_FoundChNum++;
		}
		else
		{
			/* Never occured */
			printf("Unknown system. (%s)\n", Common_System[pResult->system]);
		}
	}
	else
	{
		/* Callback is for progress only */
		if (pTunerDemod->diverMode == SONY_TUNERDEMOD_DIVERMODE_MAIN)
		{
			printf(" %3u | ------ | ------ | --- | ------- | ------- | ------- | --- | --- | --- |------|\n",
					((pResult->centerFreqKHz - pScanParam->startFrequencyKHz) * 100)
							/ (pScanParam->endFrequencyKHz - pScanParam->startFrequencyKHz));
		}
		else
		{
			printf(" %3u | ------ | ------ | --- | ------- | ------- | ------- | --- | --- |------|\n",
					((pResult->centerFreqKHz - pScanParam->startFrequencyKHz) * 100)
							/ (pScanParam->endFrequencyKHz - pScanParam->startFrequencyKHz));
		}
	}
}

void isdbtScanCallbackHandler(sony_tunerdemod_t * pTunerDemod, sony_integ_isdbt_scan_result_t * pResult,
		sony_integ_isdbt_scan_param_t * pScanParam)
{
	sony_result_t result = SONY_RESULT_OK;

	if (pResult->tuneResult == SONY_RESULT_OK)
	{
		char * tsLock;
		int32_t rfLevel_sub = 0;

		/* Callback contains channel information, so output channel information to table. */
		result = sony_integ_isdbt_WaitTSLock(pTunerDemod);
		tsLock = (result == SONY_RESULT_OK) ? "Yes" : " No";

		/* Allow the monitors time to settle */
		SONY_SLEEP(1000);

		/* Get carrier offset to compensate frequency. */
		result = sony_tunerdemod_isdbt_monitor_CarrierOffset(pTunerDemod, &carrierOffset);
		if (result != SONY_RESULT_OK)
		{
			carrierOffset = 0;

			if (pTunerDemod->diverMode == SONY_TUNERDEMOD_DIVERMODE_MAIN)
			{
				/* Use sub IC value. */
				result = sony_tunerdemod_isdbt_monitor_CarrierOffset_sub(pTunerDemod, &carrierOffset);
				if (result != SONY_RESULT_OK)
				{
					carrierOffset = 0;
				}
			}
		}

		/* Hz to kHz */
		if (carrierOffset > 0)
		{
			carrierOffset += 500;
		}
		else
		{
			carrierOffset -= 500;
		}
		carrierOffset /= 1000;
		Channels[g_FoundChNum].CentralFreqKHz = pResult->tuneParam.centerFreqKHz + carrierOffset;

		result = sony_tunerdemod_isdbt_monitor_SNR(pTunerDemod, &snr);
		if (result != SONY_RESULT_OK)
		{
			snr = 0;
		}
		Channels[g_FoundChNum].snr = snr;

		result = sony_tunerdemod_monitor_RFLevel(pTunerDemod, &rfLevel);
		if (result != SONY_RESULT_OK)
		{
			rfLevel = -999999;
		}
		Channels[g_FoundChNum].rfLevel = (rfLevel / 1000);

		if (pTunerDemod->diverMode == SONY_TUNERDEMOD_DIVERMODE_MAIN)
		{
			result = sony_tunerdemod_monitor_RFLevel_sub(pTunerDemod, &rfLevel_sub);
			if (result != SONY_RESULT_OK)
			{
				rfLevel_sub = -999999;
			}

			printf(" %3u | %6u |     %s |  %2u.%03u | %4d.%03u | %4d.%03u |\n",
					((pResult->centerFreqKHz - pScanParam->startFrequencyKHz) * 100)
							/ (pScanParam->endFrequencyKHz - pScanParam->startFrequencyKHz),
					pResult->tuneParam.centerFreqKHz + carrierOffset, /* Compensated frequency */
					tsLock, snr / 1000, snr % 1000, -((-rfLevel) / 1000), (-rfLevel) % 1000, /* RF Level < 0 */
					-((-rfLevel_sub) / 1000), (-rfLevel_sub) % 1000);
		}
		else
		{
			printf(" %3u | %6u |     %s |  %2u.%03u | %4d.%03u |\n",
					((pResult->centerFreqKHz - pScanParam->startFrequencyKHz) * 100)
							/ (pScanParam->endFrequencyKHz - pScanParam->startFrequencyKHz),
					pResult->tuneParam.centerFreqKHz + carrierOffset, /* Compensated frequency */
					tsLock, snr / 1000, snr % 1000, -((-rfLevel) / 1000), (-rfLevel) % 1000); /* RF Level < 0 */
		}
		//Here save CentralFreqKHz into channel array.
		g_FoundChNum++;
	}
	else
	{
		/* Callback is for progress only */
		if (pTunerDemod->diverMode == SONY_TUNERDEMOD_DIVERMODE_MAIN)
		{
			printf(" %3u | ------ | ------- | ------- | -------- | -------- |\n",
					((pResult->centerFreqKHz - pScanParam->startFrequencyKHz) * 100)
							/ (pScanParam->endFrequencyKHz - pScanParam->startFrequencyKHz));
		}
		else
		{
			printf(" %3u | ------ | ------- | ------- | -------- |\n",
					((pResult->centerFreqKHz - pScanParam->startFrequencyKHz) * 100)
							/ (pScanParam->endFrequencyKHz - pScanParam->startFrequencyKHz));
		}
	}
}
