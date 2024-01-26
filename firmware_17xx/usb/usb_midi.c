#include <time.h>

#include "usb_midi.h"

#include "usb_debug.h"
#include "usbapi.h"

#include "synth/synth.h"

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
	LE_WORD(9+9+9+9+7+6+9+9+5),	// wTotalLength
	0x02,					// bNumInterfaces
	0x01,					// bConfigurationValue
	0x00,					// iConfiguration
	0x80,					// bmAttributes
	0xFA,					// bMaxPower (500ma)

// interface (dummmy AudioControl)
	0x09,
	DESC_INTERFACE,
	0x00,					// bInterfaceNumber
	0x00,					// bAlternateSetting
	0x00,					// bNumEndPoints
	0x01,					// bInterfaceClass = AUDIO
	0x01,					// bInterfaceSubClass = AUDIO_CONTROL
	0x00,					// bInterfaceProtocol
	0x00,					// iInterface
	
//Class Specific Audio Interface
	0x09,        // bLength
	0x24,        // bDescriptorType (See Next Line)
	0x01,        // bDescriptorSubtype (CS_INTERFACE -> HEADER)
	0x00, 0x01,  // bcdADC 1.00
	LE_WORD(9),	 // wTotalLength
	0x01,        // binCollection 0x01
	0x01,        // baInterfaceNr 1
	
// interface (MIDIStreaming)
	0x09,
	DESC_INTERFACE,
	0x01,					// bInterfaceNumber
	0x00,					// bAlternateSetting
	0x01,					// bNumEndPoints
	0x01,					// bInterfaceClass = AUDIO
	0x03,					// bInterfaceSubClass = MIDISTREAMING
	0x00,					// bInterfaceProtocol
	0x00,					// iInterface
	
//MIDI Interface Header Descriptor
	0x07,        // bLength
	0x24,        // bDescriptorType (See Next Line)
	0x01,        // bDescriptorSubtype (CS_INTERFACE -> MS_HEADER)
	0x00, 0x01,  // bcdMSC 1.00
	LE_WORD(7+6+9), // wTotalLength

//MIDI In Jack 1
	0x06,        // bLength
	0x24,        // bDescriptorType (See Next Line)
	0x02,        // bDescriptorSubtype (CS_INTERFACE -> MIDI_IN_JACK)
	0x01,        // bJackType (EMBEDDED)
	MIDI_JACK_USB_OUT, // bJackID
	0x00,        // iJack

//MIDI Out Jack 2
	0x09,        // bLength
	0x24,        // bDescriptorType (See Next Line)
	0x03,        // bDescriptorSubtype (CS_INTERFACE -> MIDI_OUT_JACK)
	0x02,        // bJackType (EXTERNAL)
	MIDI_JACK_DEV_IN, // bJackID
	0x01,        // bNrInputPins
	MIDI_JACK_USB_OUT, // baSourceID(1)
	0x00,        // baSourcePin(1)
	0x00,        // iJack
	
// OUT EP
	0x09,
	DESC_ENDPOINT,
	MIDI_BULK_OUT_EP,		// bEndpointAddress
	0x02,					// bmAttributes = bulk
	LE_WORD(MAX_PACKET_SIZE),// wMaxPacketSize
	0x00,					// bInterval
	0x00,        // bRefresh
	0x00,        // bSyncAddress	

//Class-specific MS Bulk OUT Descriptor
	0x05,        // bLength
	0x25,        // bDescriptorType (See Next Line)
	0x01,        // bDescriptorSubtype (CS_ENDPOINT -> MS_GENERAL)
	0x01,        // bNumEmbMIDIJack (num of MIDI **IN** Jacks)
	MIDI_JACK_USB_OUT, // BaAssocJackID(1) 1

// string descriptors
	0x04,
	DESC_STRING,
	LE_WORD(0x0409),

	26,
	DESC_STRING,
	'G', 0, 'l', 0, 'i', 0, 'G', 0, 'l', 0, 'i', 0, '\'', 0, 's', 0, ' ', 0, 'D', 0, 'I', 0, 'Y', 0,

	22,
	DESC_STRING,
	'O', 0, 'v', 0, 'e', 0, 'r', 0, 'c', 0, 'y', 0, 'c', 0, 'l', 0, 'e', 0, 'r', 0,

	8,
	DESC_STRING,
	'3', 0, '.', 0, '0', 0,

// terminating zeroes
	0, 0
};

void MIDIBulkOut(U8 bEP, U8 bEPStatus)
{
	const U8 code2size[16] = {0,0,2,3,3,1,2,3,3,3,3,3,2,2,3,1};
	
	DBG("MIDIBulkOut %x %x\n", bEP, bEPStatus);
	
	// ignore events on stalled EP
	if (bEPStatus & EP_STATUS_STALLED) {
		return;
	}
	
	int len;
	U8 cmdBuf[4];
	
	len = USBHwEPRead(bEP, cmdBuf, sizeof(cmdBuf));
	
	if(len==sizeof(cmdBuf))
	{
		for(U8 p = 0; p < code2size[cmdBuf[0] & 0xf]; ++p)
			synth_usbMIDIEvent(cmdBuf[1+p]);
	}
}

void usb_midi_start(void)
{
	// initialise stack
	USBInit();
	
	// enable bulk-in interrupts on NAKs
	// these are required to get the BOT protocol going again after a STALL
	USBHwNakIntEnable(INACK_BI);

	// register descriptors
	USBRegisterDescriptors(abDescriptors);

	// register endpoint handlers
	USBHwRegisterEPIntHandler(MIDI_BULK_OUT_EP, MIDIBulkOut);

	// connect to bus
	USBHwConnect(TRUE);
}
