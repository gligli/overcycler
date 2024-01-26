#include "usb_power.h"

#include "usb_debug.h"
#include "usbapi.h"

#define MIDI_BULK_OUT_EP	0x01
#define MIDI_BULK_IN_EP		0x81

#define MIDI_JACK_USB_OUT 0x01
#define MIDI_JACK_DEV_OUT 0x02
#define MIDI_JACK_USB_IN 0x03
#define MIDI_JACK_DEV_IN 0x04

#define LE_WORD(x)		((x)&0xFF),((x)>>8)

static const U8 abDescriptors[] = {

// device descriptor	
	0x12,
	DESC_DEVICE,			
	LE_WORD(0x0110),		// bcdUSB
	0x00,					// bDeviceClass
	0x00,					// bDeviceSubClass
	0x00,					// bDeviceProtocol
	MAX_PACKET_SIZE,		// bMaxPacketSize
	LE_WORD(0x6112),		// idVendor
	LE_WORD(0x0C30),		// idProduct
	LE_WORD(0x0001),		// bcdDevice
	0x01,					// iManufacturer
	0x02,					// iProduct
	0x03,					// iSerialNumber
	0x01,					// bNumConfigurations

// configuration descriptor
	0x09,
	DESC_CONFIGURATION,
	LE_WORD(9),				// wTotalLength
	0x00,					// bNumInterfaces
	0x00,					// bConfigurationValue
	0x00,					// iConfiguration
	0x80,					// bmAttributes
	0xFA,					// bMaxPower (500ma)

// string descriptors
	0x04,
	DESC_STRING,
	LE_WORD(0x0409),

	26,
	DESC_STRING,
	'G', 0, 'l', 0, 'i', 0, 'G', 0, 'l', 0, 'i', 0, '\'', 0, 's', 0, ' ', 0, 'D', 0, 'I', 0, 'Y', 0,

	48,
	DESC_STRING,
	'O', 0, 'v', 0, 'e', 0, 'r', 0, 'c', 0, 'y', 0, 'c', 0, 'l', 0, 'e', 0, 'r', 0, ' ', 0, '(', 0, 'p', 0, 'o', 0, 'w', 0, 'e', 0, 'r', 0, ' ', 0, 'o', 0, 'n', 0, 'l', 0, 'y', 0, ')', 0,

	8,
	DESC_STRING,
	'3', 0, '.', 0, '0', 0,

// terminating zeroes
	0, 0
};

void usb_power_start(void)
{
	// initialise stack
	USBInit();
	
	// register descriptors
	USBRegisterDescriptors(abDescriptors);

	// connect to bus
	USBHwConnect(TRUE);
}
