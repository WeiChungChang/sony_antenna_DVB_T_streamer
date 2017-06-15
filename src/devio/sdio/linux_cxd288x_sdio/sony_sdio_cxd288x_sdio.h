/*------------------------------------------------------------------------------
  Copyright 2015 Sony Corporation

  Last Updated    : 2015/06/15
  Modification ID : b1bddd1bc481066dc507f82232d19cb93ea91576
------------------------------------------------------------------------------*/
/**
 @file  sony_sdio_cxd288x_sdio.h

 @brief The SDIO I/O access implemenation using "cxd288x_sdio" kernel driver.
*/

#ifndef SONY_SDIO_CXD288X_SDIO_H
#define SONY_SDIO_CXD288X_SDIO_H

#include "sony_sdio.h"

/*------------------------------------------------------------------------------
  Structs
------------------------------------------------------------------------------*/
/**
 @brief Struct to store csdio related information.
*/
typedef struct sony_sdio_cxd288x_sdio_t {
    int fd;       /**< file descriptor */
} sony_sdio_cxd288x_sdio_t;

/*------------------------------------------------------------------------------
  Functions
------------------------------------------------------------------------------*/
/**
 @brief Initialize SDIO access via "cxd288x_sdio" driver.

 @param pCxd288xSdio     sony_sdio_cxd288x_sdio_t struct instance.
 @param busNum           SDIO bus index that the device connected.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_sdio_cxd288x_sdio_Initialize (sony_sdio_cxd288x_sdio_t * pCxd288xSdio, uint8_t busNum);

/**
 @brief Finalize SDIO access via "cxd288x_sdio" driver.

 @param pCxd288xSdio     sony_sdio_cxd288x_sdio_t struct instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_sdio_cxd288x_sdio_Finalize (sony_sdio_cxd288x_sdio_t * pCxd288xSdio);

/**
 @brief Set up the SDIO access struct using "cxd288x_sdio" driver.

 @param pSdio            SDIO struct instance.
 @param pCxd288xSdio     sony_sdio_cxd288x_sdio_t struct instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_sdio_cxd288x_sdio_CreateSdio (sony_sdio_t * pSdio, sony_sdio_cxd288x_sdio_t * pCxd288xSdio);

#endif /* SONY_SDIO_CXD288X_SDIO_H */
