#ifndef __USBIP_CONSTANTS_H__
#define __USBIP_CONSTANTS_H__

// USB Decriptor Type Constants.
#define USB_DESCRIPTOR_DEVICE           0x01    // Device Descriptor.
#define USB_DESCRIPTOR_CONFIGURATION    0x02    // Configuration Descriptor.
#define USB_DESCRIPTOR_STRING           0x03    // String Descriptor.
#define USB_DESCRIPTOR_INTERFACE        0x04    // Interface Descriptor.
#define USB_DESCRIPTOR_ENDPOINT         0x05    // Endpoint Descriptor.
#define USB_DESCRIPTOR_DEVICE_QUALIFIER 0x06    // Device Qualifier.

// USB "bRequest" Constants.
// These represent the possible values contained within a USB SETUP packet which
// specify the type of request.
#define GET_STATUS 0x00
#define CLEAR_FEATURE 0x01
#define SET_FEATURE 0x03
#define SET_ADDRESS 0x05
#define GET_DESCRIPTOR 0x06
#define SET_DESCRIPTOR 0x07
#define GET_CONFIGURATION 0x08
#define SET_CONFIGURATION 0x09
#define GET_INTERFACE 0x0A
#define SET_INTERFACE 0x0B
#define SET_FRAME 0x0C

// Special "bRequest" values for HID requests.
#define GET_REPORT 0x01
#define GET_IDLE 0x02
#define GET_PROTOCOL 0x03
#define SET_REPORT 0x09
#define SET_IDLE 0x0A
#define SET_PROTOCOL 0x0B

// OP Commands.
#define OP_REQ_DEVLIST_CMD 0x8005
#define OP_REQ_IMPORT_CMD 0x8003

// Port that the server is bound to.
#define TCP_SERV_PORT 3240

#endif  // __USBIP_CONSTANTS_H__
