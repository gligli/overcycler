/*
	NES 11-6-07
	These are inherited functions
	
	Used only by bootloader.
	
	Pull 512 byte chunks from SD card and record them into LPC flash memory using IAP
	
	Current bootloader ends at sector 7
	User settings are contained in sector 8
	Start erasing for main code in sector 9
*/

#include "firmware.h"
#include <stdio.h>
#include "LPC214x.h"

#include "fat16.h"
#include "rootdir.h"
#include "system.h"
#include "sd_raw.h"
#include "rprintf.h"

#ifndef ERRORCODE
    #define ERRORCODE(x)
#endif

/* READBUFSIZE must be a size allowed
 * by the LPC2148 IAP copy ram to flash command
 */
#define READBUFSIZE 512

#define STARTSECTOR 0x00008000
#define STARTLSB    15
#define STARTNUM    8
#define TRICKYSECT  0x00079000
#define TRICKYNUM   23
#define TRICKYLSB   12
#define SECTOR_NUMBER(x) ( (x>=TRICKYSECT) ? \
                           ( ((x-TRICKYSECT ) >> TRICKYLSB) +TRICKYNUM) : \
                           ( ((x-STARTSECTOR) >> STARTLSB ) +STARTNUM ) )

#define STARTADDR  0x00010000
#define ERASE_SECT_START 9
#define ERASE_SECT_STOP  26

/* Sector Lookup */

/* Flash Programming Crapola */

void (*iap_fn)(unsigned int[],unsigned int[])=(void*)0x7ffffff1;

unsigned int prep_command[5]={50,11,11,0,0};
/* write command[4] assumes that the clock rate is 60MHz */
unsigned int write_command[5]={51,0,0,0,60000};
unsigned int erase_command[5]={52,ERASE_SECT_START,ERASE_SECT_STOP,60000};
unsigned int result[2];

/* readbuf MUST be on a word boundary */
char readbuf[READBUFSIZE];

/* Notes:
 *  Two commands needed for writing to flash:
 *   50 - "Prepare Sector", params: start sector, end sector
 *   51 - "Copy ram to flash", params: flash addr, ram addr, length, cclk in khz
 *   52 - "Erase Sectors", params: start sect, end sect, cclk in khz
 *
 *   The sequence is prepare current sector, write bytes.
 *   Each write (which is small than a whole sector) will write-protect
 *   the sector, so each write must be preceeded by a prepare
 */

/* End FPC */

int load_fw(char* filename)
{
    struct fat16_file_struct * fd;
    int read;
    int i;
    char* addy;
    addy = (char*)STARTADDR;

    /* Erase all sectors we could use */
    prep_command[1]=ERASE_SECT_START;
    prep_command[2]=ERASE_SECT_STOP;
    iap_fn(prep_command,result);
    iap_fn(erase_command,result);

    /* Open the file */
    fd = root_open(filename);

    /* Clear the buffer */
    for(i=0;i<READBUFSIZE;i++)
    {
        readbuf[i]=0;
    }

    /* Read the file contents, and print them out */
    while( (read=fat16_read_file(fd,(unsigned char*)readbuf,READBUFSIZE)) > 0 )
    {

        // Print Data to UART (DEBUG)
        /*for(i=0; i<read; i++)
        {
            rprintf("%c",readbuf[i]);
        }*/

        /* Write Data to Flash */

        /* Prepare Current Sector */
        /* This assumes that we are always only writing to one sector!
                     * This is only true if our write size necessarily aligns
                     * on proper boundaries. Be careful */
        prep_command[1] = SECTOR_NUMBER(((int)addy));
        prep_command[2] = prep_command[1];
        iap_fn(prep_command,result);


        /* *** Should check result here... but I'm not */
        /* If all went according to plan, the sector is primed for write
                     * (or erase)
                     */


        /* Now write data */
        write_command[1]=(unsigned int)addy;
        write_command[2]=(unsigned int)readbuf;
        write_command[3]=READBUFSIZE;
        iap_fn(write_command,result);

        /* *** Should check result here... but I'm not */
        /* If all went according to plan, data is in flash,
                     * and the sector is locked again
                     */


        /* Done with current data, so clear buffer
                     *  this is because we ALWAYS write out the
                     *  entire buffer, and we don't want to write
                     *  garbage data on reads smaller than READBUFSIZE
                     */
        for(i=0;i<READBUFSIZE;i++)
        {
            readbuf[i]=0;
        }


        /* Now update the address-- */
        addy = addy + READBUFSIZE;

        /* And we should probably bounds-check... *SIGH*/
        if((unsigned int)addy > (unsigned int) 0x0007CFFF)
        {
            break;
        }

    }

    /* All data copied to FLASH */
    /* Debug: Report the flash contents */
    /*  addy = (char*)STARTADDR;*/

    /*  PRINTF0("Dirty Screech\n\r");*/
    /*  delay_ms(10000);*/
    /*  while(addy<(char*)0x00050000)*/
    /*  {*/
    /*    __putchar(*addy++);*/
    /*  }*/
    /*  PRINTF0("Dirty Screech completed\n\r");*/


    /* Close the file! */
    sd_raw_sync();
    fat16_close_file(fd);
    root_delete(filename);

    return 0;
}

void call_firmware(void)
{
    /* Note that we're calling a routine that *SHOULD*
           * re-init the stack... so this function should never return...
           */
    void(*fncall)(void)=(void*)STARTADDR;
    fncall();

}
