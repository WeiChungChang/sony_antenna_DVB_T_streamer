/*------------------------------------------------------------------------------
  Copyright 2014 Sony Corporation

  Last Updated    : 2015/04/21
  Modification ID : 3016fe47ebd54ca84ce4dc6d1ab42d1012f82360
------------------------------------------------------------------------------*/
/**
 @file  sony_i2c_i2cdev.h

 @brief The I2C I/O access implemenation using "i2c-dev" driver.
*/
#ifndef SONY_I2C_I2CDEV_H
#define SONY_I2C_I2CDEV_H

#include "sony_i2c.h"

/*------------------------------------------------------------------------------
  Structs
------------------------------------------------------------------------------*/
/**
 @brief Struct to store i2c-dev related information.
*/
typedef struct sony_i2c_i2cdev_t
{
    int fd;    /**< file descriptor */
} sony_i2c_i2cdev_t;

/*------------------------------------------------------------------------------
  Functions
------------------------------------------------------------------------------*/
/**
 @brief Initialize I2C access via "i2c-dev".

 @param pI2cI2cdev       sony_i2c_i2cdev_t struct instance.
 @param busNum           Bus number.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_i2c_i2cdev_Initialize (sony_i2c_i2cdev_t* pI2cI2cdev, uint8_t busNum);

/**
 @brief Finalize I2C access via "i2c-dev".

 @param pI2cI2cdev       sony_i2c_i2cdev_t struct instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_i2c_i2cdev_Finalize (sony_i2c_i2cdev_t* pI2cI2cdev);
/**
 @brief Set up the I2C access struct using "i2c-dev".

 @param pI2c             I2C struct instance.
 @param pI2cI2cdev       sony_i2c_i2cdev_t struct instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_i2c_i2cdev_CreateI2c (sony_i2c_t* pI2c, sony_i2c_i2cdev_t* pI2cI2cdev);

/**
 @brief Set up the I2C access struct using "i2c-dev".(via I2C gateway)

 @param pI2c             I2C struct instance.
 @param pI2cI2cdev       sony_i2c_i2cdev_t struct instance.
 @param gwAddress        Gateway address (e.g Demod device address).
 @param gwSub            Gateway sub address.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_i2c_i2cdev_CreateI2cGw (sony_i2c_t* pI2c, sony_i2c_i2cdev_t* pI2cI2cdev, uint8_t gwAddress, uint8_t gwSub);
#endif /* SONY_I2C_I2CDEV_H */
