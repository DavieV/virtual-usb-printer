#ifndef __USBIP_USB_PRINTER_H__
#define __USBIP_USB_PRINTER_H__

#include "device_descriptors.h"
#include "usbip-constants.h"
#include "usbip.h"

#include <vector>

// Generice USB device interface.
class UsbPrinter {
 public:
  explicit UsbPrinter(
      const USB_DEVICE_DESCRIPTOR& device_descriptor,
      const USB_CONFIGURATION_DESCRIPTOR& configuration_descriptor,
      const std::vector<std::vector<char>>& strings,
      const std::vector<char> ieee_device_id,
      const std::vector<USB_INTERFACE_DESCRIPTOR>& interfaces,
      const std::vector<USB_ENDPOINT_DESCRIPTOR>& endpoints);

  USB_DEVICE_DESCRIPTOR device_descriptor() const { return device_descriptor_; }

  USB_CONFIGURATION_DESCRIPTOR configuration_descriptor() const {
    return configuration_descriptor_;
  }

  std::vector<std::vector<char>> strings() const { return strings_; }

  std::vector<USB_INTERFACE_DESCRIPTOR> interfaces() const {
    return interfaces_;
  }

  std::vector<USB_ENDPOINT_DESCRIPTOR> endpoints() const { return endpoints_; }

  // Determines whether |usb_request| is either a control or data request and
  // defers to the corresponding function.
  void HandleUsbRequest(int sockd, const USBIP_CMD_SUBMIT& usb_request);

  // Determines whether |usb_request| is either a standard or class-specific
  // control request and defers to the corresponding function.
  void HandleUsbControl(int sockfd, const USBIP_CMD_SUBMIT& usb_request);

  // Handles the standard USB requests.
  void HandleStandardControl(int sockfd, const USBIP_CMD_SUBMIT& usb_request,
                             const StandardDeviceRequest& control_request);

  // Handles printer-specific USB requests.
  void HandlePrinterControl(int sockfd, const USBIP_CMD_SUBMIT& usb_request,
                            const StandardDeviceRequest& control_request);

 private:
  void HandleGetDescriptor(int sockfd, const USBIP_CMD_SUBMIT& usb_request,
                           const StandardDeviceRequest& control_request) const;

  void HandleGetConfigurationDescriptor(
      int sockfd, const USBIP_CMD_SUBMIT& usb_request,
      const StandardDeviceRequest& control_request) const;

  void HandleGetStringDescriptor(
      int sockfd, const USBIP_CMD_SUBMIT& usb_request,
      const StandardDeviceRequest& control_request) const;

  void HandleGetConfiguration(int sockfd, const USBIP_CMD_SUBMIT& usb_request,
                              const StandardDeviceRequest& control_request);

  void HandleSetConfiguration(int sockfd, const USBIP_CMD_SUBMIT& usb_request,
                              const StandardDeviceRequest& control_request);

  void HandleSetInterface(int sockfd, const USBIP_CMD_SUBMIT& usb_request,
                          const StandardDeviceRequest& control_request);

  void HandleGetDeviceId(int sockfd, const USBIP_CMD_SUBMIT& usb_request,
                         const StandardDeviceRequest& control_request);

  USB_DEVICE_DESCRIPTOR device_descriptor_;
  USB_CONFIGURATION_DESCRIPTOR configuration_descriptor_;
  std::vector<std::vector<char>> strings_;
  std::vector<char> ieee_device_id_;
  std::vector<USB_INTERFACE_DESCRIPTOR> interfaces_;
  std::vector<USB_ENDPOINT_DESCRIPTOR> endpoints_;
};

#endif  // __USBIP_USB_PRINTER_H__
