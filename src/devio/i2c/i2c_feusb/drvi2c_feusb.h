/*------------------------------------------------------------------------------
  Copyright 2014 Sony Corporation

  Last Updated    : 2014/10/30
  Modification ID : 48a4df278e00b086435eed262de0e70d27d0ee29
------------------------------------------------------------------------------*/

#ifndef DRVI2C_FEUSB_H
#define DRVI2C_FEUSB_H

#include "sony_i2c.h"

#include <windows.h> /* For HANDLE definition */

typedef struct drvi2c_feusb_t
{
    HANDLE handle;
    void* user;
} drvi2c_feusb_t;

/* Bridge of I2C code */

sony_result_t drvi2c_feusb_Read(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t* pData,
                  uint32_t size, uint8_t mode);

sony_result_t drvi2c_feusb_Write(sony_i2c_t* pI2c, uint8_t deviceAddress, const uint8_t * pData,
                  uint32_t size, uint8_t mode);

sony_result_t drvi2c_feusb_ReadGw(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t* pData,
                  uint32_t size, uint8_t mode);

sony_result_t drvi2c_feusb_WriteGw(sony_i2c_t* pI2c, uint8_t deviceAddress, const uint8_t * pData,
                  uint32_t size, uint8_t mode);

sony_result_t drvi2c_feusb_ReadRegister(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t subAddress,
                  uint8_t* pData, uint32_t size);

sony_result_t drvi2c_feusb_Initialize(drvi2c_feusb_t* pDrvI2c);
sony_result_t drvi2c_feusb_Finalize(drvi2c_feusb_t* pDrvI2c);


sony_result_t drvi2c_feusb_CreateI2c(sony_i2c_t* pI2c, drvi2c_feusb_t* pDrvI2c);
sony_result_t drvi2c_feusb_CreateI2cGw(sony_i2c_t* pI2c, drvi2c_feusb_t* pDrvI2c, uint8_t gwAddress, uint8_t gwSub);

#endif /* DRVI2C_FEUSB_H */
