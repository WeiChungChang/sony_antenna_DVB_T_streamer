/*------------------------------------------------------------------------------
  Copyright 2014 Sony Corporation

  Last Updated    : 2015/01/16
  Modification ID : d4b3fe129855d90c25271de02796915983f1e6ee
------------------------------------------------------------------------------*/
/**
 @file  sony_spi_spidev.h

 @brief The SPI I/O access implemenation using "spidev" driver.
*/

#ifndef SONY_SPI_SPIDEV_H
#define SONY_SPI_SPIDEV_H

#include "sony_spi.h"

/*------------------------------------------------------------------------------
  Structs
------------------------------------------------------------------------------*/
/**
 @brief Struct to store spidev related information.
*/
typedef struct sony_spi_spidev_t {
    int fd;       /**< file descriptor */
} sony_spi_spidev_t;

/*------------------------------------------------------------------------------
  Functions
------------------------------------------------------------------------------*/
/**
 @brief Initialize SPI access via "spidev".

 @param pSpiSpidev       sony_spi_spidev_t struct instance.
 @param busNum           Bus number.
 @param chipSel          Chip select (slave select).
 @param mode             SPI mode
 @param speedHz          Clock frequency in Hz.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_spi_spidev_Initialize (sony_spi_spidev_t * pSpiSpidev, uint8_t busNum, uint8_t chipSel,
                                          sony_spi_mode_t mode, uint32_t speedHz);

/**
 @brief Finalize SPI access via "spidev".

 @param pSpiSpidev       sony_spi_spidev_t struct instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_spi_spidev_Finalize (sony_spi_spidev_t * pSpiSpidev);

/**
 @brief Set up the SPI access struct using "spidev".

 @param pSpi             SPI struct instance.
 @param pSpiSpidev       sony_spi_spidev_t struct instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_spi_spidev_CreateSpi (sony_spi_t * pSpi, sony_spi_spidev_t * pSpiSpidev);

#endif /* SONY_SPI_SPIDEV_H */
