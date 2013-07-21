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


#include "main_msc.h"
#include "type.h"

#include <stdio.h>
#include "rprintf.h"

#include "LPC214x.h"
#include "usbapi.h"
#include "usbdebug.h"

#include "msc_bot.h"
#include "blockdev.h"

#define BAUD_RATE   115200

#define MAX_PACKET_SIZE 64

#define LE_WORD(x)      ((x)&0xFF),((x)>>8)

static U8 abClassReqData[4];

//static const U8 abDescriptors[] =
static U8 abDescriptors[] =
{

    // device descriptor
    0x12,
    DESC_DEVICE,
    LE_WORD(0x0200),        // bcdUSB
    0x00,                   // bDeviceClass
    0x00,                   // bDeviceSubClass
    0x00,                   // bDeviceProtocol
    MAX_PACKET_SIZE0,       // bMaxPacketSize
    LE_WORD(0x1b4f),        // idVendor
    LE_WORD(0x0001),        // idProduct
    LE_WORD(0x0100),        // bcdDevice
    0x01,                   // iManufacturer
    0x02,                   // iProduct
    0x03,                   // iSerialNumber
    0x01,                   // bNumConfigurations

    // configuration descriptor
    0x09,
    DESC_CONFIGURATION,
    LE_WORD(32),            // wTotalLength
    0x01,                   // bNumInterfaces
    0x01,                   // bConfigurationValue
    0x00,                   // iConfiguration
    0xC0,                   // bmAttributes
    0x32,                   // bMaxPower

    // interface
    0x09,
    DESC_INTERFACE,
    0x00,                   // bInterfaceNumber
    0x00,                   // bAlternateSetting
    0x02,                   // bNumEndPoints
    0x08,                   // bInterfaceClass = mass storage
    0x06,                   // bInterfaceSubClass = transparent SCSI
    0x50,                   // bInterfaceProtocol = BOT
    0x00,                   // iInterface
    // EP
    0x07,
    DESC_ENDPOINT,
    MSC_BULK_IN_EP,         // bEndpointAddress
    0x02,                   // bmAttributes = bulk
    LE_WORD(MAX_PACKET_SIZE),// wMaxPacketSize
    0x00,                   // bInterval
    // EP
    0x07,
    DESC_ENDPOINT,
    MSC_BULK_OUT_EP,        // bEndpointAddress
    0x02,                   // bmAttributes = bulk
    LE_WORD(MAX_PACKET_SIZE),// wMaxPacketSize
    0x00,                   // bInterval

    // string descriptors
    0x04,
    DESC_STRING,
    LE_WORD(0x0409),

    0x14,
    DESC_STRING,
    'I', 0, 'N', 0, '2', 0, 'R', 0, 'o', 0, 'w', 0, 'i', 0, 'n', 0, 'g', 0,

    0x10,
    DESC_STRING,
    'D', 0, 'a', 0, 't', 0, 'a', 0, 'L', 0, 'o', 0, 'g', 0,

    0x1A,
    DESC_STRING,
    '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0,

    // terminating zero
    0
};


/*************************************************************************
    HandleClassRequest
    ==================
        Handle mass storage class request

**************************************************************************/
static BOOL HandleClassRequest(TSetupPacket *pSetup, int *piLen, U8 **ppbData)
{
    if (pSetup->wIndex != 0)
    {
        DBG("Invalid idx %X\n", pSetup->wIndex);
        return FALSE;
    }
    if (pSetup->wValue != 0)
    {
        DBG("Invalid val %X\n", pSetup->wValue);
        return FALSE;
    }

    switch (pSetup->bRequest)
    {

        // get max LUN
        case 0xFE:
            *ppbData[0] = 0;        // No LUNs
            *piLen = 1;
            break;

        // MSC reset
        case 0xFF:
            if (pSetup->wLength > 0)
            {
                return FALSE;
            }
            MSCBotReset();
            break;

        default:
            DBG("Unhandled class\n");
            return FALSE;
        }
    return TRUE;
}


/*************************************************************************
    msc_main
    ====
**************************************************************************/
int main_msc(void)
{
    // initialise the SD card
    BlockDevInit();

    rprintf("Initialising USB stack\n");

    // initialise stack
    USBInit();

    // enable bulk-in interrupts on NAKs
    // these are required to get the BOT protocol going again after a STALL
    USBHwNakIntEnable(INACK_BI);

    // register descriptors
    USBRegisterDescriptors(abDescriptors);

    // register class request handler
    USBRegisterRequestHandler(REQTYPE_TYPE_CLASS, HandleClassRequest, abClassReqData);

    // register endpoint handlers
    USBHwRegisterEPIntHandler(MSC_BULK_IN_EP, MSCBotBulkIn);
    USBHwRegisterEPIntHandler(MSC_BULK_OUT_EP, MSCBotBulkOut);

    rprintf("Starting USB communication\n");
	
    // connect to bus
    USBHwConnect(TRUE);

    // call USB interrupt handler continuously
    while (IOPIN0 & (1<<23))
	{
		USBHwISR();
    }

    return 0;
}

