#ifndef W25Q_H
#define W25Q_H

#include "lpc_types.h"

#define W25Q_PAGE_BITS 8
#define W25Q_PAGE_SIZE (1<<W25Q_PAGE_BITS)
#define W25Q_PAGE_MASK (W25Q_PAGE_SIZE-1)

#define W25Q_SECTOR_BITS 12
#define W25Q_SECTOR_SIZE (1<<W25Q_SECTOR_BITS)
#define W25Q_SECTOR_MASK (W25Q_SECTOR_SIZE-1)

#define W25Q_SECTOR_COUNT 16384 // 64MB

void W25Q_readSector(uint32_t index, uint8_t * buffer);
void W25Q_writeSector(uint32_t index, const uint8_t * buffer);

int8_t W25Q_init(void);

#endif /* W25Q_H */

