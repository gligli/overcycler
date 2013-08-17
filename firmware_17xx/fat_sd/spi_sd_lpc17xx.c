/*-----------------------------------------------------------------------*/
/* MMC/SDSC/SDHC (in SPI mode) control module for LPC17xx Version 0.8.0  */
/*-----------------------------------------------------------------------*/

/*
  Basic MMC/SD-access code: ChaN
  LPC2xxx SSP-Fifo-Suppot: Mike Anton
  LPC17xx driver: Martin Thomas
*/

/* Copyright (c) 2007, 2010, ChaN, Martin Thomas, Mike Anton
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.


  Yes, this license also means you. Even if you are a developer of
  NXP application-notes and source-code. Just removing some comments,
  renaming functions and and moving some lines around is not enough to
  get rid of this license.
*/


#include "lpc_types.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_gpdma.h"

#include "integer.h"
#include "spi_sd_lpc17xx.h"

/// #include "monitor.h"

/* available modes: */
#define SPI_SD_USE_POLLING  0
#define SPI_SD_USE_FIFO     1
#define SPI_SD_USE_DMA      2

/* used mode : */
#define SPI_SD_ACCESS_MODE  SPI_SD_USE_FIFO

// For Olimex LPC1766-STK
// MMC_PWR (P-Channel FET): P0.21
// SSEL1: P0.6, SCK1: P0.7, MISO1: P0.8, MOSI1: P0.9
#define SOCKET_POWER_PORT        0
#define SOCKET_POWER_PIN        21
#define SOCKET_POWER_MASK       (1 << SOCKET_POWER_PIN)
#define SOCKET_POWER_OPENDRAIN  PINSEL_PINMODE_OPENDRAIN /* 33k pull-up mounted */

/* used SSP-port: */
#define CARD_SSP                 1

/*--------------------------------------------------------------------------
   Module Private Functions and Variables
---------------------------------------------------------------------------*/

/* Definitions for MMC/SDC command */
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND (MMC) */
#define ACMD41	(0xC0+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(0x40+8)	/* SEND_IF_COND */
#define CMD9	(0x40+9)	/* SEND_CSD */
#define CMD10	(0x40+10)	/* SEND_CID */
#define CMD12	(0x40+12)	/* STOP_TRANSMISSION */
#define ACMD13	(0xC0+13)	/* SD_STATUS (SDC) */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD18	(0x40+18)	/* READ_MULTIPLE_BLOCK */
#define CMD23	(0x40+23)	/* SET_BLOCK_COUNT (MMC) */
#define ACMD23	(0xC0+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD25	(0x40+25)	/* WRITE_MULTIPLE_BLOCK */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */

#if ( SPI_SD_ACCESS_MODE == SPI_SD_USE_DMA )

enum dma_direction_ { MEM_TO_CARD, CARD_TO_MEM };

#define DMA_CHANNEL_TX          0
#define DMA_CHANNEL_TX_HANDLE   LPC_GPDMACH0
#define DMA_CHANNEL_RX          1
#define DMA_CHANNEL_RX_HANDLE   LPC_GPDMACH1

#define DMA_DUMMY_SIZE 512
#if USE_DMA_DUMMY_RAM
static
#else
static const
#endif /* USE_DMA_DUMMY_RAM */
BYTE dma_dummy[DMA_DUMMY_SIZE] = {
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
#endif /* USE_DMA */


static const DWORD socket_state_mask_cp = (1 << 0);
static const DWORD socket_state_mask_wp = (1 << 1);

static volatile DSTATUS Stat = STA_NOINIT;  /* Disk status */

static volatile DWORD Timer1, Timer2;       /* 100Hz decrement timers */

static BYTE CardType;                       /* Card type flags */

static BYTE socket_powered;

enum speed_setting { INTERFACE_SLOW, INTERFACE_FAST };

/*-----------------------------------------------------------------------*/
/* socket control low-level functions                                    */
/*-----------------------------------------------------------------------*/

static inline uint32_t socket_is_empty(void)
{
	return 0; // not connected on LPC1766-STK -> fake inserted
}

static inline uint32_t socket_is_write_protected(void)
{
	return 0; // not connected on LPC1766-STK -> fake not protected
}

static void socket_power_on()
{
	GPIO_ClearValue(SOCKET_POWER_PORT, SOCKET_POWER_MASK);
	socket_powered = 1;
}

static void socket_power_off()
{
	GPIO_SetValue(SOCKET_POWER_PORT, SOCKET_POWER_MASK);
	socket_powered = 0;
}

static BYTE socket_is_powered()
{
	return socket_powered;
}


static void socket_init()
{
	PINSEL_CFG_Type PinCfg;

	// Power P-Channel FET
	PinCfg.Funcnum   = 0;
	PinCfg.OpenDrain = SOCKET_POWER_OPENDRAIN;
	PinCfg.Pinmode   = 0;
	PinCfg.Pinnum    = SOCKET_POWER_PIN;
	PinCfg.Portnum   = SOCKET_POWER_PORT;
	PINSEL_ConfigPin(&PinCfg);
	socket_power_off();
	socket_powered = 0;
	GPIO_SetDir(SOCKET_POWER_PORT, SOCKET_POWER_MASK, 1);

	// card-present switch
	// n/a

	// write-protect switch
	// n/a
}


/*-----------------------------------------------------------------------*/
/* SPI low-level functions                                               */
/*-----------------------------------------------------------------------*/
#if ( CARD_SSP == 1 )

static inline void select_card()
{
	// SSEL1 P0.6 low
	GPIO_ClearValue(0, (1 << 6)); // LPC_GPIO0->FIOCLR = (1<<6);
}

static inline void de_select_card()
{
	// SSEL1 high
	GPIO_SetValue(0, (1 << 6)); // LPC_GPIO0->FIOSET = (1<<6);
}

static void spi_set_speed( enum speed_setting speed )
{
	if ( speed == INTERFACE_SLOW ) {
		SSP_SetClock(LPC_SSP1, 400000);
	} else {
		SSP_SetClock(LPC_SSP1, 25000000);
	}
}

static void spi_init(void)
{
	PINSEL_CFG_Type PinCfg;
	SSP_CFG_Type SSP_ConfigStruct;

	// SSEL1 P0.6 as GPIO, pull-up mounted but driven push/pull here
	PinCfg.Funcnum   = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode   = PINSEL_PINMODE_PULLUP;
	PinCfg.Pinnum    = 6;
	PinCfg.Portnum   = 0;
	de_select_card();
	GPIO_SetDir(0, (1 << 6), 1);
	PINSEL_ConfigPin(&PinCfg);
	// SCK P0.7 alternate function 0b10
	PinCfg.Funcnum   = PINSEL_FUNC_2;
	PinCfg.Pinnum    = 7;
	PINSEL_ConfigPin(&PinCfg);
	// MISO P0.8
	PinCfg.Pinnum    = 8;
	PINSEL_ConfigPin(&PinCfg);
	// MOSI P0.9
	PinCfg.Pinnum    = 9;
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_SSP1, CLKPWR_PCLKSEL_CCLK_DIV_2);

	SSP_Cmd(LPC_SSP1, ENABLE);

	/* wait for busy gone */
	while( LPC_SSP1->SR & SSP_SR_BSY ) { ; }

	/* drain SPI RX FIFO */
	while( LPC_SSP1->SR & SSP_SR_RNE ) {
		volatile uint32_t dummy = LPC_SSP0->DR;
		(void)dummy;
	}

#if ( SPI_SD_ACCESS_MODE == SPI_SD_USE_DMA )
	GPDMA_Init();
	LPC_GPDMA->DMACConfig = 0x01;
#endif
}

void spi_close(void)
{
	PINSEL_CFG_Type PinCfg;

	SSP_Cmd(LPC_SSP1, DISABLE);
	SSP_DeInit(LPC_SSP1);

	PinCfg.Funcnum   = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode   = PINSEL_PINMODE_PULLDOWN;
	PinCfg.Pinnum    = 6;
	PinCfg.Portnum   = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum    = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum    = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum    = 9;
	PINSEL_ConfigPin(&PinCfg);
}

static inline BYTE spi_rw( BYTE out )
{
	BYTE in;

	LPC_SSP1->DR = out;
	while (LPC_SSP1->SR & SSP_SR_BSY ) { ; }
	in = LPC_SSP1->DR;

	///xprintf("SPIRW O:0x%02x I:0x%02x\n", out, in);

	return in;
}

#define xmit_spi(dat)  spi_rw(dat)

static inline BYTE rcvr_spi(void)
{
	return spi_rw(0xff);
}

/* Alternative macro to receive data fast */
#define rcvr_spi_m(dst)  *(dst)=spi_rw(0xff)


#if ( SPI_SD_ACCESS_MODE == SPI_SD_USE_DMA )

/* reminders:
 * Peripheral Function SSP1_Tx Connection: 2
 * Peripheral Function SSP1_Rx Connection: 3
 */


volatile uint32_t dmacb_rx_tc;
volatile uint32_t dmacb_rx_error;

void DMA_IRQHandler(void)
{
	GPDMA_IntHandler(); // call the default handler
}

void GPDMA_callback_rx(uint32_t DMA_Status)
{
	if( DMA_Status & GPDMA_STAT_INTTC ) {
		dmacb_rx_tc++; // RX DMA terminated
	}
	if ( DMA_Status & GPDMA_STAT_INTERR ) {
		dmacb_rx_error++;
	}
}

static void dma_transfer(
	BYTE dir,          /* MEM_TO_CARD or CARD_TO_MEM                          */
	const BYTE *buff,  /* TO_CARD       : 512 byte data block to be transmitted
	                      FROM_CARD     : Data buffer to store received data    */
	UINT btr           /* TO_CARD       : Byte count (must be multiple of 2)
	                      FROM_CARD     : Byte count (must be 512)              */
)
{
	GPDMA_Channel_CFG_Type GPDMACfg;
	char dummy2[512]; // TODO: get rid of this once DI can be disabled

	if ( dir == MEM_TO_CARD ) {

		GPDMACfg.ChannelNum = DMA_CHANNEL_TX;
		GPDMACfg.SrcMemAddr = (uint32_t)buff;
		GPDMACfg.DstMemAddr = 0;
		GPDMACfg.TransferSize = btr;
		GPDMACfg.TransferWidth = 0;
		GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
		GPDMACfg.SrcConn = 0;
		GPDMACfg.DstConn = GPDMA_CONN_SSP1_Tx;
		GPDMACfg.DMALLI = 0;
		GPDMA_Setup( &GPDMACfg, NULL );

		GPDMACfg.ChannelNum = DMA_CHANNEL_RX;
		GPDMACfg.SrcMemAddr = 0;
		GPDMACfg.DstMemAddr = (uint32_t)dummy2; // TODO: get rid of this once DI can be disabled
		GPDMACfg.TransferSize = btr;
		GPDMACfg.TransferWidth = 0;
		GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_P2M;
		GPDMACfg.SrcConn = GPDMA_CONN_SSP1_Rx;
		GPDMACfg.DstConn = 0;
		GPDMACfg.DMALLI = 0;
		GPDMA_Setup( &GPDMACfg, GPDMA_callback_rx );
		// TODO: disable destination increment - does not work yet, terminate interrupt never fires when set
		// DMA_CHANNEL_RX_HANDLE->DMACCControl &= ~GPDMA_DMACCxControl_DI;

	} else {

		GPDMACfg.ChannelNum = DMA_CHANNEL_TX;
		GPDMACfg.SrcMemAddr = (uint32_t)dma_dummy; // TODO: get rid of this once SI can be disabled
		GPDMACfg.DstMemAddr = 0;
		GPDMACfg.TransferSize = btr;
		GPDMACfg.TransferWidth = 0;
		GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
		GPDMACfg.SrcConn = 0;
		GPDMACfg.DstConn = GPDMA_CONN_SSP1_Tx;
		GPDMACfg.DMALLI = 0;
		GPDMA_Setup( &GPDMACfg, NULL );
		// TODO: disable source increment - does not work yet, terminate interrupt never fires when set
		// DMA_CHANNEL_TX_HANDLE->DMACCControl &= ~GPDMA_DMACCxControl_SI;

		GPDMACfg.ChannelNum = DMA_CHANNEL_RX;
		GPDMACfg.SrcMemAddr = 0;
		GPDMACfg.DstMemAddr = (uint32_t)buff;
		GPDMACfg.TransferSize = btr;
		GPDMACfg.TransferWidth = 0;
		GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_P2M;
		GPDMACfg.SrcConn = GPDMA_CONN_SSP1_Rx;
		GPDMACfg.DstConn = 0;
		GPDMACfg.DMALLI = 0;
		GPDMA_Setup( &GPDMACfg, GPDMA_callback_rx );

	}

	dmacb_rx_tc = 0;
	dmacb_rx_error = 0;

	GPDMA_ChannelCmd( DMA_CHANNEL_TX, ENABLE );
	GPDMA_ChannelCmd( DMA_CHANNEL_RX, ENABLE );

	NVIC_EnableIRQ( DMA_IRQn );

	SSP_DMACmd( LPC_SSP1, SSP_DMA_RX, ENABLE);
	SSP_DMACmd( LPC_SSP1, SSP_DMA_TX, ENABLE);

	while ( dmacb_rx_tc == 0 ) { ; }

	SSP_DMACmd( LPC_SSP1, SSP_DMA_TX, DISABLE );
	SSP_DMACmd( LPC_SSP1, SSP_DMA_RX, DISABLE );

	NVIC_DisableIRQ( DMA_IRQn );

	GPDMA_ChannelCmd( DMA_CHANNEL_TX, DISABLE );
	GPDMA_ChannelCmd( DMA_CHANNEL_RX, DISABLE );
}

#elif ( SPI_SD_ACCESS_MODE == SPI_SD_USE_FIFO )

#define FIFO_ELEM 8 /* "8 frame FIFOs for both transmit and receive.*/

static inline void spi_rcvr_block (
	BYTE *buff,         /* Data buffer to store received data */
	UINT btr            /* Byte count (must be multiple of 4) */
)
{
	UINT hwtr, startcnt, i, rec;

	hwtr = btr/2;
	if ( btr < FIFO_ELEM ) {
		startcnt = hwtr;
	} else {
		startcnt = FIFO_ELEM;
	}

	LPC_SSP1->CR0 |= SSP_CR0_DSS(16); // DSS to 16 bit

	for ( i = startcnt; i; i-- ) {
		LPC_SSP1->DR = 0xffff;  // fill TX FIFO
	}

	do {
		while ( !(LPC_SSP1->SR & SSP_SR_RNE ) ) {
			// wait for data in RX FIFO (RNE set)
		}
		rec = LPC_SSP1->DR;
		if ( i < ( hwtr - startcnt ) ) {
			LPC_SSP1->DR = 0xffff;
		}
		*buff++ = (BYTE)(rec >> 8);
		*buff++ = (BYTE)(rec);
		i++;
	} while ( i < hwtr );

	LPC_SSP1->CR0 = ( LPC_SSP1->CR0 & ~SSP_CR0_DSS(16) ) | SSP_CR0_DSS(8); // DSS to 8 bit
}

static inline void spi_xmit_block (
	const BYTE *buff    /* 512 byte data block to be transmitted */
)
{
	UINT cnt;
	WORD data;

	LPC_SSP1->CR0 |= SSP_CR0_DSS(16); // DSS to 16 bit

	for ( cnt = 0; cnt < ( 512 / 2 ); cnt++ ) {
		while ( !( LPC_SSP1->SR & SSP_SR_TNF ) ) {
			; // wait for TX FIFO not full (TNF)
		}
		data  = (*buff++) << 8;
		data |= *buff++;
		LPC_SSP1->DR = data;
	}

	while ( LPC_SSP1->SR & SSP_SR_BSY ) {
		// wait for BSY gone
	}
	while ( LPC_SSP1->SR & SSP_SR_RNE ) {
		data = LPC_SSP1->DR; // drain receive FIFO
	}

	LPC_SSP1->CR0 = ( LPC_SSP1->CR0 & ~SSP_CR0_DSS(16) ) | SSP_CR0_DSS(8); // DSS to 8 bit
}

#else
	/* nothing here, polling-mode already integrated in base-functions */
#endif /* SPI_SD_ACCESS_MODE */

#else
#error "port not supported"
#endif /* CARD_SSP */


/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/
static BYTE wait_ready (void)
{
	BYTE res;

	Timer2 = 50;	/* Wait for ready in timeout of 500ms */
	rcvr_spi();
	do
		res = rcvr_spi();
	while ((res != 0xFF) && Timer2);

	return res;
}

/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/
static void release_spi (void)
{
	de_select_card();
	rcvr_spi();
}

/*-----------------------------------------------------------------------*/
/* Power up/down                                                         */
/*-----------------------------------------------------------------------*/
static void power_on()
{
	socket_init();
	socket_power_on();

	for (Timer1 = 25; Timer1; );	/* Wait for 250ms */

	spi_init();
	spi_set_speed(INTERFACE_SLOW);
	de_select_card();
}

static void power_off()
{
	if (!(Stat & STA_NOINIT)) {
		select_card();
		wait_ready();
		release_spi();
	}
	spi_close();
	socket_power_off();
	Stat |= STA_NOINIT;		/* Set STA_NOINIT */
}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from MMC                                        */
/*-----------------------------------------------------------------------*/
static Bool rcvr_datablock (
	BYTE *buff,			/* Data buffer to store received data */
	UINT btr			/* Byte count (must be multiple of 4) */
)
{
	BYTE token;


	Timer1 = 10;
	do {							/* Wait for data packet in timeout of 100ms */
		token = rcvr_spi();
	} while ((token == 0xFF) && Timer1);
	if(token != 0xFE) return FALSE;	/* If not valid data token, return with error */

#if ( SPI_SD_ACCESS_MODE == SPI_SD_USE_DMA )
	dma_transfer( CARD_TO_MEM, buff, btr );
#elif ( SPI_SD_ACCESS_MODE == SPI_SD_USE_FIFO )
	spi_rcvr_block( buff, btr );
#else
	do {							/* Receive the data block into buffer */
		rcvr_spi_m(buff++);
		rcvr_spi_m(buff++);
		rcvr_spi_m(buff++);
		rcvr_spi_m(buff++);
	} while (btr -= 4);
#endif /* SPI_SD_ACCESS_MODE */

	rcvr_spi();						/* Discard CRC */
	rcvr_spi();

	return TRUE;					/* Return with success */
}



/*-----------------------------------------------------------------------*/
/* Send a data packet to MMC                                             */
/*-----------------------------------------------------------------------*/
#if _FS_READONLY == 0
static Bool xmit_datablock (
	const BYTE *buff,	/* 512 byte data block to be transmitted */
	BYTE token			/* Data/Stop token */
)
{
	BYTE resp;
#if ( SPI_SD_ACCESS_MODE == SPI_SD_USE_POLLING )
	BYTE wc;
#endif

	if (wait_ready() != 0xFF) return FALSE;

	xmit_spi(token);					/* transmit data token */
	if (token != 0xFD) {	/* Is data token */

#if ( SPI_SD_ACCESS_MODE == SPI_SD_USE_DMA )
		dma_transfer( MEM_TO_CARD, buff, 512 );
#elif ( SPI_SD_ACCESS_MODE == SPI_SD_USE_FIFO )
		spi_xmit_block( buff );
#else
		wc = 0;
		do {							/* transmit the 512 byte data block to MMC */
			xmit_spi(*buff++);
			xmit_spi(*buff++);
		} while (--wc);
#endif /* SPI_SD_ACCESS_MODE */

		xmit_spi(0xFF);					/* CRC (Dummy) */
		xmit_spi(0xFF);
		resp = rcvr_spi();				/* Receive data response */
		if ((resp & 0x1F) != 0x05)		/* If not accepted, return with error */
			return FALSE;
	}

	return TRUE;
}
#endif /* _READONLY */

/*-----------------------------------------------------------------------*/
/* Send a command packet to MMC                                          */
/*-----------------------------------------------------------------------*/
static BYTE send_cmd (
	BYTE cmd,		/* Command byte */
	DWORD arg		/* Argument */
)
{
	BYTE n, res;


	if (cmd & 0x80) {	/* ACMD<n> is the command sequence of CMD55-CMD<n> */
		cmd &= 0x7F;
		res = send_cmd(CMD55, 0);
		if (res > 1) return res;
	}

	/* Select the card and wait for ready */
	de_select_card();
	select_card();
	if (wait_ready() != 0xFF) {
		return 0xFF;
	}

	/* Send command packet */
	xmit_spi(cmd);						/* Start + Command index */
	xmit_spi((BYTE)(arg >> 24));		/* Argument[31..24] */
	xmit_spi((BYTE)(arg >> 16));		/* Argument[23..16] */
	xmit_spi((BYTE)(arg >> 8));			/* Argument[15..8] */
	xmit_spi((BYTE)arg);				/* Argument[7..0] */
	n = 0x01;							/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	xmit_spi(n);

	/* Receive command response */
	if (cmd == CMD12) rcvr_spi();		/* Skip a stuff byte when stop reading */

	n = 10;								/* Wait for a valid response in timeout of 10 attempts */
	do
		res = rcvr_spi();
	while ((res & 0x80) && --n);

	return res;			/* Return with the response value */
}

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/
DSTATUS MMC_disk_initialize(void)
{
	BYTE n, cmd, ty, ocr[4];

	if (Stat & STA_NODISK) return Stat;	/* No card in the socket */

	power_on();							/* Force socket power on and initialize interface */
	spi_set_speed(INTERFACE_SLOW);
	for (n = 10; n; n--) rcvr_spi();	/* 80 dummy clocks with card de-selected */

	ty = 0;
	if (send_cmd(CMD0, 0) == 1) {			/* Enter Idle state */
		Timer1 = 100;						/* Initialization timeout of 1000 milliseconds */
		if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDHC */
			for (n = 0; n < 4; n++) ocr[n] = rcvr_spi();		/* Get trailing return value of R7 response */
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {				/* The card can work at VDD range of 2.7-3.6V */
				while (Timer1 && send_cmd(ACMD41, 1UL << 30));	/* Wait for leaving idle state (ACMD41 with HCS bit) */
				if (Timer1 && send_cmd(CMD58, 0) == 0) {		/* Check CCS bit in the OCR */
					for (n = 0; n < 4; n++) ocr[n] = rcvr_spi();
					ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;
				}
			}
		} else {							/* SDSC or MMC */
			if (send_cmd(ACMD41, 0) <= 1) 	{
				ty = CT_SD1; cmd = ACMD41;	/* SDSC */
			} else {
				ty = CT_MMC; cmd = CMD1;	/* MMC */
			}
			while (Timer1 && send_cmd(cmd, 0));			/* Wait for leaving idle state */
			if (!Timer1 || send_cmd(CMD16, 512) != 0)	/* Set R/W block length to 512 */
				ty = 0;
		}
	} else {
		// xprintf("cmd 0 failed\n");
	}
	CardType = ty;
	release_spi();

	if (ty) {			/* Initialization succeeded */
		Stat &= ~STA_NOINIT;		/* Clear STA_NOINIT */
		spi_set_speed(INTERFACE_FAST);
	} else {			/* Initialization failed */
		power_off();
	}

	return Stat;
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/
DSTATUS MMC_disk_status(void)
{
	return Stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT MMC_disk_read(
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..255) */
)
{
	if (!count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;

	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert to byte address if needed */

	if (count == 1) {	/* Single block read */
		if (send_cmd(CMD17, sector) == 0)	{ /* READ_SINGLE_BLOCK */
			if (rcvr_datablock(buff, 512)) {
				count = 0;
			}
		}
	}
	else {				/* Multiple block read */
		if (send_cmd(CMD18, sector) == 0) {	/* READ_MULTIPLE_BLOCK */
			do {
				if (!rcvr_datablock(buff, 512)) {
					break;
				}
				buff += 512;
			} while (--count);
			send_cmd(CMD12, 0);				/* STOP_TRANSMISSION */
		}
	}
	release_spi();

	return count ? RES_ERROR : RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
#if _FS_READONLY == 0
DRESULT MMC_disk_write(
	const BYTE *buff,	/* Pointer to the data to be written */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..255) */
)
{
	if (!count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;
	if (Stat & STA_PROTECT) return RES_WRPRT;

	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert to byte address if needed */

	if (count == 1) {	/* Single block write */
		if ((send_cmd(CMD24, sector) == 0)	/* WRITE_BLOCK */
			&& xmit_datablock(buff, 0xFE))
			count = 0;
	}
	else {				/* Multiple block write */
		if (CardType & CT_SDC) send_cmd(ACMD23, count);
		if (send_cmd(CMD25, sector) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if (!xmit_datablock(buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!xmit_datablock(0, 0xFD))	/* STOP_TRAN token */
				count = 1;
		}
	}
	release_spi();

	return count ? RES_ERROR : RES_OK;
}
#endif /* _READONLY == 0 */


DSTATUS MMC_disk_ioctl(
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	BYTE n, csd[16], *ptr = buff;
	WORD csize;

	res = RES_ERROR;

	if (ctrl == CTRL_POWER) {
		switch (*ptr) {
		case 0:		/* Sub control code == 0 (POWER_OFF) */
			if (socket_is_powered())
				power_off();		/* Power off */
			res = RES_OK;
			break;
		case 1:		/* Sub control code == 1 (POWER_ON) */
			power_on();				/* Power on */
			res = RES_OK;
			break;
		case 2:		/* Sub control code == 2 (POWER_GET) */
			*(ptr+1) = (BYTE)socket_is_powered();
			res = RES_OK;
			break;
		default :
			res = RES_PARERR;
		}
	}
	else {
		if (Stat & STA_NOINIT) return RES_NOTRDY;

		switch (ctrl) {
		case CTRL_SYNC :		/* Make sure that no pending write process */
			select_card();
			if (wait_ready() == 0xFF)
				res = RES_OK;
			break;

		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
			if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
				if ((csd[0] >> 6) == 1) {	/* SDC version 2.00 */
					csize = csd[9] + ((WORD)csd[8] << 8) + 1;
					*(DWORD*)buff = (DWORD)csize << 10;
				} else {					/* SDC version 1.XX or MMC*/
					n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
					csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
					*(DWORD*)buff = (DWORD)csize << (n - 9);
				}
				res = RES_OK;
			}
			break;

		case GET_SECTOR_SIZE :	/* Get R/W sector size (WORD) */
			*(WORD*)buff = 512;
			res = RES_OK;
			break;

		case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
			if (CardType & CT_SD2) {	/* SDC version 2.00 */
				if (send_cmd(ACMD13, 0) == 0) {	/* Read SD status */
					rcvr_spi();
					if (rcvr_datablock(csd, 16)) {				/* Read partial block */
						for (n = 64 - 16; n; n--) rcvr_spi();	/* Purge trailing data */
						*(DWORD*)buff = 16UL << (csd[10] >> 4);
						res = RES_OK;
					}
				}
			} else {					/* SDC version 1.XX or MMC */
				if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {	/* Read CSD */
					if (CardType & CT_SD1) {	/* SDC version 1.XX */
						*(DWORD*)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
					} else {					/* MMC */
						*(DWORD*)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
					}
					res = RES_OK;
				}
			}
			break;

		case MMC_GET_TYPE :		/* Get card type flags (1 byte) */
			*ptr = CardType;
			res = RES_OK;
			break;

		case MMC_GET_CSD :		/* Receive CSD as a data block (16 bytes) */
			if (send_cmd(CMD9, 0) == 0		/* READ_CSD */
				&& rcvr_datablock(ptr, 16))
				res = RES_OK;
			break;

		case MMC_GET_CID :		/* Receive CID as a data block (16 bytes) */
			if (send_cmd(CMD10, 0) == 0		/* READ_CID */
				&& rcvr_datablock(ptr, 16))
				res = RES_OK;
			break;

		case MMC_GET_OCR :		/* Receive OCR as an R3 resp (4 bytes) */
			if (send_cmd(CMD58, 0) == 0) {	/* READ_OCR */
				for (n = 4; n; n--) *ptr++ = rcvr_spi();
				res = RES_OK;
			}
			break;

		case MMC_GET_SDSTAT :	/* Receive SD status as a data block (64 bytes) */
			if (send_cmd(ACMD13, 0) == 0) {	/* SD_STATUS */
				rcvr_spi();
				if (rcvr_datablock(ptr, 64))
					res = RES_OK;
			}
			break;

		default:
			res = RES_PARERR;
		}

		release_spi();
	}

	return res;
}

/*-----------------------------------------------------------------------*/
/* Device Timer Interrupt Procedure  (Platform dependent)                */
/*-----------------------------------------------------------------------*/
/* This function must be called in period of 10ms                        */
void MMC_disk_timerproc(void)
{
	static DWORD pv;
	DWORD ns;
	BYTE n, s;

	n = Timer1;                /* 100Hz decrement timers */
	if (n) Timer1 = --n;
	n = Timer2;
	if (n) Timer2 = --n;

	ns = pv;
	pv = socket_is_empty() | socket_is_write_protected();	/* Sample socket switch */

	if (ns == pv) {                         /* Have contacts stabled? */
		s = Stat;

		if (pv & socket_state_mask_wp)      /* WP is H (write protected) */
			s |= STA_PROTECT;
		else                                /* WP is L (write enabled) */
			s &= ~STA_PROTECT;

		if (pv & socket_state_mask_cp)      /* INS = H (Socket empty) */
			s |= (STA_NODISK | STA_NOINIT);
		else                                /* INS = L (Card inserted) */
			s &= ~STA_NODISK;

		Stat = s;
	}
}
