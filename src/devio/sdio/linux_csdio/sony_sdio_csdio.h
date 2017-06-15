/*------------------------------------------------------------------------------
  Copyright 2014 Sony Corporation

  Last Updated    : 2015/01/16
  Modification ID : d4b3fe129855d90c25271de02796915983f1e6ee
------------------------------------------------------------------------------*/
/**
 @file  sony_sdio_csdio.h

 @brief The SDIO I/O access implemenation using "csdio" driver.

        Note that csdio.c should be modified.
        In original code, "count" argument of read and write function
        is the number of block.
        But in modified code, "count" is the data size in byte.
*/

#ifndef SONY_SDIO_CSDIO_H
#define SONY_SDIO_CSDIO_H

#include "sony_sdio.h"

/*------------------------------------------------------------------------------
  Structs
------------------------------------------------------------------------------*/
/**
 @brief Struct to store csdio related information.
*/
typedef struct sony_sdio_csdio_t {
    int fd_f0;    /**< file descriptor for function0 */
    int fd_f1;    /**< file descriptor for function1 */
} sony_sdio_csdio_t;

/*------------------------------------------------------------------------------
  Functions
------------------------------------------------------------------------------*/
/**
 @brief Initialize SDIO access via "csdio".

 @param pSdioCsdio       sony_sdio_csdio_t struct instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_sdio_csdio_Initialize (sony_sdio_csdio_t * pSdioCsdio);

/**
 @brief Finalize SDIO access via "csdio".

 @param pSdioCsdio       sony_sdio_csdio_t struct instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_sdio_csdio_Finalize (sony_sdio_csdio_t * pSdioCsdio);

/**
 @brief Set up the SDIO access struct using "csdio".

 @param pSdio            SDIO struct instance.
 @param pSdioCsdio       sony_sdio_csdio_t struct instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_sdio_csdio_CreateSdio (sony_sdio_t * pSdio, sony_sdio_csdio_t * pSdioCsdio);

#endif /* SONY_SDIO_CSDIO_H */
