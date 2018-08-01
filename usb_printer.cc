#include "usb_printer.h"

#include "device_descriptors.h"
#include "usbip.h"
#include "usbip-constants.h"

#include <memory>
#include <vector>

namespace {

// Returns the numeric value of the "type" stored within the |bmRequestType|
// bitmap.
int GetControlType(byte bmRequestType) {
  // The "type" of the request is stored within bits 5 and 6 of |bmRequestType|.
  // So we shift these bits down to the least significant bits and perform a
  // bitwise AND operation in order to clear any other bits and return strictly
  // the number value of the type bits.
  return (bmRequestType >> 5) & 3;
}

// Unpacks the standard USB SETUP packet contained within |setup| into a //
// StandardDeviceRequest struct and returns the result.
StandardDeviceRequest CreateStandardDeviceRequest(long long setup) {
  StandardDeviceRequest request;
  request.bmRequestType = (setup & 0xFF00000000000000) >> 56;
  request.bRequest = (setup & 0x00FF000000000000) >> 48;
  request.wValue0 = (setup & 0x0000FF0000000000) >> 40;
  request.wValue1 = (setup & 0x000000FF00000000) >> 32;
  request.wIndex0 = (setup & 0x00000000FF000000) >> 24;
  request.wIndex1 = (setup & 0x0000000000FF0000) >> 16;
  request.wLength = ntohs(setup & 0x000000000000FFFF);
  return request;
}

// Fills |buffer| with the contents of |descriptor|.
void FillBuffer(const char* descriptor, int size, char* buffer) {
  for (int i = 0; i < size; ++i) {
    buffer[i] = descriptor[i];
  }
}

}  // namespace

// explicit
UsbPrinter::UsbPrinter(
    const USB_DEVICE_DESCRIPTOR& device_descriptor,
    const USB_CONFIGURATION_DESCRIPTOR& configuration_descriptor,
    const std::vector<std::vector<char>>& strings,
    const std::vector<char> ieee_device_id,
    const std::vector<USB_INTERFACE_DESCRIPTOR>& interfaces,
    const std::vector<USB_ENDPOINT_DESCRIPTOR>& endpoints)
    : device_descriptor_(device_descriptor),
      configuration_descriptor_(configuration_descriptor),
      strings_(strings),
      ieee_device_id_(ieee_device_id),
      interfaces_(interfaces),
      endpoints_(endpoints) {}

void UsbPrinter::HandleUsbRequest(int sockfd,
                                  const USBIP_CMD_SUBMIT& usb_request) {
  // Endpoint 0 is used for USB control requests.
  if (usb_request.ep == 0) {
    printf("# control requests\n");
    HandleUsbControl(sockfd, usb_request);
  } else {
    printf("# data requests\n");
  }
}

void UsbPrinter::HandleUsbControl(int sockfd,
                                  const USBIP_CMD_SUBMIT& usb_request) {
  StandardDeviceRequest control_request =
      CreateStandardDeviceRequest(usb_request.setup);
  print_standard_device_request(control_request);
  int request_type = GetControlType(control_request.bmRequestType);
  switch (request_type) {
    case STANDARD_TYPE:
      HandleStandardControl(sockfd, usb_request, control_request);
      break;
    case CLASS_TYPE:
      HandlePrinterControl(sockfd, usb_request, control_request);
      break;
    case VENDOR_TYPE:
    case RESERVED_TYPE:
    default:
      printf("Unable to handle request of type %d\n", request_type);
      break;
  }
}

void UsbPrinter::HandleStandardControl(
    int sockfd, const USBIP_CMD_SUBMIT& usb_request,
    const StandardDeviceRequest& control_request) {
  switch (control_request.bRequest) {
    case GET_STATUS:
      break;
    case GET_DESCRIPTOR:
      HandleGetDescriptor(sockfd, usb_request, control_request);
      break;
    case SET_DESCRIPTOR:
      break;
    case GET_CONFIGURATION:
      HandleGetConfiguration(sockfd, usb_request, control_request);
      break;
    case SET_CONFIGURATION:
      HandleSetConfiguration(sockfd, usb_request, control_request);
      break;
    case GET_INTERFACE:
      // Support for this will be needed for interfaces with alt settings.
      break;
    case SET_INTERFACE:
      HandleSetInterface(sockfd, usb_request, control_request);
      break;
    default:
      break;
  }
}

void UsbPrinter::HandlePrinterControl(
    int sockfd, const USBIP_CMD_SUBMIT& usb_request,
    const StandardDeviceRequest& control_request) {
  switch (control_request.bRequest) {
    case GET_DEVICE_ID:
      HandleGetDeviceId(sockfd, usb_request, control_request);
      break;
    case GET_PORT_STATUS:
      break;
    case SOFT_RESET:
      break;
    default:
      printf("Unknown printer class request\n");
  }
}

void UsbPrinter::HandleGetDescriptor(
    int sockfd, const USBIP_CMD_SUBMIT& usb_request,
    const StandardDeviceRequest& control_request) const {
  printf("HandleGetDescriptor %u[%u]\n", control_request.wValue1,
         control_request.wValue0);

  switch (control_request.wValue1) {
    case USB_DESCRIPTOR_DEVICE:
      SendUsbRequest(sockfd, usb_request, (char*)&device_descriptor_,
                     device_descriptor_.bLength, 0);
      break;
    case USB_DESCRIPTOR_CONFIGURATION:
      HandleGetConfigurationDescriptor(sockfd, usb_request, control_request);
      break;
    case USB_DESCRIPTOR_STRING:
      HandleGetStringDescriptor(sockfd, usb_request, control_request);
      break;
    case USB_DESCRIPTOR_INTERFACE:
      break;
    case USB_DESCRIPTOR_ENDPOINT:
      break;
    case USB_DESCRIPTOR_DEVICE_QUALIFIER:
      break;
    default:
      printf("Unkown descriptor type requested: %d\n", control_request.wValue1);
  }
}

void UsbPrinter::HandleGetConfigurationDescriptor(
    int sockfd, const USBIP_CMD_SUBMIT& usb_request,
    const StandardDeviceRequest& control_request) const {
  printf("HandleGetConfigurationDescriptor %u[%u]\n", control_request.wValue1,
         control_request.wValue0);

  if (control_request.wLength == configuration_descriptor_.bLength) {
    // Only the configuration descriptor itself has been requested.
    printf("Only configuration descriptor requested\n");
    SendUsbRequest(sockfd, usb_request, (char*)&configuration_descriptor_,
                   control_request.wLength, 0);
    return;
  }

  // NOTE: This currently assumes that if the requested length was not just the
  // configuration descriptor then all of the descriptors have been requested.
  // TODO(daviev): If there are any instances where the client requests a size
  // other than the configuration descriptors bLength or wTotalLength then
  // change this function to address it.

  auto buf = std::make_unique<char[]>(control_request.wLength);
  int total = 0;

  // Place the configuration descriptor into the buffer.
  FillBuffer((char*)&configuration_descriptor_,
             configuration_descriptor_.bLength, buf.get() + total);
  total += configuration_descriptor_.bLength;

  // Place each of the corresponding interfaces into the buffer.
  for (int i = 0; i < configuration_descriptor_.bNumInterfaces; ++i) {
    auto interface = interfaces_[i];
    FillBuffer((char*)&interface, interface.bLength, buf.get() + total);
    total += interface.bLength;
    // TODO(daviev): For now this assumes that each interface has 2
    // corresponding endpoints. Update this in the future to support interfaces
    // that have only 1 endpoint.
    for (int j = i * 2; j < (i * 2) + 2; ++j) {
      auto endpoint = endpoints_[j];
      FillBuffer((char*)&endpoint, endpoint.bLength, buf.get() + total);
      total += endpoint.bLength;
    }
  }

  // After filling the buffer with all of the necessary descriptors we can send
  // a response.
  SendUsbRequest(sockfd, usb_request, buf.get(), control_request.wLength, 0);
}

void UsbPrinter::HandleGetStringDescriptor(
    int sockfd, const USBIP_CMD_SUBMIT& usb_request,
    const StandardDeviceRequest& control_request) const {
  printf("HandleGetStringDescriptor %u[%u]\n", control_request.wValue1,
         control_request.wValue0);

  int index = control_request.wValue0;
  int string_length = (strings_[index][0] / 2) - 1;
  if (index != 0) {
    char str[255];
    memset(str, 0, 255);
    for (int i = 0; i < string_length; ++i) {
      str[i] = strings_[index][i * 2 + 2];
    }
    printf("String (%s)\n", str);
  }
  SendUsbRequest(sockfd, usb_request, strings_[index].data(),
                 strings_[index][0], 0);
}

void UsbPrinter::HandleGetConfiguration(
    int sockfd, const USBIP_CMD_SUBMIT& usb_request,
    const StandardDeviceRequest& control_request) {
  printf("HandleGetConfiguration %u[%u]\n", control_request.wValue1,
         control_request.wValue0);

  // Note: For now we only have on configuration set, so we just respond with
  // with |configuration_descriptor_.bConfigurationValue|.
  SendUsbRequest(sockfd, usb_request,
                 (const char*)&configuration_descriptor_.bConfigurationValue, 1,
                 0);
}

void UsbPrinter::HandleSetConfiguration(
    int sockfd, const USBIP_CMD_SUBMIT& usb_request,
    const StandardDeviceRequest& control_request) {
  printf("HandleSetConfiguration %u[%u]\n", control_request.wValue1,
         control_request.wValue0);

  // NOTE: For now we have only one configuration to set, so we just respond
  // with an empty message as a confirmation.
  SendUsbRequest(sockfd, usb_request, 0, 0, 0);
}

void UsbPrinter::HandleSetInterface(
    int sockfd, const USBIP_CMD_SUBMIT& usb_request,
    const StandardDeviceRequest& control_request) {
  printf("HandleSetInterface %u[%u]\n", control_request.wValue1,
         control_request.wValue0);

  // NOTE: For now we have only one interface to set, so we just respond
  // with an empty message as a confirmation.
  SendUsbRequest(sockfd, usb_request, 0, 0, 0);
}

void UsbPrinter::HandleGetDeviceId(
    int sockfd, const USBIP_CMD_SUBMIT& usb_request,
    const StandardDeviceRequest& control_request) {
  printf("HandleGetDeviceId %u[%u]\n", control_request.wValue1,
         control_request.wValue0);

  SendUsbRequest(sockfd, usb_request, ieee_device_id_.data(),
                 ieee_device_id_.size(), 0);
}
