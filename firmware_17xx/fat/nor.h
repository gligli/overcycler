#ifndef NOR_H
#define NOR_H

#include "lpc_types.h"
#include "diskio.h"

DSTATUS nor_disk_initialize(void);
DSTATUS nor_disk_status(void);
DRESULT nor_disk_ioctl(BYTE ctrl, void *buff);
DRESULT nor_disk_read(BYTE* buff, DWORD sector, BYTE count);
DRESULT nor_disk_write(const BYTE* buff, DWORD sector, BYTE count);

void nor_test(void);

#endif /* NOR_H */

