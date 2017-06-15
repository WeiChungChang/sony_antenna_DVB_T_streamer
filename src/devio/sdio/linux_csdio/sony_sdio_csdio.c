/*------------------------------------------------------------------------------
  Copyright 2014 Sony Corporation

  Last Updated    : 2015/01/16
  Modification ID : d4b3fe129855d90c25271de02796915983f1e6ee
------------------------------------------------------------------------------*/
#include "sony_sdio_csdio.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/csdio.h>

/*------------------------------------------------------------------------------
 Static Functions
------------------------------------------------------------------------------*/
static sony_result_t sony_sdio_csdio_ReadCMD52 (sony_sdio_t * pSdio, uint32_t registerAddress, uint8_t * pData, uint8_t function)
{
    sony_sdio_csdio_t* pSdioCsdio = NULL;
    struct csdio_cmd52_ctrl_t cmdCtrl;

    SONY_TRACE_IO_ENTER ("sony_sdio_csdio_ReadCMD52");

    if ((!pSdio) || (!pSdio->user)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    pSdioCsdio = (sony_sdio_csdio_t*)(pSdio->user);

    if (pSdioCsdio->fd_f1 < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    cmdCtrl.m_write = 0;
    cmdCtrl.m_address = registerAddress;
    cmdCtrl.m_data = 0;
    cmdCtrl.m_ret = 0;

    if (ioctl (pSdioCsdio->fd_f1, CSDIO_IOC_CMD52, &cmdCtrl) < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    if (cmdCtrl.m_ret != 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_IO);
    }

    *pData = cmdCtrl.m_data;

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

static sony_result_t sony_sdio_csdio_WriteCMD52 (sony_sdio_t * pSdio, uint32_t registerAddress, uint8_t data, uint8_t function)
{
    sony_sdio_csdio_t* pSdioCsdio = NULL;
    struct csdio_cmd52_ctrl_t cmdCtrl;

    SONY_TRACE_IO_ENTER ("sony_sdio_csdio_WriteCMD52");

    if ((!pSdio) || (!pSdio->user)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    pSdioCsdio = (sony_sdio_csdio_t*)(pSdio->user);

    if (pSdioCsdio->fd_f1 < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    cmdCtrl.m_write = 1;
    cmdCtrl.m_address = registerAddress;
    cmdCtrl.m_data = data;
    cmdCtrl.m_ret = 0;

    if (ioctl (pSdioCsdio->fd_f1, CSDIO_IOC_CMD52, &cmdCtrl) < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    if (cmdCtrl.m_ret != 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_IO);
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

static sony_result_t sony_sdio_csdio_ReadCMD53 (sony_sdio_t * pSdio, uint32_t registerAddress, uint8_t * pData, uint32_t size,
    uint8_t function, sony_sdio_op_code_t opCode)
{
    sony_sdio_csdio_t* pSdioCsdio = NULL;
    struct csdio_cmd53_ctrl_t cmdCtrl;
    ssize_t retSize = 0;

    SONY_TRACE_IO_ENTER ("sony_sdio_csdio_ReadCMD53");

    if ((!pSdio) || (!pSdio->user) || (!pData) || (size == 0)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    pSdioCsdio = (sony_sdio_csdio_t*)(pSdio->user);

    if (pSdioCsdio->fd_f1 < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    cmdCtrl.m_block_mode = 1;
    cmdCtrl.m_op_code = (uint32_t)opCode;
    cmdCtrl.m_address = registerAddress;

    if (ioctl (pSdioCsdio->fd_f1, CSDIO_IOC_CMD53, &cmdCtrl) < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    retSize = read (pSdioCsdio->fd_f1, pData, (size_t) size);
    if (retSize != size) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_IO);
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

static sony_result_t sony_sdio_csdio_WriteCMD53 (sony_sdio_t * pSdio, uint32_t registerAddress, const uint8_t* pData, uint32_t size,
    uint8_t function, sony_sdio_op_code_t opCode)
{
    sony_sdio_csdio_t* pSdioCsdio = NULL;
    struct csdio_cmd53_ctrl_t cmdCtrl;
    ssize_t retSize = 0;

    SONY_TRACE_IO_ENTER ("sony_sdio_csdio_WriteCMD53");

    if ((!pSdio) || (!pSdio->user) || (!pData) || (size == 0)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    pSdioCsdio = (sony_sdio_csdio_t*)(pSdio->user);

    if (pSdioCsdio->fd_f1 < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    cmdCtrl.m_block_mode = 1;
    cmdCtrl.m_op_code = (uint32_t)opCode;
    cmdCtrl.m_address = registerAddress;

    if (ioctl (pSdioCsdio->fd_f1, CSDIO_IOC_CMD53, &cmdCtrl) < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    retSize = write (pSdioCsdio->fd_f1, pData, (size_t) size);
    if (retSize != size) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_IO);
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
sony_result_t sony_sdio_csdio_Initialize (sony_sdio_csdio_t * pSdioCsdio)
{
    SONY_TRACE_IO_ENTER ("sony_sdio_csdio_Initialize");

    if (!pSdioCsdio) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Open csdio driver. */
    pSdioCsdio->fd_f0 = open ("/dev/csdio0", O_RDWR);
    if (pSdioCsdio->fd_f0 < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    pSdioCsdio->fd_f1 = open ("/dev/csdiof1", O_RDWR);
    if (pSdioCsdio->fd_f1 < 0) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

sony_result_t sony_sdio_csdio_Finalize (sony_sdio_csdio_t * pSdioCsdio)
{
    SONY_TRACE_IO_ENTER ("sony_sdio_csdio_Finalize");

    if (!pSdioCsdio) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pSdioCsdio->fd_f1 >= 0) {
        close (pSdioCsdio->fd_f1);
        pSdioCsdio->fd_f1 = -1;
    }

    if (pSdioCsdio->fd_f0 >= 0) {
        close (pSdioCsdio->fd_f0);
        pSdioCsdio->fd_f0 = -1;
    }

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

sony_result_t sony_sdio_csdio_CreateSdio (sony_sdio_t * pSdio, sony_sdio_csdio_t * pSdioCsdio)
{
    SONY_TRACE_IO_ENTER ("sony_sdio_csdio_Create");

    if ((!pSdio) || (!pSdioCsdio)) {
        SONY_TRACE_IO_RETURN(SONY_RESULT_ERROR_ARG);
    }

    pSdio->ReadCMD52 = sony_sdio_csdio_ReadCMD52;
    pSdio->WriteCMD52 = sony_sdio_csdio_WriteCMD52;
    pSdio->ReadCMD53 = sony_sdio_csdio_ReadCMD53;
    pSdio->WriteCMD53 = sony_sdio_csdio_WriteCMD53;
    pSdio->flags = 0;
    pSdio->user = pSdioCsdio;

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}
