#include "server.h"
#include "device_descriptors.h"
#include "usbip.h"
#include "usbip-constants.h"
#include "usb_printer.h"

#include <vector>

int main() {
  const USB_DEVICE_DESCRIPTOR device = {
      0x12,                   // Size of this descriptor in bytes
      USB_DESCRIPTOR_DEVICE,  // descriptor type
      0x0110,                 // USB Spec Release Number in BCD format
      0x00,                   // Class Code
      0x00,                   // Subclass code
      0x00,                   // Protocol code
      0x08,                   // Max packet size for EP0, see usb_config.h
      0x04a9,                 // Vendor ID
      0x27e8,                 // Product ID
      0x0000,                 // Device release number in BCD format
      0x01,                   // Manufacturer string descriptor index
      0x02,                   // Product string descriptor index
      0x01,                   // Device serial number string descriptor index
      0x01,                   // Number of possible configurations
  };

  const USB_CONFIGURATION_DESCRIPTOR configuration = {
      0x09,                          // Size of this descriptor in bytes.
      USB_DESCRIPTOR_CONFIGURATION,  // descriptor type.
      0x20,  // Total length of data for this configuration.
      0x01,  // Number of interfaces in this configuration.
      0x01,  // Index value for this configuration.
      0x00,  // Configuration string descriptor index.
      0x80,  // Configuration characteristics (bmAttributes).
      0x00,  // Max power consumption (2X mA).
  };

  std::vector<char> str1 = {
      0x04, USB_DESCRIPTOR_STRING, // bLength, bDscType
      0x09, 0x04
  };

  std::vector<char> str2 = {
      0x0E, USB_DESCRIPTOR_STRING, // bLength, bDscType
      'D', 0x00,
      'a', 0x00,
      'v', 0x00,
      'i', 0x00,
      'e', 0x00,
      'V', 0x00,
  };

  std::vector<char> str3 = {
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

  const std::vector<std::vector<char>> strings = {str1, str2, str3};

  const std::vector<char> ieee_device_id = {
    0x00, 0x1A,  // Length (26 bytes)
    'M', 'F', 'G', ':', 'D', 'V', '3', ';',
    'C', 'M', 'D', ':', 'P', 'D', 'F', ';',
    'M', 'D', 'L', ':', 'V', 'T', 'L', ';',
  };

  const std::vector<USB_INTERFACE_DESCRIPTOR> interfaces = {
      {
          0x09,                      // Size of this descriptor in bytes
          USB_DESCRIPTOR_INTERFACE,  // descriptor type.
          0x00,                      // Interface Number.
          0x00,                      // Alternate Setting Number.
          0x02,                      // Number of endpoints in this interface.
          0x07,                      // Class code.
          0x01,                      // Subclass code.
          0x02,                      // Protocol code.
          0x00,                      // Interface string index.
      },
      /*
      {
          0x09,                      // Size of this descriptor in bytes
          USB_DESCRIPTOR_INTERFACE,  // descriptor type.
          0x01,                      // Interface Number.
          0x00,                      // Alternate Setting Number.
          0x02,                      // Number of endpoints in this interface.
          0x07,                      // Class code.
          0x01,                      // Subclass code.
          0x02,                      // Protocol code.
          0x00,                      // Interface string index.
      }*/};

  const std::vector<USB_ENDPOINT_DESCRIPTOR> endpoints = {
      {
          0x07,                     // Size of this descriptor in bytes.
          USB_DESCRIPTOR_ENDPOINT,  // descriptor type.
          0x01,                     // Endpoint Address.
          0x02,                     // Attributes (Bulk Endpoint).
          512,                      // Max transfer size.
          0x00,                     // Interval.
      },
      {
          0x07,                     // Size of this descriptor in bytes.
          USB_DESCRIPTOR_ENDPOINT,  // descriptor type.
          0x81,                     // Endpoint Address.
          0x02,                     // Attributes (Bulk Endpoint).
          512,                      // Max transfer size.
          0x00,                     // Interval.
      },
      /*
      {
          0x07,                     // Size of this descriptor in bytes.
          USB_DESCRIPTOR_ENDPOINT,  // descriptor type.
          0x03,                     // Endpoint Address.
          0x02,                     // Attributes (Bulk Endpoint).
          0x08,                     // Max transfer size.
          0x00,                     // Interval.
      },
      {
          0x07,                     // Size of this descriptor in bytes.
          USB_DESCRIPTOR_ENDPOINT,  // descriptor type.
          0x84,                     // Endpoint Address.
          0x02,                     // Attributes (Bulk Endpoint).
          0x08,                     // Max transfer size.
          0x00,                     // Interval.
      }*/};

  UsbPrinter printer(device, configuration, strings, ieee_device_id, interfaces,
                     endpoints);
  run_server(printer);
}
