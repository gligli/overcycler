
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <string.h>
#include "sd_raw.h"
#include "LPC214x.h"
#include <stdio.h>
#include "rprintf.h"

/**
 * \addtogroup sd_raw MMC/SD card raw access
 *
 * This module implements read and write access to MMC and
 * SD cards. It serves as a low-level driver for the higher
 * level modules such as partition and file system access.
 *
 * @{
 */
/**
 * \file
 * MMC/SD raw access implementation.
 *
 * \author Roland Riegel
 */

/**
 * \addtogroup sd_raw_config MMC/SD configuration
 * Preprocessor defines to configure the MMC/SD support.
 */

/**
 * @}
 */

/* commands available in SPI mode */

/* CMD0: response R1 */
#define CMD_GO_IDLE_STATE 0x00
/* CMD1: response R1 */
#define CMD_SEND_OP_COND 0x01
/* CMD9: response R1 */
#define CMD_SEND_CSD 0x09
/* CMD10: response R1 */
#define CMD_SEND_CID 0x0a
/* CMD12: response R1 */
#define CMD_STOP_TRANSMISSION 0x0c
/* CMD13: response R2 */
#define CMD_SEND_STATUS 0x0d
/* CMD16: arg0[31:0]: block length, response R1 */
#define CMD_SET_BLOCKLEN 0x10
/* CMD17: arg0[31:0]: data address, response R1 */
#define CMD_READ_SINGLE_BLOCK 0x11
/* CMD18: arg0[31:0]: data address, response R1 */
#define CMD_READ_MULTIPLE_BLOCK 0x12
/* CMD24: arg0[31:0]: data address, response R1 */
#define CMD_WRITE_SINGLE_BLOCK 0x18
/* CMD25: arg0[31:0]: data address, response R1 */
#define CMD_WRITE_MULTIPLE_BLOCK 0x19
/* CMD27: response R1 */
#define CMD_PROGRAM_CSD 0x1b
/* CMD28: arg0[31:0]: data address, response R1b */
#define CMD_SET_WRITE_PROT 0x1c
/* CMD29: arg0[31:0]: data address, response R1b */
#define CMD_CLR_WRITE_PROT 0x1d
/* CMD30: arg0[31:0]: write protect data address, response R1 */
#define CMD_SEND_WRITE_PROT 0x1e
/* CMD32: arg0[31:0]: data address, response R1 */
#define CMD_TAG_SECTOR_START 0x20
/* CMD33: arg0[31:0]: data address, response R1 */
#define CMD_TAG_SECTOR_END 0x21
/* CMD34: arg0[31:0]: data address, response R1 */
#define CMD_UNTAG_SECTOR 0x22
/* CMD35: arg0[31:0]: data address, response R1 */
#define CMD_TAG_ERASE_GROUP_START 0x23
/* CMD36: arg0[31:0]: data address, response R1 */
#define CMD_TAG_ERASE_GROUP_END 0x24
/* CMD37: arg0[31:0]: data address, response R1 */
#define CMD_UNTAG_ERASE_GROUP 0x25
/* CMD38: arg0[31:0]: stuff bits, response R1b */
#define CMD_ERASE 0x26
/* CMD42: arg0[31:0]: stuff bits, response R1b */
#define CMD_LOCK_UNLOCK 0x2a
/* CMD58: response R3 */
#define CMD_READ_OCR 0x3a
/* CMD59: arg0[31:1]: stuff bits, arg0[0:0]: crc option, response R1 */
#define CMD_CRC_ON_OFF 0x3b

/* command responses */
/* R1: size 1 byte */
#define R1_IDLE_STATE 0
#define R1_ERASE_RESET 1
#define R1_ILL_COMMAND 2
#define R1_COM_CRC_ERR 3
#define R1_ERASE_SEQ_ERR 4
#define R1_ADDR_ERR 5
#define R1_PARAM_ERR 6
/* R1b: equals R1, additional busy bytes */
/* R2: size 2 bytes */
#define R2_CARD_LOCKED 0
#define R2_WP_ERASE_SKIP 1
#define R2_ERR 2
#define R2_CARD_ERR 3
#define R2_CARD_ECC_FAIL 4
#define R2_WP_VIOLATION 5
#define R2_INVAL_ERASE 6
#define R2_OUT_OF_RANGE 7
#define R2_CSD_OVERWRITE 7
#define R2_IDLE_STATE (R1_IDLE_STATE + 8)
#define R2_ERASE_RESET (R1_ERASE_RESET + 8)
#define R2_ILL_COMMAND (R1_ILL_COMMAND + 8)
#define R2_COM_CRC_ERR (R1_COM_CRC_ERR + 8)
#define R2_ERASE_SEQ_ERR (R1_ERASE_SEQ_ERR + 8)
#define R2_ADDR_ERR (R1_ADDR_ERR + 8)
#define R2_PARAM_ERR (R1_PARAM_ERR + 8)
/* R3: size 5 bytes */
#define R3_OCR_MASK (0xffffffffUL)
#define R3_IDLE_STATE (R1_IDLE_STATE + 32)
#define R3_ERASE_RESET (R1_ERASE_RESET + 32)
#define R3_ILL_COMMAND (R1_ILL_COMMAND + 32)
#define R3_COM_CRC_ERR (R1_COM_CRC_ERR + 32)
#define R3_ERASE_SEQ_ERR (R1_ERASE_SEQ_ERR + 32)
#define R3_ADDR_ERR (R1_ADDR_ERR + 32)
#define R3_PARAM_ERR (R1_PARAM_ERR + 32)
/* Data Response: size 1 byte */
#define DR_STATUS_MASK 0x0e
#define DR_STATUS_ACCEPTED 0x05
#define DR_STATUS_CRC_ERR 0x0a
#define DR_STATUS_WRITE_ERR 0x0c

#if !SD_RAW_SAVE_RAM
    
    /* static data buffer for acceleration */
    static unsigned char raw_block[512];
    /* offset where the data within raw_block lies on the card */
    static unsigned int raw_block_address;
    #if SD_RAW_WRITE_BUFFERING
    /* flag to remember if raw_block was written to the card */
    static unsigned char raw_block_written;
#endif

#endif

/* private helper functions */
static void sd_raw_send_byte(unsigned char b);
static unsigned char sd_raw_rec_byte(void);
static unsigned char sd_raw_send_command_r1(unsigned char command, unsigned int arg);
//static unsigned short sd_raw_send_command_r2(unsigned char command, unsigned int arg);

/**
 * \ingroup sd_raw
 * Initializes memory card communication.
 *
 * \returns 0 on failure, 1 on success.
 */
unsigned char sd_raw_init()
{
    /* enable inputs for reading card status */
    /*    configure_pin_available();*/
    /*    configure_pin_locked();*/

    /* enable outputs for MOSI, SCK, SS, input for MISO */
    configure_pin_ss();
    configure_pin_mosi();
    configure_pin_miso();
    configure_pin_sck();

    unselect_card();

    /* initialize SPI with lowest frequency; max. 400kHz during identification mode of card */
    S0SPCCR = 150;  /* Set frequency to 400kHz */
    S0SPCR = 0x38;


    /* initialization procedure */

    if(!sd_raw_available())
    {
        rprintf("SD RAW NOT AVAILABLE\n\r");
        return 0;
    }
    configure_pin_ss();
    unselect_card();

    unsigned short i;
    /* card needs 74 cycles minimum to start up */
    for(i = 0; i < 10; ++i)
    {
        /* wait 8 clock cycles */
        sd_raw_rec_byte();
    }

    /* address card */
    select_card();

    /* reset card */
    unsigned char response;
    for(i = 0; ; ++i)
    {
        response = sd_raw_send_command_r1(CMD_GO_IDLE_STATE, 0);
        if(response == (1 << R1_IDLE_STATE))
            break;

        if(i == 0x1ff)
        {
            rprintf("\n\rresponse: %d\n\r",response);
            unselect_card();
            return 0;
        }
    }

    /* wait for card to get ready */
    for(i = 0; ; ++i)
    {
        response = sd_raw_send_command_r1(CMD_SEND_OP_COND, 0);
        if(!(response & (1 << R1_IDLE_STATE)))
            break;

        if(i == 0x7fff)
        {
            unselect_card();
            rprintf("i = 0x7fff\n\r");
            return 0;
        }
    }

    /* set block size to 512 bytes */
    if(sd_raw_send_command_r1(CMD_SET_BLOCKLEN, 512))
    {
        unselect_card();
        rprintf("BLOCK SIZE SET ERR \n\r");
        return 0;
    }

    /* deaddress card */
    unselect_card();

    /* switch to highest SPI frequency possible */
    S0SPCCR = 60; /* ~1MHz-- potentially can be faster */

    #if !SD_RAW_SAVE_RAM
        /* the first block is likely to be accessed first, so precache it here */
        raw_block_address = 0xffffffff;
        #if SD_RAW_WRITE_BUFFERING
        raw_block_written = 1;
    #endif
    if(!sd_raw_read(0, raw_block, sizeof(raw_block)))
    {
        rprintf("sd_raw_read borks\n\r");
        return 0;
    }
    #endif

    return 1;
}

/**
 * \ingroup sd_raw
 * Checks wether a memory card is located in the slot.
 *
 * \returns 1 if the card is available, 0 if it is not.
 */
unsigned char sd_raw_available()
{
    unsigned int i;
    configure_pin_available();
    for(i=0;i<100000;i++);
    i = get_pin_available();
    configure_pin_ss();
    return i == 0x00;
}

/**
 * \ingroup sd_raw
 * Checks wether the memory card is locked for write access.
 *
 * \returns 1 if the card is locked, 0 if it is not.
 */
unsigned char sd_raw_locked()
{
    return get_pin_locked() == 0x00;
}

/**
 * \ingroup sd_raw
 * Sends a raw byte to the memory card.
 *
 * \param[in] b The byte to sent.
 * \see sd_raw_rec_byte
 */
void sd_raw_send_byte(unsigned char b)
{
    S0SPDR = b;
    /* wait for byte to be shifted out */
    while(!(S0SPSR & 0x80));
}

/**
 * \ingroup sd_raw
 * Receives a raw byte from the memory card.
 *
 * \returns The byte which should be read.
 * \see sd_raw_send_byte
 */
unsigned char sd_raw_rec_byte(void)
{
    /* send dummy data for receiving some */
    S0SPDR = 0xff;
    while(!(S0SPSR & 0x80));

    return S0SPDR;
}

/**
 * \ingroup sd_raw
 * Send a command to the memory card which responses with a R1 response.
 *
 * \param[in] command The command to send.
 * \param[in] arg The argument for command.
 * \returns The command answer.
 */
unsigned char sd_raw_send_command_r1(unsigned char command, unsigned int arg)
{
    unsigned char response;
    unsigned char i;

    /* wait some clock cycles */
    sd_raw_rec_byte();

    /* send command via SPI */
    sd_raw_send_byte(0x40 | command);
    sd_raw_send_byte((arg >> 24) & 0xff);
    sd_raw_send_byte((arg >> 16) & 0xff);
    sd_raw_send_byte((arg >> 8) & 0xff);
    sd_raw_send_byte((arg >> 0) & 0xff);
    sd_raw_send_byte((command == CMD_GO_IDLE_STATE) ? 0x95 : 0xff);

    /* receive response */
    for(i = 0; i < 10; ++i)
    {
        response = sd_raw_rec_byte();
        if(response != 0xff)
            break;
    }

    return response;
}

/**
 * \ingroup sd_raw
 * Send a command to the memory card which responses with a R2 response.
 *
 * \param[in] command The command to send.
 * \param[in] arg The argument for command.
 * \returns The command answer.
 */
/*
unsigned short sd_raw_send_command_r2(unsigned char command, unsigned int arg)
{
    unsigned short response;
    unsigned char i;

    // wait some clock cycles
    sd_raw_rec_byte();

    // send command via SPI
    sd_raw_send_byte(0x40 | command);
    sd_raw_send_byte((arg >> 24) & 0xff);
    sd_raw_send_byte((arg >> 16) & 0xff);
    sd_raw_send_byte((arg >> 8) & 0xff);
    sd_raw_send_byte((arg >> 0) & 0xff);
    sd_raw_send_byte(command == CMD_GO_IDLE_STATE ? 0x95 : 0xff);

    // receive response
    for(i = 0; i < 10; ++i)
    {
        response = sd_raw_rec_byte();
        if(response != 0xff)
            break;
    }
    response <<= 8;
    response |= sd_raw_rec_byte();

    return response;
}
*/

/**
 * \ingroup sd_raw
 * Reads raw data from the card.
 *
 * \param[in] offset The offset from which to read.
 * \param[out] buffer The buffer into which to write the data.
 * \param[in] length The number of bytes to read.
 * \returns 0 on failure, 1 on success.
 * \see sd_raw_read_interval, sd_raw_write
 */
unsigned char sd_raw_read(unsigned int offset, unsigned char* buffer, unsigned short length)
{
    unsigned int block_address;
    unsigned short block_offset;
    unsigned short read_length;
    while(length > 0)
    {
        /* determine byte count to read at once */
        block_address = offset & 0xfffffe00;
        block_offset = offset & 0x01ff;
        read_length = 512 - block_offset; /* read up to block border */
        if(read_length > length)
            read_length = length;

        #if !SD_RAW_SAVE_RAM
            /* check if the requested data is cached */
            if(block_address != raw_block_address)
            #endif
        {
            #if SD_RAW_WRITE_BUFFERING
                if(!raw_block_written)
                {
                    if(!sd_raw_write(raw_block_address, raw_block, sizeof(raw_block)))
                        return 0;
                }
            #endif

            /* address card */
            select_card();

            /* send single block request */
            if(sd_raw_send_command_r1(CMD_READ_SINGLE_BLOCK, block_address))
            {
                unselect_card();
                return 0;
            }

            /* wait for data block (start byte 0xfe) */
            while(sd_raw_rec_byte() != 0xfe);

            #if SD_RAW_SAVE_RAM
                /* read byte block */
                unsigned short read_to = block_offset + read_length;
                for(unsigned short i = 0; i < 512; ++i)
                {
                    unsigned char b = sd_raw_rec_byte();
                    if(i >= block_offset && i < read_to)
                        *buffer++ = b;
                }
            #else
                /* read byte block */
                unsigned char* cache = raw_block;
                unsigned short i;
                for(i = 0; i < 512; ++i)
                    *cache++ = sd_raw_rec_byte();
                raw_block_address = block_address;
    
                memcpy(buffer, raw_block + block_offset, read_length);
                buffer += read_length;
            #endif

            /* read crc16 */
            sd_raw_rec_byte();
            sd_raw_rec_byte();

            /* deaddress card */
            unselect_card();

            /* let card some time to finish */
            sd_raw_rec_byte();
        }
        #if !SD_RAW_SAVE_RAM
            else
            {
                /* use cached data */
                memcpy(buffer, raw_block + block_offset, read_length);
            }
        #endif

        length -= read_length;
        offset += read_length;
    }

    return 1;
}

/**
 * \ingroup sd_raw
 * Continuously reads units of \c interval bytes and calls a callback function.
 *
 * This function starts reading at the specified offset. Every \c interval bytes,
 * it calls the callback function with the associated data buffer.
 *
 * By returning zero, the callback may stop reading.
 *
 * \note Within the callback function, you can not start another read or
 *       write operation.
 * \note This function only works if the following conditions are met:
 *       - (offset - (offset % 512)) % interval == 0
 *       - length % interval == 0
 *
 * \param[in] offset Offset from which to start reading.
 * \param[in] buffer Pointer to a buffer which is at least interval bytes in size.
 * \param[in] interval Number of bytes to read before calling the callback function.
 * \param[in] length Number of bytes to read altogether.
 * \param[in] callback The function to call every interval bytes.
 * \param[in] p An opaque pointer directly passed to the callback function.
 * \returns 0 on failure, 1 on success
 * \see sd_raw_read, sd_raw_write
 */
unsigned char sd_raw_read_interval(unsigned int offset, unsigned char* buffer, unsigned short interval, unsigned short length, sd_raw_interval_handler callback, void* p)
{
    if(!buffer || interval == 0 || length < interval || !callback)
        return 0;

    #if !SD_RAW_SAVE_RAM
        while(length >= interval)
        {
            /* as reading is now buffered, we directly
                     * hand over the request to sd_raw_read()
                     */
            if(!sd_raw_read(offset, buffer, interval))
                return 0;
            if(!callback(buffer, offset, p))
                break;
            offset += interval;
            length -= interval;
        }
    
        return 1;
    #else
        /* address card */
        select_card();
    
        unsigned short block_offset;
        unsigned short read_length;
        unsigned char* buffer_cur;
        unsigned char finished = 0;
        do
        {
            /* determine byte count to read at once */
            block_offset = offset & 0x01ff;
            read_length = 512 - block_offset;
    
            /* send single block request */
            if(sd_raw_send_command_r1(CMD_READ_SINGLE_BLOCK, offset & 0xfffffe00))
            {
                unselect_card();
                return 0;
            }
    
            /* wait for data block (start byte 0xfe) */
            while(sd_raw_rec_byte() != 0xfe);
            unsigned short i;
            /* read up to the data of interest */
            for(i = 0; i < block_offset; ++i)
                sd_raw_rec_byte();
    
            /* read interval bytes of data and execute the callback */
            do
            {
                if(read_length < interval || length < interval)
                    break;
    
                buffer_cur = buffer;
                for(i = 0; i < interval; ++i)
                    *buffer_cur++ = sd_raw_rec_byte();
    
                if(!callback(buffer, offset + (512 - read_length), p))
                {
                    finished = 1;
                    break;
                }
    
                read_length -= interval;
                length -= interval;
    
            }
            while(read_length > 0 && length > 0);
    
            /* read rest of data block */
            while(read_length-- > 0)
                sd_raw_rec_byte();
    
            /* read crc16 */
            sd_raw_rec_byte();
            sd_raw_rec_byte();
    
            if(length < interval)
                break;
    
            offset = (offset & 0xfffffe00) + 512;
    
        }
        while(!finished);
    
        /* deaddress card */
        unselect_card();
    
        /* let card some time to finish */
        sd_raw_rec_byte();
    
        return 1;
    #endif
}

/**
 * \ingroup sd_raw
 * Writes raw data to the card.
 *
 * \note If write buffering is enabled, you might have to
 *       call sd_raw_sync() before disconnecting the card
 *       to ensure all remaining data has been written.
 *
 * \param[in] offset The offset where to start writing.
 * \param[in] buffer The buffer containing the data to be written.
 * \param[in] length The number of bytes to write.
 * \returns 0 on failure, 1 on success.
 * \see sd_raw_read
 */
unsigned char sd_raw_write(unsigned int offset, const unsigned char* buffer, unsigned short length)
{
    #if SD_RAW_WRITE_SUPPORT
    
        if(get_pin_locked())
            return 0;
    
        unsigned int block_address;
        unsigned short block_offset;
        unsigned short write_length;
        while(length > 0)
        {
            /* determine byte count to write at once */
            block_address = offset & 0xfffffe00;
            block_offset = offset & 0x01ff;
            write_length = 512 - block_offset; /* write up to block border */
            if(write_length > length)
                write_length = length;
    
            /* Merge the data to write with the content of the block.
                     * Use the cached block if available.
                     */
            if(block_address != raw_block_address)
            {
                #if SD_RAW_WRITE_BUFFERING
                if(!raw_block_written)
                {
                    if(!sd_raw_write(raw_block_address, raw_block, sizeof(raw_block)))
                        return 0;
                }
            #endif

            if(block_offset || write_length < 512)
            {
                if(!sd_raw_read(block_address, raw_block, sizeof(raw_block)))
                    return 0;
            }
            raw_block_address = block_address;
        }

        if(buffer != raw_block)
        {
            memcpy(raw_block + block_offset, buffer, write_length);

            #if SD_RAW_WRITE_BUFFERING
                raw_block_written = 0;
    
                if(length == write_length)
                    return 1;
            #endif
        }

        buffer += write_length;

        /* address card */
        select_card();

        /* send single block request */
        if(sd_raw_send_command_r1(CMD_WRITE_SINGLE_BLOCK, block_address))
        {
            unselect_card();
            return 0;
        }

        /* send start byte */
        sd_raw_send_byte(0xfe);

        /* write byte block */
        unsigned char* cache = raw_block;
        unsigned short i;
        for(i = 0; i < 512; ++i)
            sd_raw_send_byte(*cache++);

        /* write dummy crc16 */
        sd_raw_send_byte(0xff);
        sd_raw_send_byte(0xff);

        /* wait while card is busy */
        while(sd_raw_rec_byte() != 0xff);
        sd_raw_rec_byte();

        /* deaddress card */
        unselect_card();

        length -= write_length;
        offset += write_length;

        #if SD_RAW_WRITE_BUFFERING
            raw_block_written = 1;
        #endif
    }

    return 1;
    #else
        return 0;
    #endif
}

/**
 * \ingroup sd_raw
 * Writes the write buffer's content to the card.
 *
 * \note When write buffering is enabled, you should
 *       call this function before disconnecting the
 *       card to ensure all remaining data has been
 *       written.
 *
 * \returns 0 on failure, 1 on success.
 * \see sd_raw_write
 */
unsigned char sd_raw_sync()
{
    #if SD_RAW_WRITE_SUPPORT
        #if SD_RAW_WRITE_BUFFERING
        if(raw_block_written)
            return 1;
        if(!sd_raw_write(raw_block_address, raw_block, sizeof(raw_block)))
            return 0;
    #endif
    return 1;
    #else
    return 0;
    #endif
}

/**
 * \ingroup sd_raw
 * Reads informational data from the card.
 *
 * This function reads and returns the card's registers
 * containing manufacturing and status information.
 *
 * \note: The information retrieved by this function is
 *        not required in any way to operate on the card,
 *        but it might be nice to display some of the data
 *        to the user.
 *
 * \param[in] info A pointer to the structure into which to save the information.
 * \returns 0 on failure, 1 on success.
 */
unsigned char sd_raw_get_info(struct sd_raw_info* info)
{
    if(!info || !sd_raw_available())
        return 0;

    memset(info, 0, sizeof(*info));

    select_card();

    /* read cid register */
    if(sd_raw_send_command_r1(CMD_SEND_CID, 0))
    {
        unselect_card();
        return 0;
    }
    while(sd_raw_rec_byte() != 0xfe);
    unsigned char i;
    for(i = 0; i < 18; ++i)
    {
        unsigned char b = sd_raw_rec_byte();

        switch(i)
        {
            case 0:
                info->manufacturer = b;
                break;
            case 1:
            case 2:
                info->oem[i - 1] = b;
                break;
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                info->product[i - 3] = b;
                break;
            case 8:
                info->revision = b;
                break;
            case 9:
            case 10:
            case 11:
            case 12:
                info->serial |= (unsigned int) b << ((12 - i) * 8);
                break;
            case 13:
                info->manufacturing_year = b << 4;
                break;
            case 14:
                info->manufacturing_year |= b >> 4;
                info->manufacturing_month = b & 0x0f;
                break;
        }
    }

    /* read csd register */
    unsigned char csd_read_bl_len = 0;
    unsigned char csd_c_size_mult = 0;
    unsigned short csd_c_size = 0;
    if(sd_raw_send_command_r1(CMD_SEND_CSD, 0))
    {
        unselect_card();
        return 0;
    }
    while(sd_raw_rec_byte() != 0xfe);
    for(i = 0; i < 18; ++i)
    {
        unsigned char b = sd_raw_rec_byte();

        switch(i)
        {
            case 5:
                csd_read_bl_len = b & 0x0f;
                break;
            case 6:
                csd_c_size = (unsigned short) (b & 0x03) << 8;
                break;
            case 7:
                csd_c_size |= b;
                csd_c_size <<= 2;
                break;
            case 8:
                csd_c_size |= b >> 6;
                ++csd_c_size;
                break;
            case 9:
                csd_c_size_mult = (b & 0x03) << 1;
                break;
            case 10:
                csd_c_size_mult |= b >> 7;

                info->capacity = (unsigned int) csd_c_size << (csd_c_size_mult + csd_read_bl_len + 2);

                break;
            case 14:
                if(b & 0x40)
                    info->flag_copy = 1;
                if(b & 0x20)
                    info->flag_write_protect = 1;
                if(b & 0x10)
                    info->flag_write_protect_temp = 1;
                info->format = (b & 0x0c) >> 2;
                break;
        }
    }

    unselect_card();

    return 1;
}

void SDoff(void)
{
    SPI_SS_IODIR &= ~(1<<SPI_SS_PIN);
    PINSEL0 &= ~(0x1500);
}

//NES : 10-28-7 
//Low-level formats a 512MB card
//Assumes *many* things
//You must pass this fuction 0xAA to get it to work (safety check)
char format_card(char make_sure)
{
	#define MBR_LOCATION	0x00
	#define BR_LOCATION		(MBR_LOCATION+0x80000)
	#define FAT_TABLE		(BR_LOCATION + (0x200 * 512))
	#define ROOT_DIR		(BR_LOCATION + (0x0200 * 512) + (0x00F5 * 2 * 512))

	//Safety check
	if (make_sure != 0xAA) return 0;
	
	int i;
	unsigned char my_buff[512];
	for(i = 0 ; i < 512 ; i++) my_buff[i] = 0x00;
	
	//Init SD card interface
	sd_raw_init();

	//Erase Master Boot record
	sd_raw_sync();
	sd_raw_write(MBR_LOCATION, my_buff, 512);

	//Erase Boot record
	sd_raw_sync();
	sd_raw_write(BR_LOCATION, my_buff, 512);

	//Erase FAT tables
	for(i = 0 ; i < 0x00F5 ; i++) //0x00F5 = 245 bytes : comes from byte 0x16 from Boot Record
	{
		sd_raw_sync();
		sd_raw_write( (FAT_TABLE + (i*512)), my_buff, 512);
	}
	
	//Write Master Boot Record
	#define PART1	0x01BE
	my_buff[PART1 + 0] = 0x00;
	my_buff[PART1 + 1] = 0x00;
	my_buff[PART1 + 2] = 0x01;
	my_buff[PART1 + 3] = 0x01;
	my_buff[PART1 + 4] = 0x06;
	my_buff[PART1 + 5] = 0x1F;
	my_buff[PART1 + 6] = 0xE0;
	my_buff[PART1 + 7] = 0xD3;
	my_buff[PART1 + 8] = 0x00;
	my_buff[PART1 + 9] = 0x04;
	my_buff[PART1 + 10] = 0x00;
	my_buff[PART1 + 11] = 0x00;
	my_buff[PART1 + 12] = 0x00;
	my_buff[PART1 + 13] = 0x4C;
	my_buff[PART1 + 14] = 0x0F;
	my_buff[510] = 0x55;
	my_buff[511] = 0xAA;

	sd_raw_sync();
	sd_raw_write(MBR_LOCATION, my_buff, 512);
	sd_raw_sync();

	//Write Boot Record
	#define BOOTRECORD1	0x80000
	my_buff[0] = 0xEB;
	my_buff[1] = 0xFE;
	my_buff[2] = 0x90;
	my_buff[12] = 0x02;
	my_buff[13] = 0x10;
	my_buff[14] = 0x16;
	my_buff[16] = 0x02;
	my_buff[18] = 0x02;
	my_buff[21] = 0xF8;
	my_buff[22] = 0xF5;
	my_buff[24] = 0x20;
	my_buff[26] = 0x20;
	my_buff[29] = 0x04;
	my_buff[33] = 0x4C;
	my_buff[34] = 0x0F;
	my_buff[38] = 0x29;
	my_buff[54] = 0x46;
	my_buff[55] = 0x41;
	my_buff[56] = 0x54;
	my_buff[57] = 0x31;
	my_buff[58] = 0x36;
	my_buff[59] = 0x20;
	my_buff[60] = 0x20;
	my_buff[61] = 0x20;
	my_buff[510] = 0x55;
	my_buff[511] = 0xAA;
	
	sd_raw_sync();
	sd_raw_write(BR_LOCATION, my_buff, 512);
	sd_raw_sync();
	
	return(0x55); //Successful format
}
