#include "xt26g.h"
#include "lpc177x_8x_ssp.h"
#include "lpc177x_8x_pinsel.h"

//SD card SPI initialize
void XT26G_Init(void)
{
	//Configure pins
	PINSEL_ConfigPin(0, 6, 2);
	PINSEL_ConfigPin(0, 7, 2);
	PINSEL_ConfigPin(0, 8, 2);
	PINSEL_ConfigPin(0, 9, 2);

	// SSP Configuration structure variable
	SSP_CFG_Type SSP_ConfigStruct;
	// initialize SSP configuration structure to default
	SSP_ConfigStructInit(&SSP_ConfigStruct);
	SSP_ConfigStruct.ClockRate = 30000000;
	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);
	// Enable SD_SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);
}



/*/
///SD card SPI write and read one byte data
uint8_t SD_SPI_WriteAndRead(uint8_t data)
{
	uint8_t i = 0;
	while(RESET == SSP_GetStatus(SD_SSP, SSP_STAT_TXFIFO_EMPTY))
	{
		i++; if(i == 250) break;
	}
	
	SSP_SendData(SD_SSP, data);
	
	i = 0;
	while(RESET == SSP_GetStatus(SD_SSP, SSP_STAT_RXFIFO_NOTEMPTY))
	{
		i ++; if(i == 250) break;
	}

	return SSP_ReceiveData(SD_SSP);
}
*/