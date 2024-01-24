#ifndef NAND_H
#define NAND_H

#include "lpc_types.h"
#include "diskio.h"

#define NAND_EMULATED_SECTOR_BITS 12
#define NAND_EMULATED_SECTOR_SIZE (1<<NAND_EMULATED_SECTOR_BITS)

DSTATUS nand_disk_initialize(void);
DSTATUS nand_disk_status(void);
DRESULT nand_disk_ioctl(BYTE ctrl, void *buff);
DRESULT nand_disk_read(BYTE* buff, DWORD sector, BYTE count);
DRESULT nand_disk_write(const BYTE* buff, DWORD sector, BYTE count);

void nand_test(void);

#endif /* NAND_H */

