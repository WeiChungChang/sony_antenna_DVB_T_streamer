/* Copyright 2016-2017 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by JackHsu      2016-09-02
 * Modified by 
 *
 * Copyright (c) 2016-2017 Noovo Crop.  All rights reserved
 */
#define POCKET_LEN 188

#include <stdio.h>
#include "noovo_common.h"
#include "noovo_mesgdef.h"

#include "libtsmgr/libtsmgr_tsfmt.h"
#include "libtsmgr/libtsmgr_shmdef.h"
#include "libtsmgr/libtsmgr_api.h"

static SHARED_MEMORY_MGR *gTsMgr = NULL;
static unsigned char *gTsDataBuffer = NULL;
static TS_SCAN_PAT_INFO PadInfo = { 0 };
static TS_SCAN_INFO ScanInfo = { 0 };
static int IsGetPatFlag = 0;
static int Passed10K = 0;

/*
 * Noovo Initialize_Shared_Memory
 * Description: Initial Transport stream's share memory (Manager and Data buffer)
 * @param: None
 * @return: SONY_RESULT_ERROR_OTHER
 *          SONY_RESULT_OK
 */
int Initialize_Shared_Memory() {
	if (libtsmgr_sharemem_open(&gTsMgr, &gTsDataBuffer) < 0) {
		//NV_ERR("dataShmKey got faild in shmget()...%s(%d)", strerror(errno), errno);
		printf("dataShmKey got faild in shmget()...");
		return -1;
	}

	return 0;
}

static int isGetPat(unsigned char *ts, TS_SCAN_PAT_INFO *patInfo) {
	MPEG_TS_PACKET tsPkt;
	MPEG_TS_SI_HEAD siHead;
	MPEG_TS_PA_SECTION paSec;
	unsigned char *siData;

	tsPkt.syncByte = ts[0];
	tsPkt.errorFlag = (ts[1] >> 7) & 0x01;
	tsPkt.payloadFlag = (ts[1] >> 6) & 0x01;
	tsPkt.transFlag = (ts[1] >> 5) & 0x01;
	tsPkt.pid = ((ts[1] & 0x1F) << 8) + ts[2];
	tsPkt.scramble = (ts[3] >> 6) & 0x03;
	tsPkt.adapField = (ts[3] >> 4) & 0x03;
	tsPkt.cc = ts[3] & 0x0f;

	if (tsPkt.pid != TS_PAT_PID || (tsPkt.adapField & 0x01) != 1) {
		return -1;
	}

	if (tsPkt.adapField & 0x10) {
		//TODO : get Adap Len Jump to playload field
	}

	siData = ts + 4 + tsPkt.payloadFlag;
	//Parse SI_Header
	siHead.tableId = siData[0];
	siHead.syntaxFlag = (siData[1] >> 7) & 0x01;
	siHead.sectionLen = ((siData[1] & 0x0f) << 8) + siData[2];

	NV_DBG("siHead.tableId = %x\n", siHead.tableId);
	NV_DBG("siHead.syntaxFlag = %x\n", siHead.syntaxFlag);
	NV_DBG("siHead.sectionLen = %x\n", siHead.sectionLen);

	//TODO: check table id
	if (siHead.tableId == TS_TABLE_ID_PAS) {
		unsigned char *paData = siData + 3;
		//Parse PAT table
		paSec.tsId = (paData[0] << 8) + paData[1];
		paSec.versionNum = (paData[2] >> 1) & 0x01f;
		paSec.nextFlag = paData[3] & 0x01;
		paSec.sectionNum = paData[3];
		paSec.lastSectionNum = paData[4];
		paSec.crc32 = (paData[siHead.sectionLen - 4] << 24) + (paData[siHead.sectionLen - 3] << 16)
				+ (paData[siHead.sectionLen - 2] << 8) + paData[siHead.sectionLen - 1];
		NV_DBG("paSec.tsId = %x\n", paSec.tsId);
		NV_DBG("paSec.versionNum = %x\n", paSec.versionNum);
		NV_DBG("paSec.nextFlag = %x\n", paSec.nextFlag);
		NV_DBG("paSec.sectionNum = %x\n", paSec.sectionNum);
		NV_DBG("paSec.lastSectionNum = %x\n", paSec.lastSectionNum);
		NV_DBG("paSec.crc32 = %x\n", paSec.crc32);
		unsigned char *extData = paData + 5;
		int extDataLen = siHead.sectionLen - 5 - 4; //Data len - CRC32(4)
		while (extDataLen > 0) {
			unsigned int progNum = ((extData[0] << 8) + extData[1]);
			unsigned int pmid = ((extData[2] & 0x1F) << 8) + extData[3];
			NV_DBG("Prog num = %d\n", progNum);
			if (progNum == 0) {
				NV_DBG("Network ID = %x\n", pmid);
			} else {
				NV_DBG("program_map_PID = %x\n", pmid);
				patInfo->pmtProgs[patInfo->pmtNum] = progNum;
				patInfo->pmtPids[patInfo->pmtNum] = pmid;
				patInfo->pmtNum++;
			}
			extData += 4;
			extDataLen -= 4;
		}
	} else {
		return -1;
	}

	NV_DBG("patInfo->pmtNum = %d\n", patInfo->pmtNum);
	NV_DBG("patInfo->pmtPids[0] = %x\n", patInfo->pmtPids[0]);
	NV_DBG("patInfo->pmtPids[1] = %x\n", patInfo->pmtPids[1]);
	return 0;
}

static int isGetPmt(unsigned char *ts, unsigned int pmtPid, unsigned int progNum, TS_SCAN_PMT_INFO *pmtInfo) {
	MPEG_TS_PACKET tsPkt;
	MPEG_TS_SI_HEAD siHead;
	MPEG_TS_PM_SECTION pmSec;
	unsigned char *siData;

	tsPkt.syncByte = ts[0];
	tsPkt.errorFlag = (ts[1] >> 7) & 0x01;
	tsPkt.payloadFlag = (ts[1] >> 6) & 0x01;
	tsPkt.transFlag = (ts[1] >> 5) & 0x01;
	tsPkt.pid = ((ts[1] & 0x1F) << 8) + ts[2];
	tsPkt.scramble = (ts[3] >> 6) & 0x03;
	tsPkt.adapField = (ts[3] >> 4) & 0x03;
	tsPkt.cc = ts[3] & 0x0f;

	if (tsPkt.pid != pmtPid || (tsPkt.adapField & 0x01) != 1) {
		return -1;
	}

	if (tsPkt.adapField & 0x10) {
		//TODO : get Adap Len Jump to playload field
	}

	NV_DBG("tsPkt.syncByte = %x\n", tsPkt.syncByte);
	NV_DBG("tsPkt.errorFlag = %x\n", tsPkt.errorFlag);
	NV_DBG("tsPkt.payloadFlag = %x\n", tsPkt.payloadFlag);
	NV_DBG("tsPkt.transFlag = %x\n", tsPkt.transFlag);
	NV_DBG("tsPkt.pid = %x\n", tsPkt.pid);
	NV_DBG("tsPkt.scramble = %x\n", tsPkt.scramble);
	NV_DBG("tsPkt.adapField = %x\n", tsPkt.adapField);
	NV_DBG("tsPkt.cc = %x\n", tsPkt.cc);

	siData = ts + 4 + tsPkt.payloadFlag;
	//Parse SI_Header
	siHead.tableId = siData[0];
	siHead.syntaxFlag = (siData[1] >> 7) & 0x01;
	siHead.sectionLen = ((siData[1] & 0x0f) << 8) + siData[2];

	NV_DBG("siHead.tableId = %x\n", siHead.tableId);
	NV_DBG("siHead.syntaxFlag = %x\n", siHead.syntaxFlag);
	NV_DBG("siHead.sectionLen = %x\n", siHead.sectionLen);

	//TODO: check table id
	if (siHead.tableId == TS_TABLE_ID_PMS) {
		unsigned char *pmData = siData + 3;
		//Parse PMT table
		pmSec.progNum = (pmData[0] << 8) + pmData[1];
		pmSec.versionNum = (pmData[2] >> 1) & 0x01f;
		pmSec.nextFlag = pmData[3] & 0x01;
		pmSec.sectionNum = pmData[3];
		pmSec.lastSectionNum = pmData[4];
		pmSec.pcrPid = ((pmData[5] & 0x1F) << 8) + pmData[6];
		pmSec.progInfoLen = ((pmData[7] & 0x0F) << 8) + pmData[8];
		pmSec.crc32 = (pmData[siHead.sectionLen - 4] << 24) + (pmData[siHead.sectionLen - 3] << 16)
				+ (pmData[siHead.sectionLen - 2] << 8) + pmData[siHead.sectionLen - 1];
		NV_DBG("pmSec.progNum = %d\n", pmSec.progNum);
		NV_DBG("pmSec.versionNum = %x\n", pmSec.versionNum);
		NV_DBG("pmSec.nextFlag = %x\n", pmSec.nextFlag);
		NV_DBG("pmSec.sectionNum = %x\n", pmSec.sectionNum);
		NV_DBG("pmSec.lastSectionNum = %x\n", pmSec.lastSectionNum);
		NV_DBG("pmSec.pcrPid = %x\n", pmSec.pcrPid);
		NV_DBG("pmSec.progInfoLen = %x\n", pmSec.progInfoLen);
		NV_DBG("pmSec.crc32 = %x\n", pmSec.crc32);
		unsigned char *extData = pmData + 9 + pmSec.progInfoLen;
		int extDataLen = siHead.sectionLen - 9 - pmSec.progInfoLen - 4; //Data len - CRC32(4)
		NV_DBG("extDataLen = %d\n", extDataLen);

		if (pmSec.progNum != progNum) {
			return -1;
		}

		while (extDataLen > 0) {
			unsigned int streamType = extData[0];
			unsigned int ePID = ((extData[1] & 0x1F) << 8) + extData[2];
			unsigned int esLen = ((extData[3] & 0x0F) << 8) + extData[4];

			switch (streamType) {
			case STREAM_TYPE_VIDEO_MPEG1:
			case STREAM_TYPE_VIDEO_MPEG2:
			case STREAM_TYPE_VIDEO_MPEG4:
			case STREAM_TYPE_VIDEO_H264:
			case STREAM_TYPE_VIDEO_VC1:
			case STREAM_TYPE_VIDEO_DIRAC:
				pmtInfo->videoPids[pmtInfo->videoPidNum] = ePID;
				pmtInfo->videoPidNum++;
				break;
			case STREAM_TYPE_AUDIO_MPEG1:
			case STREAM_TYPE_AUDIO_MPEG2:
			case STREAM_TYPE_AUDIO_AAC:
			case STREAM_TYPE_AUDIO_AAC_LATM:
			case STREAM_TYPE_AUDIO_AC3:
			case STREAM_TYPE_AUDIO_DTS:
				pmtInfo->audioPids[pmtInfo->audioPidNum] = ePID;
				pmtInfo->audioPidNum++;
				break;
			default:
				NV_DBG("Unknow streamType = %x\n", streamType);
			}
			NV_DBG("streamType = %x\n", streamType);
			NV_DBG("ePID = %x\n", ePID);

			extData += (5 + esLen);
			extDataLen -= (5 + esLen);
		}

	} else {
		return -1;
	}

	return 0;
}

int DigChannelInfo(uint8_t * pData, uint32_t packetNum, int tune_freq, int band_width) {
	int i;
	uint8_t *data;
	NV_DBG("DigChannelInfo got %d packets in Freq: %d BW: %d.\n", packetNum, tune_freq, (band_width * 1000));
	for (i = 0; i <= packetNum; i++) {
		data = (pData + i * POCKET_LEN);
		if (IsGetPatFlag == 0) {
			IsGetPatFlag = isGetPat(data, &PadInfo) ? 0 : 1;
			if (IsGetPatFlag) {
				int pmtIdx = 0;
				ScanInfo.freq = tune_freq;
				ScanInfo.band = band_width * 1000;
				memcpy(&ScanInfo.patInfo, &PadInfo, sizeof(TS_SCAN_PAT_INFO));
				for (pmtIdx = 0; pmtIdx < PadInfo.pmtNum; pmtIdx++) {
					ScanInfo.pmts[pmtIdx].isGet = 0;
					ScanInfo.pmts[pmtIdx].progNum = PadInfo.pmtProgs[pmtIdx];
					ScanInfo.pmts[pmtIdx].pmtPid = PadInfo.pmtPids[pmtIdx];
				}
			}
		} else {
			int allScan = 1;
			int pmtIdx = 0;
			for (pmtIdx = 0; pmtIdx < PadInfo.pmtNum; pmtIdx++) {
				if (ScanInfo.pmts[pmtIdx].isGet == 0) {
					ScanInfo.pmts[pmtIdx].isGet =
							isGetPmt(data, ScanInfo.pmts[pmtIdx].pmtPid, ScanInfo.pmts[pmtIdx].progNum, &ScanInfo.pmts[pmtIdx]) ? 0 : 1;
				}
				if (ScanInfo.pmts[pmtIdx].isGet == 0) {
					allScan = 0;
				}
			}
			if (allScan == 1) {
				int scaned_num = ScanInfo.patInfo.pmtNum;
				NV_DBG("\t ScanInfo->pmtNum=%d \n", ScanInfo.patInfo.pmtNum);
				//Here reset all data struct for next record.
				libtsmgr_add_channels(gTsMgr, &ScanInfo);
				IsGetPatFlag = 0;
				memset(&PadInfo, 0, sizeof(TS_SCAN_PAT_INFO));
				memset(&ScanInfo, 0, sizeof(TS_SCAN_INFO));
				return scaned_num;
			}
		}
	}
	return 0;
}

uint32_t RecordTSData(uint8_t * pData, uint32_t packetNum, int tune_freq, int band_width) {
	int i;
	uint8_t *data;
	for (i = 0; i < packetNum; i++) {
		data = (pData + i * POCKET_LEN);
		TS_PACKET_DATA_IN tsDataIn;
		tsDataIn.freq = tune_freq;
		tsDataIn.band = band_width * 1000;
		tsDataIn.data = data;
		tsDataIn.len = POCKET_LEN;
		libtsmgr_write_ts(gTsMgr, gTsDataBuffer, &tsDataIn);
	}
	Passed10K = Passed10K + packetNum;
	if (Passed10K > 10000) {
		NV_DBG("Recorded %d packets.\n", Passed10K);
		Passed10K = 0;
	}
	return packetNum;
}
