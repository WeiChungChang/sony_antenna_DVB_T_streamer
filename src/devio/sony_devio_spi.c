/*------------------------------------------------------------------------------
  Copyright 2014 Sony Corporation

  Last Updated    : 2015/01/16
  Modification ID : d4b3fe129855d90c25271de02796915983f1e6ee
------------------------------------------------------------------------------*/

#include "sony_devio_spi.h"

#include "sony_stdlib.h" /* for memcpy */

#define BURST_WRITE_MAX 128 /* Max length of burst write */

/*------------------------------------------------------------------------------
 Static Functions
------------------------------------------------------------------------------*/
static sony_result_t sony_regio_spi_ReadRegister (sony_regio_t * pRegio, sony_regio_target_t target,
    uint8_t subAddress, uint8_t * pData, uint32_t size)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_spi_t* pSpi = NULL;
    uint8_t sendData[6];
    uint8_t * pReadDataTop = pData;

    SONY_TRACE_IO_ENTER ("sony_regio_spi_ReadRegister");

    if ((!pRegio) || (!pRegio->pIfObject) || (!pData)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (subAddress + size > 0x100) {
        /* This subAddress, size argument will be incorrect. */
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_RANGE);
    }

    pSpi = (sony_spi_t*)(pRegio->pIfObject);

    if (target == SONY_REGIO_TARGET_SYSTEM) {
        sendData[0] = 0x0B; /* SYSTEM */
    } else {
        sendData[0] = 0x0A; /* DEMOD */
    }

    sendData[3] = sendData[4] = sendData[5] = 0; /* Dummy data */

    while (size > 0) {
        sendData[1] = subAddress;
        if (size > 255) {
            sendData[2] = 255;
        } else {
            sendData[2] = (uint8_t)size;
        }

        result = pSpi->WriteRead (pSpi, sendData, sizeof (sendData), pReadDataTop, sendData[2]);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_IO_RETURN (result);
        }

        subAddress += sendData[2];
        pReadDataTop += sendData[2];
        size -= sendData[2];
    }

    SONY_TRACE_IO_RETURN (result);
}

static sony_result_t sony_regio_spi_WriteRegister (sony_regio_t * pRegio, sony_regio_target_t target,
    uint8_t subAddress, const uint8_t * pData, uint32_t size)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_spi_t* pSpi = NULL;
    uint8_t sendData[BURST_WRITE_MAX + 4]; /* Command, Address, Length and Dummy Data */
    const uint8_t * pWriteDataTop = pData;

    SONY_TRACE_IO_ENTER ("sony_regio_spi_WriteRegister");

    if ((!pRegio) || (!pRegio->pIfObject) || (!pData)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if(size > BURST_WRITE_MAX){
        /* Buffer is too small... */
        SONY_TRACE_IO_RETURN(SONY_RESULT_ERROR_OVERFLOW);
    }

    if (subAddress + size > 0x100) {
        /* This subAddress, size argument will be incorrect. */
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_RANGE);
    }

    pSpi = (sony_spi_t*)(pRegio->pIfObject);

    if (target == SONY_REGIO_TARGET_SYSTEM) {
        sendData[0] = 0x0F; /* SYSTEM */
    } else {
        sendData[0] = 0x0E; /* DEMOD */
    }

    while (size > 0) {
        sendData[1] = subAddress;
        if (size > 255) {
            sendData[2] = 255;
        } else {
            sendData[2] = (uint8_t)size;
        }

        sony_memcpy (&sendData[3], pWriteDataTop, sendData[2]);

        if (target == SONY_REGIO_TARGET_SYSTEM) {
            sendData[3 + sendData[2]] = 0x00; /* Set 0 for dummy data */
            result = pSpi->Write (pSpi, sendData, sendData[2] + 4);
        } else {
            result = pSpi->Write (pSpi, sendData, sendData[2] + 3);
        }
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_IO_RETURN (result);
        }

        subAddress += sendData[2];
        pWriteDataTop += sendData[2];
        size -= sendData[2];
    }

    SONY_TRACE_IO_RETURN (result);
}

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
sony_result_t sony_regio_spi_Create (sony_regio_t * pRegio, sony_spi_t * pSpi, uint8_t slaveSelect)
{
    SONY_TRACE_IO_ENTER ("sony_regio_spi_Create");

    if ((!pRegio) || (!pSpi)) {
        SONY_TRACE_IO_RETURN(SONY_RESULT_ERROR_ARG);
    }

    pRegio->ReadRegister = sony_regio_spi_ReadRegister;
    pRegio->WriteRegister = sony_regio_spi_WriteRegister;
    pRegio->WriteOneRegister = sony_regio_CommonWriteOneRegister;
    pRegio->pIfObject = pSpi;
    pRegio->i2cAddressSystem = 0;
    pRegio->i2cAddressDemod = 0;
    pRegio->slaveSelect = slaveSelect;

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

sony_result_t sony_devio_spi_ReadTS (sony_spi_t * pSpi, uint8_t * pData, uint32_t packetNum)
{
    sony_result_t result = SONY_RESULT_OK;
    uint8_t sendData[3];

    SONY_TRACE_IO_ENTER ("sony_devio_spi_ReadTS");

    if ((!pSpi) || (!pData) || (packetNum == 0)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (packetNum > 0xFFFF) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_RANGE);
    }

    sendData[0] = 0x10;
    sendData[1] = (uint8_t)((packetNum >> 8) & 0xFF);
    sendData[2] = (uint8_t)(packetNum & 0xFF);

    result = pSpi->WriteRead (pSpi, sendData, sizeof (sendData), pData, packetNum * 188);

    SONY_TRACE_IO_RETURN (result);
}

sony_result_t sony_devio_spi_ReadTSBufferInfo (sony_spi_t * pSpi, sony_tunerdemod_ts_buffer_info_t * pInfo)
{
    sony_result_t result = SONY_RESULT_OK;
    uint8_t sendData = 0x20;
    uint8_t recvData[2];

    SONY_TRACE_IO_ENTER ("sony_devio_spi_ReadTSBufferInfo");

    if ((!pSpi) || (!pInfo)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    result = pSpi->WriteRead (pSpi, &sendData, 1, recvData, sizeof (recvData));
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_IO_RETURN (result);
    }

    pInfo->readReady = (uint8_t)((recvData[0] & 0x80) ? 1 : 0);
    pInfo->almostFull = (uint8_t)((recvData[0] & 0x40) ? 1 : 0);
    pInfo->almostEmpty = (uint8_t)((recvData[0] & 0x20) ? 1 : 0);
    pInfo->overflow = (uint8_t)((recvData[0] & 0x10) ? 1 : 0);
    pInfo->underflow = (uint8_t)((recvData[0] & 0x08) ? 1 : 0);
    pInfo->packetNum = (uint16_t)(((recvData[0] & 0x07) << 8) | recvData[1]);

    SONY_TRACE_IO_RETURN (result);
}

sony_result_t sony_devio_spi_ReadTS_TSBufferInfo (sony_spi_t * pSpi, uint8_t * pData, uint32_t packetNum, sony_tunerdemod_ts_buffer_info_t * pInfo)
{
    sony_result_t result = SONY_RESULT_OK;
    uint8_t sendData[3];

    SONY_TRACE_IO_ENTER ("sony_devio_spi_ReadTS_TSBufferInfo");

    if ((!pSpi) || (!pData) || (packetNum == 0) || (!pInfo)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (packetNum > 0xFFFF + 2) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_RANGE);
    }

    sendData[0] = 0x30;
    sendData[1] = (uint8_t)((packetNum >> 8) & 0xFF);
    sendData[2] = (uint8_t)(packetNum & 0xFF);

    result = pSpi->WriteRead (pSpi, sendData, sizeof (sendData), pData, (packetNum * 188) + 2);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_IO_RETURN (result);
    }

    pInfo->readReady = (uint8_t)((pData[0] & 0x80) ? 1 : 0);
    pInfo->almostFull = (uint8_t)((pData[0] & 0x40) ? 1 : 0);
    pInfo->almostEmpty = (uint8_t)((pData[0] & 0x20) ? 1 : 0);
    pInfo->overflow = (uint8_t)((pData[0] & 0x10) ? 1 : 0);
    pInfo->underflow = (uint8_t)((pData[0] & 0x08) ? 1 : 0);
    pInfo->packetNum = (uint16_t)(((pData[0] & 0x07) << 8) | pData[1]);

    SONY_TRACE_IO_RETURN (result);
}

sony_result_t sony_devio_spi_ClearTSBuffer (sony_spi_t * pSpi)
{
    sony_result_t result = SONY_RESULT_OK;
    uint8_t sendData = 0x03;

    SONY_TRACE_IO_ENTER ("sony_devio_spi_ClearTSBuffer");

    if (!pSpi) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    result = pSpi->Write (pSpi, &sendData, 1);

    SONY_TRACE_IO_RETURN (result);
}
