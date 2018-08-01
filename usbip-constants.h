#ifndef __USBIP_CONSTANTS_H__
#define __USBIP_CONSTANTS_H__

// Type definitions.
#define byte unsigned char
#define word unsigned short

// USB Decriptor Type Constants.
#define USB_DESCRIPTOR_DEVICE           0x01    // Device Descriptor.
#define USB_DESCRIPTOR_CONFIGURATION    0x02    // Configuration Descriptor.
#define USB_DESCRIPTOR_STRING           0x03    // String Descriptor.
#define USB_DESCRIPTOR_INTERFACE        0x04    // Interface Descriptor.
#define USB_DESCRIPTOR_ENDPOINT         0x05    // Endpoint Descriptor.
#define USB_DESCRIPTOR_DEVICE_QUALIFIER 0x06    // Device Qualifier.

#define STANDARD_TYPE 0  // Standard USB Request.
#define CLASS_TYPE    1  // Class-specific USB Request.
#define VENDOR_TYPE   2  // Vendor-specific USB Request.
#define RESERVED_TYPE 3  // Reserved.

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

// Special "bRequest" values for printer requests.
#define GET_DEVICE_ID 0
#define GET_PORT_STATUS 1
#define SOFT_RESET 2

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

// USBIP Command Constants.
// TODO(daviev): Change these to remove the "COMMAND_" prefix once all of the
// usbip structs have been updated to use a different naming scheme.
#define COMMAND_USBIP_CMD_SUBMIT 0x0001
#define COMMAND_USBIP_CMD_UNLINK 0X0002
#define COMMAND_USBIP_RET_SUBMIT 0x0003
#define COMMAND_USBIP_RET_UNLINK 0x0004

// Port that the server is bound to.
#define TCP_SERV_PORT 3240

#endif  // __USBIP_CONSTANTS_H__
