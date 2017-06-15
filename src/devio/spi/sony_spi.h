/*------------------------------------------------------------------------------
  Copyright 2014 Sony Corporation

  Last Updated    : 2015/01/16
  Modification ID : d4b3fe129855d90c25271de02796915983f1e6ee
------------------------------------------------------------------------------*/
/**
 @file  sony_spi.h

 @brief The SPI I/O access interface.

        The user should implement SPI read/write functions and
        set function pointer of them to this struct's member.
*/

#ifndef SONY_SPI_H
#define SONY_SPI_H

#include "sony_common.h"

/*------------------------------------------------------------------------------
  Enums
------------------------------------------------------------------------------*/
/**
 @brief SPI mode definition.
*/
typedef enum {
    SONY_SPI_MODE_0,    /**< Mode 0 (CPOL, CPHA) = (0, 0) */
    SONY_SPI_MODE_1,    /**< Mode 1 (CPOL, CPHA) = (0, 1) */
    SONY_SPI_MODE_2,    /**< Mode 2 (CPOL, CPHA) = (1, 0) */
    SONY_SPI_MODE_3     /**< Mode 3 (CPOL, CPHA) = (1, 1) */
} sony_spi_mode_t;

/*------------------------------------------------------------------------------
  Structs
------------------------------------------------------------------------------*/
/**
 @brief The SPI driver API defintion.
*/
typedef struct sony_spi_t {
    /**
     @brief Read data via SPI.

     @param pSpi SPI driver instance.
     @param pData The buffer to store the read data.
     @param size The number of bytes to read from the SPI.

     @return SONY_RESULT_OK if successful.
    */
    sony_result_t (* Read) (struct sony_spi_t * pSpi, uint8_t * pData, uint32_t size);
    /**
     @brief Write data via SPI.

     @param pSpi SPI driver instance.
     @param pData The data to write to the device.
     @param size The number of bytes to write to the SPI.

     @return SONY_RESULT_OK if successful.
    */
    sony_result_t (* Write) (struct sony_spi_t * pSpi, const uint8_t * pData, uint32_t size);
    /**
     @brief Write and Read data via SPI. (Half duplex)

     @note  This function will be commonly used for CXD2880.

     @param pSpi SPI driver instance.
     @param pTxData The data to write to the device.
     @param txSize The number of bytes to write to the SPI.
     @param pRxData The buffer to store the read data.
     @param rxSize The number of bytes to read from the SPI.

     @return SONY_RESULT_OK if successful.
    */
    sony_result_t (* WriteRead) (struct sony_spi_t * pSpi, const uint8_t * pTxData, uint32_t txSize, uint8_t * pRxData, uint32_t rxSize);

    uint32_t         flags;     /**< Flags that can be used by SPI code */
    void*            user;      /**< User defined data. */
} sony_spi_t;

#endif /* SONY_SPI_H */
