/*------------------------------------------------------------------------------
  Copyright 2014 Sony Corporation

  Last Updated    : 2015/04/02
  Modification ID : b7d3fbfff615b33d0612092777b65e338801de65
------------------------------------------------------------------------------*/
#include "sony_spi_spidev.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/spi/spidev.h>

#include <stdio.h>
#include <stdint.h>

/*------------------------------------------------------------------------------
 Static Functions
------------------------------------------------------------------------------*/
static sony_result_t sony_spi_spidev_Read (sony_spi_t * pSpi, uint8_t * pData, uint32_t size)
{
    sony_spi_spidev_t* pSpiSpidev = NULL;
    ssize_t retSize = 0;

    SONY_TRACE_IO_ENTER ("sony_spi_spidev_Read");

    if ((!pSpi) || (!pSpi->user) || (!pData) || (size == 0)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    pSpiSpidev = (sony_spi_spidev_t*)(pSpi->user);

    if (pSpiSpidev->fd < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    retSize = read (pSpiSpidev->fd, pData, (size_t) size);
    if (retSize != size) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_IO);
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

static sony_result_t sony_spi_spidev_Write (sony_spi_t * pSpi, const uint8_t * pData, uint32_t size)
{
    sony_spi_spidev_t* pSpiSpidev = NULL;
    ssize_t retSize = 0;

    SONY_TRACE_IO_ENTER ("sony_spi_spidev_Write");

    if ((!pSpi) || (!pSpi->user) || (!pData) || (size == 0)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    pSpiSpidev = (sony_spi_spidev_t*)(pSpi->user);

    if (pSpiSpidev->fd < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    retSize = write (pSpiSpidev->fd, pData, (size_t) size);
    if (retSize != size) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_IO);
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

static sony_result_t sony_spi_spidev_WriteRead (struct sony_spi_t * pSpi, const uint8_t * pTxData, uint32_t txSize, uint8_t * pRxData, uint32_t rxSize)
{
    sony_spi_spidev_t* pSpiSpidev = NULL;
    struct spi_ioc_transfer msg[2] = { {0} };

    SONY_TRACE_IO_ENTER ("sony_spi_spidev_WriteRead");

    if ((!pSpi) || (!pSpi->user) || (!pTxData) || (txSize == 0) || (!pRxData) || (rxSize == 0)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    pSpiSpidev = (sony_spi_spidev_t*)(pSpi->user);

    if (pSpiSpidev->fd < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Write data */
    msg[0].tx_buf = (uintptr_t)pTxData;
    msg[0].rx_buf = (uintptr_t)NULL;
    msg[0].len = txSize;
    msg[0].delay_usecs = 0;
    msg[0].speed_hz = 0;
    msg[0].bits_per_word = 0;
    msg[0].cs_change = 0;

    /* Read data */
    msg[1].tx_buf = (uintptr_t)NULL;
    msg[1].rx_buf = (uintptr_t)pRxData;
    msg[1].len = rxSize;
    msg[1].delay_usecs = 0;
    msg[1].speed_hz = 0;
    if(pTxData[0] == 0x10) //Read TS
        msg[1].bits_per_word = 32;
    else
        msg[1].bits_per_word = 0;
    msg[1].cs_change = 1;

    if (ioctl (pSpiSpidev->fd, SPI_IOC_MESSAGE(2), msg) < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_IO);
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
sony_result_t sony_spi_spidev_Initialize (sony_spi_spidev_t * pSpiSpidev, uint8_t busNum, uint8_t chipSel,
                                          sony_spi_mode_t mode, uint32_t speedHz)
{
    char devName[64];
    uint8_t modeValue = 0;

    SONY_TRACE_IO_ENTER ("sony_spi_spidev_Initialize");

    if (!pSpiSpidev) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    snprintf (devName, sizeof (devName), "/dev/spidev%d.%d", busNum, chipSel);

    /* Open spidev driver. */
    pSpiSpidev->fd = open (devName, O_RDWR);
    if (pSpiSpidev->fd < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    switch (mode) {
    case SONY_SPI_MODE_0:
        modeValue = SPI_MODE_0;
        break;
    case SONY_SPI_MODE_1:
        modeValue = SPI_MODE_1;
        break;
    case SONY_SPI_MODE_2:
        modeValue = SPI_MODE_2;
        break;
    case SONY_SPI_MODE_3:
        modeValue = SPI_MODE_3;
        break;
    default:
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (ioctl (pSpiSpidev->fd, SPI_IOC_WR_MODE, &modeValue) < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    if (ioctl (pSpiSpidev->fd, SPI_IOC_WR_MAX_SPEED_HZ, &speedHz) < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

sony_result_t sony_spi_spidev_Finalize (sony_spi_spidev_t * pSpiSpidev)
{
    SONY_TRACE_IO_ENTER ("sony_spi_spidev_Finalize");

    if (!pSpiSpidev) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pSpiSpidev->fd >= 0) {
        close (pSpiSpidev->fd);
        pSpiSpidev->fd = -1;
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

sony_result_t sony_spi_spidev_CreateSpi (sony_spi_t * pSpi, sony_spi_spidev_t * pSpiSpidev)
{
    SONY_TRACE_IO_ENTER ("sony_spi_spidev_Create");

    if ((!pSpi) || (!pSpiSpidev)) {
        SONY_TRACE_IO_RETURN(SONY_RESULT_ERROR_ARG);
    }

    pSpi->Read = sony_spi_spidev_Read;
    pSpi->Write = sony_spi_spidev_Write;
    pSpi->WriteRead = sony_spi_spidev_WriteRead;
    pSpi->flags = 0;
    pSpi->user = pSpiSpidev;

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}
