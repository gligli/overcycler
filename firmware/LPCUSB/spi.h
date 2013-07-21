#include "type.h"

#define SPI_PRESCALE_MIN  8

void	SPIInit(void);
void	SPISetSpeed(U8 speed);

U8		SPISend(U8 outgoing);
void	SPISendN(U8 *pbBuf, int iLen);
void	SPIRecvN(U8 *pbBuf, int iLen);

#define SS_PORT_0
#define SPI_SS_PIN	7

//SPI Chip Select Defines for SD Access
#ifdef SS_PORT_1
	#define	SPI_SS_IODIR	IODIR1
	#define	SPI_SS_IOCLR	IOCLR1
	#define	SPI_SS_IOSET	IOSET1   
#endif
#ifdef	SS_PORT_0
	#define	SPI_SS_IODIR	IODIR0
	#define	SPI_SS_IOCLR	IOCLR0
	#define	SPI_SS_IOSET	IOSET0
#endif  

//SPI Pin Location Definitions
#define SPI_IODIR      	IODIR0
#define SPI_SCK_PIN    	4       
#define SPI_MISO_PIN   	5         
#define SPI_MOSI_PIN   	6        
		 

#define SPI_PINSEL     		PINSEL0
#define SPI_SCK_FUNCBIT   	8
#define SPI_MISO_FUNCBIT  	10
#define SPI_MOSI_FUNCBIT  	12

#define SPI_PRESCALE_REG  	S0SPCCR

#define SELECT_CARD()   	SPI_SS_IOCLR |= (1 << SPI_SS_PIN)
#define UNSELECT_CARD() 	SPI_SS_IOSET |= (1 << SPI_SS_PIN)
