///////////////////////////////////////////////////////////////////////////////
// OverCyler, 2*DAC/VCF/VCA polyphonic wavetable synthesizer
// based on SparkFun's ARM Bootloader
///////////////////////////////////////////////////////////////////////////////

#include "LPC214x.h"

//UART0 Debugging
#include "serial.h"
#include "rprintf.h"

//Memory manipulation and basic system stuff
#include "firmware.h"
#include "system.h"

//SD Logging
#include "rootdir.h"
#include "sd_raw.h"

//USB
#include "main_msc.h"

#define LED		(1<<24)	//The Red LED is on Port 0-Pin 18

int main (void)
{
	boot_up();
	
	IODIR1 |= LED;	//Set the Red, Green and Blue LED pins as outputs
	IOSET1 = LED;	//Initially turn all of the LED's off
	while(1){	//Now that everything is initialized, let's run an endless program loop
		IOCLR1 = LED;	//Turn on the Red LED
		delay_ms(333);		//Leave the Red LED on for 1/3 of a second
		IOSET1 = LED;	//and turn off Red and Blue
		delay_ms(333);		//Leave the Green LED on for 1/3 of a second
	}

	while(1);
}
