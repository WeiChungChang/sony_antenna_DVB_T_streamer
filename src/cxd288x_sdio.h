#ifndef _CXD288X_SDIO_H
#define _CXD288X_SDIO_H

#include <linux/ioctl.h>
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#include <stddef.h>
#endif

/*
 * In CMD53, register address is 17 bit length.
 * CMD53 can handle both fixed address (FIFO) and incrementing address.
 * This driver uses special address to handle both access.
 *
 * 0x00000 - 0x1FFFF  ->  Fixed.
 * 0x20000 - 0x3FFFF  ->  Increment.
 *
 * Following value should be added to absolute position value in fseek like as follows.
 * fseek(fd, 0x01000 + CXD288X_SDIO_FPOS_INCREMENT, SEEK_SET);
 */
#define CXD288X_SDIO_FPOS_OP_FIXED              0x00000
#define CXD288X_SDIO_FPOS_OP_INCREMENT          0x20000

struct cxd288x_sdio_cmd52_ctrl_t {
	uint8_t     m_write;       /* 0: read, 1: write */
	uint32_t    m_address;     /* 17bit address */
	uint8_t     m_data;        /* 1byte data */
} __attribute__ ((packed));

struct cxd288x_sdio_atomic_regaccess_t {
	uint8_t     m_write;       /* 0: read, 1: write */
	uint32_t    m_address;     /* 17bit address */
	uint8_t     m_data;        /* 1byte data */
	uint8_t     m_bank;        /* bank is set before access, and restored after access */
} __attribute__ ((packed));

#define CXD288X_SDIO_IOC_MAGIC                  'w'

#define CXD288X_SDIO_IOC_CMD52                  _IOWR(CXD288X_SDIO_IOC_MAGIC, 0, struct cxd288x_sdio_cmd52_ctrl_t)
#define CXD288X_SDIO_IOC_CMD52_FN0              _IOWR(CXD288X_SDIO_IOC_MAGIC, 1, struct cxd288x_sdio_cmd52_ctrl_t)
#define CXD288X_SDIO_IOC_ATOMIC_REGACCESS       _IOWR(CXD288X_SDIO_IOC_MAGIC, 2, struct cxd288x_sdio_atomic_regaccess_t)
#define CXD288X_SDIO_IOC_ALLOC_BUFFER           _IOW(CXD288X_SDIO_IOC_MAGIC, 3, size_t)
#define CXD288X_SDIO_IOC_SET_BLOCK_SIZE         _IOW(CXD288X_SDIO_IOC_MAGIC, 4, unsigned int)

#endif /* _CXD288X_SDIO_H */
