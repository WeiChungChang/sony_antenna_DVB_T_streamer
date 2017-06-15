/*------------------------------------------------------------------------------
  Copyright 2014 Sony Corporation

  Last Updated    : 2015/01/16
  Modification ID : d4b3fe129855d90c25271de02796915983f1e6ee
------------------------------------------------------------------------------*/
/**
 @file  sony_sdio.h

 @brief The SDIO I/O access interface.

        The user should implement SDIO read/write functions and
        set function pointer of them to this struct's member.
*/

#ifndef SONY_SDIO_H
#define SONY_SDIO_H

#include "sony_common.h"

/*------------------------------------------------------------------------------
  Enums
------------------------------------------------------------------------------*/
/**
 @brief OP code definition.
*/
typedef enum {
    SONY_SDIO_OP_CODE_FIXED,    /**< Fixed address */
    SONY_SDIO_OP_CODE_INCREMENT /**< Incrementing address */
} sony_sdio_op_code_t;

/*------------------------------------------------------------------------------
  Structs
------------------------------------------------------------------------------*/
/**
 @brief The SDIO driver API defintion.
*/
typedef struct sony_sdio_t {
    /**
     @brief Read data via SDIO using CMD52.

            Should be used for device register access.

     @param pSdio SDIO driver instance.
     @param registerAddress The register address. (17bit)
     @param pData The 1byte buffer to store the read data.

     @param function Function ID. (Not used for CXD2880)

     @return SONY_RESULT_OK if successful.
    */
    sony_result_t (* ReadCMD52) (struct sony_sdio_t * pSdio, uint32_t registerAddress, uint8_t * pData, uint8_t function);
    /**
     @brief Write data via SDIO using CMD52.

            Should be used for device register access.

     @param pSdio SDIO driver instance.
     @param registerAddress The register address. (17bit)
     @param data The 1byte buffer to store the read data.

     @param function Function ID. (Not used for CXD2880)

     @return SONY_RESULT_OK if successful.
    */
    sony_result_t (* WriteCMD52) (struct sony_sdio_t * pSdio, uint32_t registerAddress, uint8_t data, uint8_t function);
    /**
     @brief Read data via SDIO using CMD53.

            Should be used for TS data read.

     @param pSdio SDIO driver instance.
     @param registerAddress The register address. (17bit)
     @param pData The buffer to store the read data.
     @param size The number of bytes to read from the SDIO.
                 To reserve portability, this size should be multiple of block size.

     @param function Function ID. (Not used for CXD2880)
     @param opCode OP code.

     @return SONY_RESULT_OK if successful.
    */
    sony_result_t (* ReadCMD53) (struct sony_sdio_t * pSdio, uint32_t registerAddress, uint8_t * pData, uint32_t size,
        uint8_t function, sony_sdio_op_code_t opCode);
    /**
     @brief Write data via SDIO using CMD53.

     @param pSdio SDIO driver instance.
     @param registerAddress The register address. (17bit)
     @param pData The data to write to the device.
     @param size The number of bytes to write to the SDIO.

     @param function Function ID. (Not used for CXD2880)
     @param opCode OP code.

     @return SONY_RESULT_OK if successful.
    */
    sony_result_t (* WriteCMD53) (struct sony_sdio_t * pSdio, uint32_t registerAddress, const uint8_t * pData, uint32_t size,
        uint8_t function, sony_sdio_op_code_t opCode);

    uint32_t         flags;     /**< Flags that can be used by SDIO code */
    void*            user;      /**< User defined data. */
} sony_sdio_t;

#endif /* SONY_SDIO_H */
