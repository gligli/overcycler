/*****************************************************************************\
*              efs - General purpose Embedded Filesystem library              *
*          --------------------- -----------------------------------          *
*                                                                             *
* Filename : lpc2000_spi.c                                                     *
* Description : This  contains the functions needed to use efs for        *
*               accessing files on an SD-card connected to an LPC2xxx.        *
*                                                                             *
* This library is free software; you can redistribute it and/or               *
* modify it under the terms of the GNU Lesser General Public                  *
* License as published by the Free Software Foundation; either                *
* version 2.1 of the License, or (at your option) any later version.          *
*                                                                             *
* This library is distributed in the hope that it will be useful,             *
* but WITHOUT ANY WARRANTY; without even the implied warranty of              *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU           *
* Lesser General Public License for more details.                             *
*                                                                             *
*                                                    (c)2005 Martin Thomas    *
*                                                                             *
\*****************************************************************************/

/*
    2006, Bertrik Sikken, modified for LPCUSB
*/


/*****************************************************************************/
#include "type.h"
#include <stdio.h>
#include "rprintf.h"
#include "LPC214x.h"
#include "spi.h"
/*****************************************************************************/

// SP0SPCR  Bit-Definitions
#define CPHA    3
#define CPOL    4
#define MSTR    5
// SP0SPSR  Bit-Definitions
#define SPIF    7

/*****************************************************************************/

/*****************************************************************************/

// Utility-functions which does not toogle CS.
// Only needed during card-init. During init
// the automatic chip-select is disabled for SSP

static U8 my_SPISend(U8 outgoing)
{
    S0SPDR = outgoing;
    while (!(S0SPSR & (1 << SPIF)));
    return S0SPDR;
}

/*****************************************************************************/

void SPISetSpeed(U8 speed)
{
    speed &= 0xFE;
    if (speed < SPI_PRESCALE_MIN)
    {
        speed = SPI_PRESCALE_MIN;
    }
    SPI_PRESCALE_REG = speed;
}


void SPIInit(void)
{
    U8 i;
    //U32 j;

    rprintf("spiInit for SPI(0)\n");

    // setup GPIO
    PINSEL2 = 0;

	SPI_IODIR |= (1 << SPI_SCK_PIN) | (1 << SPI_MOSI_PIN);
    SPI_SS_IODIR |= (1 << SPI_SS_PIN);
    SPI_IODIR &= ~(1 << SPI_MISO_PIN);

    // reset Pin-Functions
    SPI_PINSEL &= ~((3 << SPI_SCK_FUNCBIT) | (3 << SPI_MISO_FUNCBIT) | (3 << SPI_MOSI_FUNCBIT));
    SPI_PINSEL |= ((1 << SPI_SCK_FUNCBIT) | (1 << SPI_MISO_FUNCBIT) | (1 << SPI_MOSI_FUNCBIT));

    // set Chip-Select high - unselect card
    UNSELECT_CARD();

    // enable SPI-Master
    S0SPCR = (1 << MSTR) | (0 << CPOL);   // TODO: check CPOL

    // low speed during init
    SPISetSpeed(254);

    /* Send 20 spi commands with card not selected */
    for (i = 0; i < 21; i++)
    {
        my_SPISend(0xff);
    }
}

/*****************************************************************************/

/*****************************************************************************/

U8 SPISend(U8 outgoing)
{
    U8 incoming;

    SELECT_CARD();
    S0SPDR = outgoing;
    while (!(S0SPSR & (1 << SPIF)));
    incoming = S0SPDR;
    UNSELECT_CARD();

    return incoming;
}

void SPISendN(U8 * pbBuf, int iLen)
{
    int i;

    SELECT_CARD();
    for (i = 0; i < iLen; i++)
    {
        S0SPDR = pbBuf[i];
        while (!(S0SPSR & (1 << SPIF)));
    }
    UNSELECT_CARD();
}

void SPIRecvN(U8 * pbBuf, int iLen)
{
    int i;

    SELECT_CARD();
    for (i = 0; i < iLen; i++)
    {
        S0SPDR = 0xFF;
        while (!(S0SPSR & (1 << SPIF)));
        pbBuf[i] = S0SPDR;
    }
    UNSELECT_CARD();
}

/*****************************************************************************/
