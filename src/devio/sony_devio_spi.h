/*------------------------------------------------------------------------------
  Copyright 2014 Sony Corporation

  Last Updated    : 2015/01/16
  Modification ID : d4b3fe129855d90c25271de02796915983f1e6ee
------------------------------------------------------------------------------*/
/**
 @file  sony_devio_spi.h

 @brief The I/O interface via SPI.
*/
/*----------------------------------------------------------------------------*/

#ifndef SONY_DEVIO_SPI_H
#define SONY_DEVIO_SPI_H

#include "sony_common.h"
#include "sony_regio.h"
#include "sony_spi.h"

#include "sony_tunerdemod.h"

/*------------------------------------------------------------------------------
  APIs
------------------------------------------------------------------------------*/
/**
 @brief Set up the Register I/O struct instance for SPI.

 @param pRegio           Register I/O struct instance.
 @param pSpi             The SPI APIs that the driver will use.
 @param slaveSelect      *Optional* ID value that can be used to distinguish the device.
                         May be used in SPI I/O implementation.
                         This ID is output as log using sony_regio_log.c/h.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_regio_spi_Create (sony_regio_t * pRegio, sony_spi_t * pSpi, uint8_t slaveSelect);

/**
 @brief Read TS from SPI.

 @param pSpi             The SPI APIs that the driver will use.
 @param pData            TS data buffer read from SPI. Note that the buffer size should be (packetNum * 188).
 @param packetNum        The number of packets to read from the SPI.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_devio_spi_ReadTS (sony_spi_t * pSpi, uint8_t * pData, uint32_t packetNum);

/**
 @brief Read TS buffer status from SPI.

 @param pSpi             The SPI APIs that the driver will use.
 @param pInfo            TS buffer information struct instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_devio_spi_ReadTSBufferInfo (sony_spi_t * pSpi, sony_tunerdemod_ts_buffer_info_t * pInfo);

/**
 @brief Read TS and TS buffer status at the same time from SPI.

 @note  This API uses Read Status/TS command.
        The device sends status information (2bytes) BEFORE TS data.
        To avoid useless buffer copy, this API will read (size - 2) TS data.
        So, the user should pass (necessary buffer size + 2) size of buffer,
        and ignore first 2 bytes in the buffer.

 @param pSpi             The SPI APIs that the driver will use.
 @param pData            Status and TS data buffer read from SPI.
                         Note that the buffer size should be (packetNum * 188) + 2.
                         The FIRST 2 bytes are NOT TS data but TS status data. Should ignore it.
 @param packetNum        The number of packets to read from the SPI.
 @param pInfo            TS buffer information struct instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_devio_spi_ReadTS_TSBufferInfo (sony_spi_t * pSpi, uint8_t * pData, uint32_t packetNum, sony_tunerdemod_ts_buffer_info_t * pInfo);

/**
 @brief Clear TS buffer from SPI.

 @param pSpi             The SPI APIs that the driver will use.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_devio_spi_ClearTSBuffer (sony_spi_t * pSpi);

#endif /* SONY_DEVIO_SPI_H */
