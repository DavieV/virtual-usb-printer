// David Valleau

#include "device_descriptors.h"
#include "usbip-constants.h"
#include "usbip.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Device Descriptor.
const USB_DEVICE_DESCRIPTOR dev_dsc = {
    0x12,    // Size of this descriptor in bytes
    0x01,    // descriptor type
    0x0110,  // USB Spec Release Number in BCD format
    0x00,    // Class Code
    0x00,    // Subclass code
    0x00,    // Protocol code
    0x08,    // Max packet size for EP0, see usb_config.h
    0x2706,  // Vendor ID
    0x0100,  // Product ID
    0x0000,  // Device release number in BCD format
    0x01,    // Manufacturer string descriptor index
    0x02,    // Product string descriptor index
    0x01,    // Device serial number string descriptor index
    0x01     // Number of possible configurations
};

// Generic Configuration.
const CONFIG_PRINTER printer_config = {
  {
    // Configuration Descriptor.
    0x09,  // Size of this descriptor in bytes.
    USB_DESCRIPTOR_CONFIGURATION,  // descriptor type.
    0x0020,  // Total length of data for this configuration.
    0x01,  // Number of interfaces in this configuration.
    0x01,  // Index value for this configuration.
    0x00,  // Configuration string descriptor index.
    0x80,  // Configuration characteristics (bmAttributes).
    0x32,  // Max power consumption (2X mA).
  },
  {
    // Interface Descriptor.
    0x09,  // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,  // descriptor type.
    0x00,  // Interface Number.
    0x00,  // Alternate Setting Number.
    0x02,  // Number of endpoints in this interface.
    0x07,  // Class code.
    0x01,  // Subclass code.
    0x01,  // Protocol code.
    0x00,  // Interface string index.
  },
  {
    // Endpoint Descriptor (Bulk Out).
    0x07,  // Size of this descriptor in bytes.
    USB_DESCRIPTOR_ENDPOINT,  // descriptor type.
    0x00,  // Endpoint Address.
    0x02,  // Attributes (Bulk Endpoint).
    255,   // Max transfer size.
    0x00,  // Interval.
  },
  {
    // Endpoint Descriptor (Bulk In).
    0x07,  // Size of this descriptor in bytes.
    USB_DESCRIPTOR_ENDPOINT,  // descriptor type.
    0x81,  // Endpoint Address.
    0x02,  // Attributes (Bulk Endpoint).
    0x08,   // Max transfer size.
    0x00,  // Interval.
  }
};

// Available languages descriptor.
const unsigned char string_0[] = { // available languages  descriptor
  0x04, USB_DESCRIPTOR_STRING, // bLength, bDscType
  0x09, 0x04
};

// Manufacturer String Descriptor.
const unsigned char string_1[] = {
  0x0E, USB_DESCRIPTOR_STRING, // bLength, bDscType
  'D', 0x00,
  'a', 0x00,
  'v', 0x00,
  'i', 0x00,
  'e', 0x00,
  'V', 0x00,
};

// Product String Descriptor.
const unsigned char string_2[] = {
  0x28, USB_DESCRIPTOR_STRING, // bLength, bDscType
  'V', 0x00,
  'i', 0x00,
  'r', 0x00,
  't', 0x00,
  'u', 0x00,
  'a', 0x00,
  'l', 0x00,
  ' ', 0x00,
  'U', 0x00,
  'S', 0x00,
  'B', 0x00,
  ' ', 0x00,
  'P', 0x00,
  'r', 0x00,
  'i', 0x00,
  'n', 0x00,
  't', 0x00,
  'e', 0x00,
  'r', 0x00,
};

// Configuration String Descriptor.
const unsigned char string_3[] = {
  0x0E, USB_DESCRIPTOR_STRING,  // bLength, bDscType
  'i', 0x00,
  'p', 0x00,
  'p', 0x00,
  'u', 0x00,
  's', 0x00,
  'b', 0x00,
};

const char *configuration = (const char *)&printer_config;
const USB_INTERFACE_DESCRIPTOR *interfaces[] = {&printer_config.dev_int};
const unsigned char *strings[] = {string_0, string_1, string_2, string_3};
const USB_DEVICE_QUALIFIER_DESCRIPTOR dev_qua = {};

int main() {
  printf("usb printer started....\n");
  usbip_run(&dev_dsc);
}
