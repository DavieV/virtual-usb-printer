#ifndef __DEVICE_DESCRIPTORS_H__
#define __DEVICE_DESCRIPTORS_H__

#include "usbip-constants.h"

// USB Device Descriptor
// https://www.keil.com/pack/doc/mw/USB/html/_u_s_b__device__descriptor.html
typedef struct __attribute__((__packed__)) _USB_DEVICE_DESCRIPTOR {
  byte bLength;
  byte bDescriptorType;  // Type = 0x01 (USB_DESCRIPTOR_DEVICE).
  word bcdUSB;
  byte bDeviceClass;
  byte bDeviceSubClass;
  byte bDeviceProtocol;
  byte bMaxPacketSize0;
  word idVendor;
  word idProduct;
  word bcdDevice;
  byte iManufacturer;
  byte iProduct;
  byte iSerialNumber;
  byte bNumConfigurations;
} USB_DEVICE_DESCRIPTOR;

// USB Configuration Descriptor
// https://www.keil.com/pack/doc/mw/USB/html/_u_s_b__configuration__descriptor.html
typedef struct __attribute__((__packed__)) _USB_CONFIGURATION_DESCRIPTOR {
  byte bLength;
  byte bDescriptorType;  // Type = 0x02 (USB_DESCRIPTOR_CONFIGURATION).
  word wTotalLength;
  byte bNumInterfaces;
  byte bConfigurationValue;
  byte iConfiguration;
  byte bmAttributes;
  byte bMaxPower;
} USB_CONFIGURATION_DESCRIPTOR;

// USB Interface Descriptor
// https://www.keil.com/pack/doc/mw/USB/html/_u_s_b__interface__descriptor.html
typedef struct __attribute__((__packed__)) _USB_INTERFACE_DESCRIPTOR {
  byte bLength;
  byte bDescriptorType;  // Type = 0x04 (USB_DESCRIPTOR_INTERFACE).
  byte bInterfaceNumber;
  byte bAlternateSetting;
  byte bNumEndpoints;
  byte bInterfaceClass;
  byte bInterfaceSubClass;
  byte bInterfaceProtocol;
  byte iInterface;
} USB_INTERFACE_DESCRIPTOR;

// USB Endpoint Descriptor
// https://www.keil.com/pack/doc/mw/USB/html/_u_s_b__endpoint__descriptor.html
typedef struct __attribute__((__packed__)) _USB_ENDPOINT_DESCRIPTOR {
  byte bLength;
  byte bDescriptorType;   // Type = 0x05 (USB_DESCRIPTOR_ENDPOINT).
  byte bEndpointAddress;  // Bit 7 indicates direction (0=OUT, 1=IN).
  byte bmAttributes;
  word wMaxPacketSize;
  byte bInterval;
} USB_ENDPOINT_DESCRIPTOR;

// USB Device Qualifier Descriptor
// https://www.keil.com/pack/doc/mw/USB/html/_u_s_b__device__qualifier__descriptor.html
typedef struct __attribute__((__packed__)) _USB_DEVICE_QUALIFIER_DESCRIPTOR {
  byte bLength;
  byte bDescriptorType;  // Type = 0x06 (USB_DESCRIPTOR_DEVICE_QUALIFIER).
  word bcdUSB;
  byte bDeviceClass;
  byte bDeviceSubClass;
  byte bDeviceProtocol;
  byte bMaxPacketSize0;
  byte bNumConfigurations;
  byte bReserved;  // Always zero (0)
} USB_DEVICE_QUALIFIER_DESCRIPTOR;

// Generic Configuration
typedef struct __attribute__((__packed__)) _CONFIG_GEN {
  USB_CONFIGURATION_DESCRIPTOR dev_conf;
  USB_INTERFACE_DESCRIPTOR dev_int;
} CONFIG_GEN;

typedef struct __attribute__((__packed__)) _CONFIG_PRINTER {
  USB_CONFIGURATION_DESCRIPTOR dev_conf;
  USB_INTERFACE_DESCRIPTOR dev_int;
  USB_ENDPOINT_DESCRIPTOR dev_end_out;
  USB_ENDPOINT_DESCRIPTOR dev_end_in;
} CONFIG_PRINTER;

// Human Input Device (HID) Descriptor
// For more details refer to http://www.usb.org/developers/hidpage/HID1_11.pdf
// Section 6.2.1 HID Descriptor
typedef struct __attribute__((__packed__)) _USB_HID_DESCRIPTOR {
  byte bLength;
  byte bDescriptorType;
  word bcdHID;
  byte bCountryCode;
  byte bNumDescriptors;
  byte bRPDescriptorType;
  word wRPDescriptorLength;
} USB_HID_DESCRIPTOR;

// Represents a configured HID.
typedef struct __attribute__((__packed__)) _CONFIG_HID {
  USB_CONFIGURATION_DESCRIPTOR dev_conf;
  USB_INTERFACE_DESCRIPTOR dev_int;
  USB_HID_DESCRIPTOR dev_hid;
  USB_ENDPOINT_DESCRIPTOR dev_ep;
} CONFIG_HID;

#endif  // __DEVICE_DESCRIPTORS_H__
