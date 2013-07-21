/*
    LPCUSB, an USB device driver for LPC microcontrollers
    Copyright (C) 2006 Bertrik Sikken (bertrik@sikken.nl)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
    This is the SCSI layer of the USB mass storage application example.
    This layer depends directly on the blockdev layer.
*/


#include <string.h>     // memcpy

#include "type.h"
#include "usbdebug.h"
#include <stdio.h>
#include "rprintf.h"

#include "blockdev.h"
#include "msc_scsi.h"

#undef MIN
#define MIN(x,y)	((x)<(y)?(x):(y))	/**< MIN */

#define BLOCKSIZE       512

// SCSI commands
#define SCSI_CMD_TEST_UNIT_READY    0x00
#define SCSI_CMD_REQUEST_SENSE      0x03
#define SCSI_CMD_INQUIRY            0x12
#define SCSI_CMD_READ_CAPACITY      0x25
#define SCSI_CMD_READ_10            0x28
#define SCSI_CMD_WRITE_10           0x2A

// sense code
#define WRITE_ERROR             0x030C00
#define READ_ERROR              0x031100
#define INVALID_CMD_OPCODE      0x052000
#define INVALID_FIELD_IN_CDB    0x052400

//  Sense code, which is set on error conditions
static U32          dwSense;    // hex: 00aabbcc, where aa=KEY, bb=ASC, cc=ASCQ

static const U8     abInquiry[] =
{
    0x00,       // PDT = direct-access device
    0x80,       // removeable medium bit = set
    0x04,       // version = complies to SPC2r20
    0x02,       // response data format = SPC2r20
    0x1F,       // additional length
    0x00,
    0x00,
    0x00,
    'L','P','C','U','S','B',' ',' ',    // vendor
    'M','a','s','s',' ','s','t','o',    // product
    'r','a','g','e',' ',' ',' ',' ',
    '0','.','1',' '                     // revision
};

//  Data for "request sense" command. The 0xFF are filled in later
static const U8 abSense[] =
{
    0x70, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x0A,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00
};

//  Buffer for holding one block of disk data
static U8 abBlockBuf[512];


typedef struct
{
    U8      bOperationCode;
    U8      abLBA[3];
    U8      bLength;
    U8      bControl;
}
TCDB6;


/*************************************************************************
    SCSIReset
    =========
        Resets any SCSI state

**************************************************************************/
void SCSIReset(void)
{
    dwSense = 0;
}


/*************************************************************************
    SCSIHandleCmd
    =============
        Verifies a SCSI CDB and indicates the direction and amount of data
        that the device wants to transfer.

    If this call fails, a sense code is set in dwSense.

    IN      pbCDB       Command data block
            iCDBLen     Command data block len
    OUT     *piRspLen   Length of intended response data:
            *pfDevIn    TRUE if data is transferred from device-to-host

    Returns a pointer to the data exchange buffer if successful,
    return NULL otherwise.
**************************************************************************/
U8 * SCSIHandleCmd(U8 *pbCDB, int iCDBLen, int *piRspLen, BOOL *pfDevIn)
{
    int     i;
    TCDB6   *pCDB;
    U32     dwLen, dwLBA;

	//pCDB = (TCDB6 *)pbCDB;
	//Compiler warning fix
    TCDB6 cdb;
	pCDB = &cdb;
	memcpy(pCDB, pbCDB, sizeof(TCDB6));
	
    // default direction is from device to host
    *pfDevIn = TRUE;

    switch (pCDB->bOperationCode)
    {

        // test unit ready (6)
        case SCSI_CMD_TEST_UNIT_READY:
            DBG("TEST UNIT READY\n");
            *piRspLen = 0;
            break;

        // request sense (6)
        case SCSI_CMD_REQUEST_SENSE:
            DBG("REQUEST SENSE (%06X)\n", dwSense);
            // check params
            *piRspLen = MIN(18, pCDB->bLength);
            break;

        // inquiry (6)
        case SCSI_CMD_INQUIRY:
            DBG("INQUIRY\n");
            // see SPC20r20, 4.3.4.6
            *piRspLen = MIN(36, pCDB->bLength);
            break;

        // read capacity (10)
        case SCSI_CMD_READ_CAPACITY:
            DBG("READ CAPACITY\n");
            *piRspLen = 8;
            break;

        // read (10)
        case SCSI_CMD_READ_10:
            if (iCDBLen != 10)
            {
                return NULL;
            }
            dwLBA = (pbCDB[2] << 24) | (pbCDB[3] << 16) | (pbCDB[4] << 8) | (pbCDB[5]);
            dwLen = (pbCDB[7] << 8) | pbCDB[8];
            DBG("READ10, LBA=%d, len=%d\n", dwLBA, dwLen);
            *piRspLen = dwLen * BLOCKSIZE;
            break;

        // write (10)
        case SCSI_CMD_WRITE_10:
            if (iCDBLen != 10)
            {
                return NULL;
            }
            dwLBA = (pbCDB[2] << 24) | (pbCDB[3] << 16) | (pbCDB[4] << 8) | (pbCDB[5]);
            dwLen = (pbCDB[7] << 8) | pbCDB[8];
            DBG("WRITE10, LBA=%d, len=%d\n", dwLBA, dwLen);
            *piRspLen = dwLen * BLOCKSIZE;
            *pfDevIn = FALSE;
            break;

        default:
            DBG("Unhandled SCSI: ");
            for (i = 0; i < iCDBLen; i++)
            {
                DBG(" %02X", pbCDB[i]);
            }
            DBG("\n");
            // unsupported command
            dwSense = INVALID_CMD_OPCODE;
            *piRspLen = 0;
            return NULL;
        }


    return abBlockBuf;
}


/*************************************************************************
    SCSIHandleData
    ==============
        Handles a block of SCSI data.

    IN      pbCDB       Command data block
            iCDBLen     Command data block len
    IN/OUT  pbData      Data buffer
    IN      dwOffset    Offset in data

    Returns a pointer to the next data to be exchanged if successful,
    returns NULL otherwise.
**************************************************************************/
U8 * SCSIHandleData(U8 *pbCDB, int iCDBLen, U8 *pbData, U32 dwOffset)
{
    TCDB6   *pCDB;
    U32     dwLBA;
    U32     dwBufPos, dwBlockNr;
    U32     dwNumBlocks, dwMaxBlock;

	//pCDB = (TCDB6 *)pbCDB;
	//Compiler warning fix
    TCDB6 cdb;
	pCDB = &cdb;
	memcpy(pCDB, pbCDB, sizeof(TCDB6));
	
    switch (pCDB->bOperationCode)
    {

        // test unit ready
        case 0x00:
            if (dwSense != 0)
            {
                return NULL;
            }
            break;

        // request sense
        case SCSI_CMD_REQUEST_SENSE:
            memcpy(pbData, abSense, 18);
            // fill in KEY/ASC/ASCQ
            pbData[2] = (dwSense >> 16) & 0xFF;
            pbData[12] = (dwSense >> 8) & 0xFF;
            pbData[13] = (dwSense >> 0) & 0xFF;
            // reset sense data
            dwSense = 0;
            break;

        // inquiry
        case SCSI_CMD_INQUIRY:
            memcpy(pbData, abInquiry, sizeof(abInquiry));
            break;

        // read capacity
        case SCSI_CMD_READ_CAPACITY:
        // get size of drive (bytes)
            BlockDevGetSize(&dwNumBlocks);
            // calculate highest LBA
            dwMaxBlock = (dwNumBlocks - 1) / 512;

            pbData[0] = (dwMaxBlock >> 24) & 0xFF;
            pbData[1] = (dwMaxBlock >> 16) & 0xFF;
            pbData[2] = (dwMaxBlock >> 8) & 0xFF;
            pbData[3] = (dwMaxBlock >> 0) & 0xFF;
            pbData[4] = (BLOCKSIZE >> 24) & 0xFF;
            pbData[5] = (BLOCKSIZE >> 16) & 0xFF;
            pbData[6] = (BLOCKSIZE >> 8) & 0xFF;
            pbData[7] = (BLOCKSIZE >> 0) & 0xFF;
            break;

        // read10
        case SCSI_CMD_READ_10:
            dwLBA = (pbCDB[2] << 24) | (pbCDB[3] << 16) | (pbCDB[4] << 8) | (pbCDB[5]);

            // copy data from block buffer
            dwBufPos = (dwOffset & (BLOCKSIZE - 1));
            if (dwBufPos == 0)
            {
                // read new block
                dwBlockNr = dwLBA + (dwOffset / BLOCKSIZE);
                DBG("R");
                if (BlockDevRead(dwBlockNr, abBlockBuf) < 0)
                {
                    dwSense = READ_ERROR;
                    DBG("BlockDevRead failed\n");
                    return NULL;
                }
            }
            // return pointer to data
            return abBlockBuf + dwBufPos;

            // write10
            case SCSI_CMD_WRITE_10:
            dwLBA = (pbCDB[2] << 24) | (pbCDB[3] << 16) | (pbCDB[4] << 8) | (pbCDB[5]);

            // copy data to block buffer
            dwBufPos = ((dwOffset + 64) & (BLOCKSIZE - 1));
            if (dwBufPos == 0)
            {
                // write new block
                dwBlockNr = dwLBA + (dwOffset / BLOCKSIZE);
                DBG("W");
                if (BlockDevWrite(dwBlockNr, abBlockBuf) < 0)
                {
                    dwSense = WRITE_ERROR;
                    DBG("BlockDevWrite failed\n");
                    return NULL;
                }
            }
            // return pointer to next data
            return abBlockBuf + dwBufPos;

            default:
            // unsupported command
            dwSense = INVALID_CMD_OPCODE;
            return NULL;
        }

    // default: return pointer to start of block buffer
    return abBlockBuf;
}


