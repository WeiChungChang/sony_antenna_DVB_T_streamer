/*------------------------------------------------------------------------------
  Copyright 2014 Sony Corporation

  Last Updated    : 2015/01/16
  Modification ID : d4b3fe129855d90c25271de02796915983f1e6ee
------------------------------------------------------------------------------*/

#include "sony_devio_sdio.h"

/*------------------------------------------------------------------------------
 Static Functions
------------------------------------------------------------------------------*/
static sony_result_t sony_regio_sdio_ReadRegister (sony_regio_t * pRegio, sony_regio_target_t target,
    uint8_t subAddress, uint8_t * pData, uint32_t size)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_sdio_t* pSdio = NULL;
    uint32_t registerAddress = subAddress;

    SONY_TRACE_IO_ENTER ("sony_regio_sdio_ReadRegister");

    if ((!pRegio) || (!pRegio->pIfObject) || (!pData)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (subAddress + size > 0x100) {
        /* This subAddress, size argument will be incorrect. */
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_RANGE);
    }

    pSdio = (sony_sdio_t*)(pRegio->pIfObject);

    /* SYSTEM registers are mapped to 0x00000 - 0x000FF
       DEMOD register are mapped to 0x00100 - 0x001FF */
    if (target == SONY_REGIO_TARGET_DEMOD) {
        registerAddress += 0x100;
    }

#ifdef SONY_REGIO_SDIO_USE_CMD53
    /* Function 1
       Block Mode is 0. (byte access) and
       OP Code is 1. (incrementing address) */
    result = pSdio->ReadCMD53 (pSdio, registerAddress, pData, size, 1, SONY_SDIO_OP_CODE_INCREMENT);
#else /* SONY_REGIO_SDIO_USE_CMD53 */
    {
        uint32_t i = 0;
        int regFreezeDone = 0;

        if (size > 1) {
            /* Multiple size read.
               This function should ensure multiple read data consistency.
               To achive it, register freeze should be used. */
            uint8_t data = 0;

            result = pSdio->ReadCMD52 (pSdio, target == SONY_REGIO_TARGET_SYSTEM ? 0x001 : 0x101, &data, 1);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_IO_RETURN (result);
            }

            if (data == 0x00) {
                /* Not frozen now. */
                result = pSdio->WriteCMD52 (pSdio, target == SONY_REGIO_TARGET_SYSTEM ? 0x001 : 0x101, 0x01, 1);
                if (result != SONY_RESULT_OK) {
                    SONY_TRACE_IO_RETURN (result);
                }

                regFreezeDone = 1;
            }
        }

        for (i = 0; i < size; i++) {
            result = pSdio->ReadCMD52 (pSdio, registerAddress + i, &pData[i], 1);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_IO_RETURN (result);
            }
        }

        if (regFreezeDone) {
            result = pSdio->WriteCMD52 (pSdio, target == SONY_REGIO_TARGET_SYSTEM ? 0x001 : 0x101, 0x00, 1);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_IO_RETURN (result);
            }
        }
    }
#endif /* SONY_REGIO_SDIO_USE_CMD53 */

    SONY_TRACE_IO_RETURN (result);
}

static sony_result_t sony_regio_sdio_WriteRegister (sony_regio_t * pRegio, sony_regio_target_t target,
    uint8_t subAddress, const uint8_t * pData, uint32_t size)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_sdio_t* pSdio = NULL;
    uint32_t registerAddress = subAddress;

    SONY_TRACE_IO_ENTER ("sony_regio_sdio_WriteRegister");

    if ((!pRegio) || (!pRegio->pIfObject) || (!pData)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (subAddress + size > 0x100) {
        /* This subAddress, size argument will be incorrect. */
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_RANGE);
    }

    pSdio = (sony_sdio_t*)(pRegio->pIfObject);

    /* SYSTEM registers are mapped to 0x00000 - 0x000FF
       DEMOD register are mapped to 0x00100 - 0x001FF */
    if (target == SONY_REGIO_TARGET_DEMOD) {
        registerAddress += 0x100;
    }

#ifdef SONY_REGIO_SDIO_USE_CMD53
    /* Function 1
       Block Mode is 0. (byte access) and
       OP Code is 1. (incrementing address) */
    result = pSdio->WriteCMD53 (pSdio, registerAddress, pData, size, 1, SONY_SDIO_OP_CODE_INCREMENT);
#else /* SONY_REGIO_SDIO_USE_CMD53 */
    {
        uint32_t i = 0;

        for (i = 0; i < size; i++) {
            result = pSdio->WriteCMD52 (pSdio, registerAddress + i, pData[i], 1);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_IO_RETURN (result);
            }
        }
    }
#endif /* SONY_REGIO_SDIO_USE_CMD53 */

    SONY_TRACE_IO_RETURN (result);
}

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
sony_result_t sony_regio_sdio_Create (sony_regio_t * pRegio, sony_sdio_t * pSdio, uint8_t slaveSelect)
{
    SONY_TRACE_IO_ENTER ("sony_regio_sdio_Create");

    if ((!pRegio) || (!pSdio)) {
        SONY_TRACE_IO_RETURN(SONY_RESULT_ERROR_ARG);
    }

    pRegio->ReadRegister = sony_regio_sdio_ReadRegister;
    pRegio->WriteRegister = sony_regio_sdio_WriteRegister;
    pRegio->WriteOneRegister = sony_regio_CommonWriteOneRegister;
    pRegio->pIfObject = pSdio;
    pRegio->i2cAddressSystem = 0;
    pRegio->i2cAddressDemod = 0;
    pRegio->slaveSelect = slaveSelect;

    SONY_TRACE_IO_RETURN (SONY_RESULT_OK);
}

sony_result_t sony_devio_sdio_ReadTS (sony_sdio_t * pSdio, uint8_t * pData, uint32_t size)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_IO_ENTER ("sony_devio_sdio_ReadTS");

    if ((!pSdio) || (!pData) || (size == 0)) {
        SONY_TRACE_IO_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Function 1
       OP Code is 0. (fixed address)
       Register Address is 0x10000. */
    result = pSdio->ReadCMD53 (pSdio, 0x10000, pData, size, 1, SONY_SDIO_OP_CODE_FIXED);

    SONY_TRACE_IO_RETURN (result);
}
