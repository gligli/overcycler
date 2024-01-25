#ifndef __XT26G_H__
#define __XT26G_H__

#include "lpc_types.h"

#define XT26G_PAGE_BITS 11
#define XT26G_PAGE_SIZE (1<<XT26G_PAGE_BITS)
#define XT26G_PAGE_MASK (XT26G_PAGE_SIZE-1)

#define XT26G_BLOCK_BITS (XT26G_PAGE_BITS+6)
#define XT26G_BLOCK_SIZE (1<<XT26G_BLOCK_BITS)
#define XT26G_BLOCK_MASK (XT26G_BLOCK_SIZE-1)

#define XT26G_TOTAL_BLOCK_COUNT 2048 // 256MB
#define XT26G_USABLE_BLOCK_COUNT 2000

#define XT26G_UNIQUE_ID_SIZE 16

void XT26G_readPage(uint32_t address, uint16_t size, int8_t is_spare, uint8_t * buffer);
void XT26G_writePage(uint32_t address, uint16_t size, uint8_t * buffer);
void XT26G_eraseBlock(uint32_t address);
void XT26G_movePage(uint32_t address, uint32_t newAddress, uint16_t size, uint8_t * buffer);
void XT26G_getUniqueID(uint8_t * buffer); // needs XT26G_UNIQUE_ID_SIZE bytes buffer

int8_t XT26G_init(void);

#endif
