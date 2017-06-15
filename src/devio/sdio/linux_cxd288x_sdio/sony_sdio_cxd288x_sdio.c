/*------------------------------------------------------------------------------
  Copyright 2015 Sony Corporation

  Last Updated    : 2015/01/28
  Modification ID : a92e075f570b4d88dd18061b530b0cc2ff88abac
------------------------------------------------------------------------------*/
#include "sony_sdio_cxd288x_sdio.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <stdio.h>

#include "cxd288x_sdio.h"

/*------------------------------------------------------------------------------
 Static Functions
------------------------------------------------------------------------------*/
static sony_result_t sony_sdio_cxd288x_sdio_ReadCMD52 (sony_sdio_t * pSdio, uint32_t registerAddress, uint8_t * pData, uint8_t function)
{
    sony_sdio_cxd288x_sdio_t* pCxd288xSdio = NULL;
    struct cxd288x_sdio_cmd52_ctrl_t cmdCtrl;

    SONY_TRACE_IO_ENTER ("sony_sdio_cxd288x_sdio_ReadCMD52");

    if ((!pSdio) || (!pSdio->user)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    pCxd288xSdio = (sony_sdio_cxd288x_sdio_t*)(pSdio->user);

    if (pCxd288xSdio->fd < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    cmdCtrl.m_write = 0;
    cmdCtrl.m_address = registerAddress;
    cmdCtrl.m_data = 0;

    if (ioctl (pCxd288xSdio->fd, CXD288X_SDIO_IOC_CMD52, &cmdCtrl) < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_IO);
    }

    *pData = cmdCtrl.m_data;

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

static sony_result_t sony_sdio_cxd288x_sdio_WriteCMD52 (sony_sdio_t * pSdio, uint32_t registerAddress, uint8_t data, uint8_t function)
{
    sony_sdio_cxd288x_sdio_t* pCxd288xSdio = NULL;
    struct cxd288x_sdio_cmd52_ctrl_t cmdCtrl;

    SONY_TRACE_IO_ENTER ("sony_sdio_cxd288x_sdio_WriteCMD52");

    if ((!pSdio) || (!pSdio->user)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    pCxd288xSdio = (sony_sdio_cxd288x_sdio_t*)(pSdio->user);

    if (pCxd288xSdio->fd < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    cmdCtrl.m_write = 1;
    cmdCtrl.m_address = registerAddress;
    cmdCtrl.m_data = data;

    if (ioctl (pCxd288xSdio->fd, CXD288X_SDIO_IOC_CMD52, &cmdCtrl) < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_IO);
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

static sony_result_t sony_sdio_cxd288x_sdio_ReadCMD53 (sony_sdio_t * pSdio, uint32_t registerAddress, uint8_t * pData, uint32_t size,
    uint8_t function, sony_sdio_op_code_t opCode)
{
    sony_sdio_cxd288x_sdio_t* pCxd288xSdio = NULL;
    ssize_t retSize = 0;

    SONY_TRACE_IO_ENTER ("sony_sdio_cxd288x_sdio_ReadCMD53");

    if ((!pSdio) || (!pSdio->user) || (!pData) || (size == 0)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    pCxd288xSdio = (sony_sdio_cxd288x_sdio_t*)(pSdio->user);

    if (pCxd288xSdio->fd < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    switch (opCode) {
    case SONY_SDIO_OP_CODE_FIXED:
        registerAddress += CXD288X_SDIO_FPOS_OP_FIXED;
        break;
    case SONY_SDIO_OP_CODE_INCREMENT:
        registerAddress += CXD288X_SDIO_FPOS_OP_INCREMENT;
        break;
    default:
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    retSize = pread (pCxd288xSdio->fd, pData, (size_t) size, (off_t) registerAddress);
    if (retSize != size) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_IO);
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

static sony_result_t sony_sdio_cxd288x_sdio_WriteCMD53 (sony_sdio_t * pSdio, uint32_t registerAddress, const uint8_t* pData, uint32_t size,
    uint8_t function, sony_sdio_op_code_t opCode)
{
    sony_sdio_cxd288x_sdio_t* pCxd288xSdio = NULL;
    ssize_t retSize = 0;

    SONY_TRACE_IO_ENTER ("sony_sdio_cxd288x_sdio_WriteCMD53");

    if ((!pSdio) || (!pSdio->user) || (!pData) || (size == 0)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    pCxd288xSdio = (sony_sdio_cxd288x_sdio_t*)(pSdio->user);

    if (pCxd288xSdio->fd < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    switch (opCode) {
    case SONY_SDIO_OP_CODE_FIXED:
        registerAddress += CXD288X_SDIO_FPOS_OP_FIXED;
        break;
    case SONY_SDIO_OP_CODE_INCREMENT:
        registerAddress += CXD288X_SDIO_FPOS_OP_INCREMENT;
        break;
    default:
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    retSize = pwrite (pCxd288xSdio->fd, pData, (size_t) size, (off_t) registerAddress);
    if (retSize != size) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_IO);
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
sony_result_t sony_sdio_cxd288x_sdio_Initialize (sony_sdio_cxd288x_sdio_t * pCxd288xSdio, uint8_t busNum)
{
    char devName[64];
    
    SONY_TRACE_IO_ENTER ("sony_sdio_cxd288x_sdio_Initialize");
    
    if (!pCxd288xSdio) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }
    snprintf (devName, sizeof (devName), "/dev/sony_ew100_sdio%d", busNum);
    printf("sony_sdio_cxd288x_sdio_Initialize start, devName=%s \n",devName);
    /* Open cxd288x_sdio driver. */
    pCxd288xSdio->fd = open (devName, O_RDWR);
    if (pCxd288xSdio->fd < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

sony_result_t sony_sdio_cxd288x_sdio_Finalize (sony_sdio_cxd288x_sdio_t * pCxd288xSdio)
{
    SONY_TRACE_IO_ENTER ("sony_sdio_cxd288x_sdio_Finalize");

    if (!pCxd288xSdio) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pCxd288xSdio->fd >= 0) {
        close (pCxd288xSdio->fd);
        pCxd288xSdio->fd = -1;
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

sony_result_t sony_sdio_cxd288x_sdio_CreateSdio (sony_sdio_t * pSdio, sony_sdio_cxd288x_sdio_t * pCxd288xSdio)
{
    SONY_TRACE_IO_ENTER ("sony_sdio_cxd288x_sdio_Create");

    if ((!pSdio) || (!pCxd288xSdio)) {
        SONY_TRACE_IO_RETURN(SONY_RESULT_ERROR_ARG);
    }

    pSdio->ReadCMD52 = sony_sdio_cxd288x_sdio_ReadCMD52;
    pSdio->WriteCMD52 = sony_sdio_cxd288x_sdio_WriteCMD52;
    pSdio->ReadCMD53 = sony_sdio_cxd288x_sdio_ReadCMD53;
    pSdio->WriteCMD53 = sony_sdio_cxd288x_sdio_WriteCMD53;
    pSdio->flags = 0;
    pSdio->user = pCxd288xSdio;

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}
