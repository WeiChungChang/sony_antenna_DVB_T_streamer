/*------------------------------------------------------------------------------
  Copyright 2014 Sony Corporation

  Last Updated    : 2015/01/16
  Modification ID : d4b3fe129855d90c25271de02796915983f1e6ee
------------------------------------------------------------------------------*/
/**
 @file  sony_devio_sdio.h

 @brief The I/O interface via SDIO.
*/
/*----------------------------------------------------------------------------*/

#ifndef SONY_DEVIO_SDIO_H
#define SONY_DEVIO_SDIO_H

#include "sony_common.h"
#include "sony_regio.h"
#include "sony_sdio.h"

/*------------------------------------------------------------------------------
  APIs
------------------------------------------------------------------------------*/
/**
 @brief Set up the Register I/O struct instance for SDIO.

 @param pRegio           Register I/O struct instance.
 @param pSdio            The SDIO APIs that the driver will use.
 @param slaveSelect      *Optional* ID value that can be used to distinguish the device.
                         May be used in SDIO I/O implementation.
                         This ID is output as log using sony_regio_log.c/h.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_regio_sdio_Create (sony_regio_t * pRegio, sony_sdio_t * pSdio, uint8_t slaveSelect);

/**
 @brief Read TS from SDIO.

 @note  This function is implemented to show how to read TS using SDIO CMD53.
        The user need not use this function.
        Instead of this function, the user can use driver API in user's system directly.

 @param pSdio            The SDIO APIs that the driver will use.
 @param pData            TS data buffer read from SDIO.
 @param size             The number of bytes to read from the SDIO.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_devio_sdio_ReadTS (sony_sdio_t * pSdio, uint8_t * pData, uint32_t size);

#endif /* SONY_DEVIO_SDIO_H */
