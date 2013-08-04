///////////////////////////////////////////////////////////////////////////////
// OverCyler, 2*DAC/VCF/VCA polyphonic wavetable synthesizer
// based on SparkFun's ARM Bootloader
///////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "synth.h"

#include "partition.h"
#include "sd_raw.h"
#include "LPCUSB/main_msc.h"
#include "LPCUSB/type.h"
#include "LPCUSB/usbhw_lpc.h"

///////////////////////////////////////////////////////////////////////////////
// OverCycler main code
///////////////////////////////////////////////////////////////////////////////

int main_initStorage(void);


struct partition_struct* partition;
struct fat16_fs_struct* fs;

int main_initStorage(void)
{
    /* open first partition */
    partition = partition_open((device_read_t) sd_raw_read,
                               (device_read_interval_t) sd_raw_read_interval,
                               (device_write_t) sd_raw_write,
                               0);

    if(!partition)
    {
        /* If the partition did not open, assume the storage device
             *      * is a "superfloppy", i.e. has no MBR.
             *           */
        partition = partition_open((device_read_t) sd_raw_read,
                                   (device_read_interval_t) sd_raw_read_interval,
                                   (device_write_t) sd_raw_write,
                                   -1);
        if(!partition)
        {
            rprintf("opening partition failed\n\r");
            return 1;
        }
    }

    /* open file system */
    fs = fat16_open(partition);
    if(!fs)
    {
        rprintf("opening filesystem failed\n\r");
        return 1;
    }

    return 0;
}

int main(void)
{
	boot_up();
	
	disableInts();
	
	// init Portios
	SCS=0x03; // select the "fast" version of the I/O ports

	if(sd_raw_init()) //Initialize the SD card
	{
		main_initStorage(); 
	}
	else
	{
		//Didn't find a card to initialize
		rprintf("No SD Card Detected\n");
		delay_ms(1000);
		reset();
	}
	
	synth_init();
	
	enableInts();
	
	for(;;)
	{
		synth_update();
	}
}
