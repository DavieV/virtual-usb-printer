/* ########################################################################

   USBIP hardware emulation 

   ########################################################################

   Copyright (c) : 2016  Luis Claudio Gambôa Lopes

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   For e-mail suggestions :  lcgamboa@yahoo.com
   ######################################################################## */

//system headers dependent

#include "usbip.h"

#include "device_descriptors.h"
#include "usbip-constants.h"
#include "usb_printer.h"

const char kUsbPath[] = "/sys/devices/pci0000:00/0000:00:01.2/usb1/1-1";
const char kBusId[] = "1-1";

void set_op_header(word version, word command, int status, OP_HEADER *header) {
  header->version = version;
  header->command = command;
  header->status = status;
}

void set_op_rep_devlist_header(word version, word command, int status,
                               int numExportedDevices,
                               OP_REP_DEVLIST_HEADER *devlist_header) {
  set_op_header(version, command, status, &devlist_header->header);
  devlist_header->numExportedDevices = numExportedDevices;
}

// Sets the members of |device| using the corresponding values in
// |device| and |config|.
void set_op_rep_device(const USB_DEVICE_DESCRIPTOR& dev_dsc,
                       const USB_CONFIGURATION_DESCRIPTOR& config,
                       OP_REP_DEVICE* device) {
  // Set constants.
  memset(device->usbPath, 0, 256);
  strcpy(device->usbPath, kUsbPath);
  memset(device->busID, 0, 32);
  strcpy(device->busID, kBusId);

  device->busnum=htonl(1);
  device->devnum=htonl(2);
  device->speed=htonl(2);

  // Set values using |device|.
  device->idVendor = htons(dev_dsc.idVendor);
  device->idProduct = htons(dev_dsc.idProduct);
  device->bcdDevice = htons(dev_dsc.bcdDevice);
  device->bDeviceClass = dev_dsc.bDeviceClass;
  device->bDeviceSubClass = dev_dsc.bDeviceSubClass;
  device->bDeviceProtocol = dev_dsc.bDeviceProtocol;
  device->bNumConfigurations = dev_dsc.bNumConfigurations;

  // Set values using |config|.
  device->bConfigurationValue = config.bConfigurationValue;
  device->bNumInterfaces = config.bNumInterfaces;
}

void set_op_rep_devlist_interfaces(
    const std::vector<USB_INTERFACE_DESCRIPTOR>& interfaces,
    OP_REP_DEVLIST_INTERFACE **rep_interfaces) {
  // TODO(daviev): Change this to use a smart pointer at some point.
  *rep_interfaces = (OP_REP_DEVLIST_INTERFACE *)malloc(
      interfaces.size() * sizeof(OP_REP_DEVLIST_INTERFACE));
  for (size_t i = 0; i < interfaces.size(); ++i) {
    const auto& interface = interfaces[i];
    (*rep_interfaces)[i].bInterfaceClass = interface.bInterfaceClass;
    (*rep_interfaces)[i].bInterfaceSubClass = interface.bInterfaceSubClass;
    (*rep_interfaces)[i].bInterfaceProtocol = interface.bInterfaceProtocol;
    (*rep_interfaces)[i].padding = 0;
  }
}

// Creates the OP_REP_DEVLIST message used to respond to a request to list the
// host's exported USB devices.
void create_op_rep_devlist(
    const USB_DEVICE_DESCRIPTOR& device,
    const USB_CONFIGURATION_DESCRIPTOR& config,
    const std::vector<USB_INTERFACE_DESCRIPTOR>& interfaces,
    OP_REP_DEVLIST *list) {
  set_op_rep_devlist_header(htons(273), htons(5), 0, htonl(1), &list->header);
  set_op_rep_device(device, config, &list->device);
  set_op_rep_devlist_interfaces(interfaces, &list->interfaces);
}

void handle_device_list(const UsbPrinter& printer, int sockfd) {
  OP_REP_DEVLIST list;
  printf("list devices\n");

  create_op_rep_devlist(printer.device_descriptor(),
                        printer.configuration_descriptor(),
                        printer.interfaces(), &list);

  ssize_t sent = send(sockfd, &list.header, sizeof(list.header), 0);
  if (sent != sizeof(OP_REP_DEVLIST_HEADER)) {
    printf("send error : %s \n", strerror(errno));
    goto cleanup;
  }

  sent = send(sockfd, &list.device, sizeof(list.device), 0);
  if (sent != sizeof(list.device)) {
    printf("send error : %s \n", strerror(errno));
    goto cleanup;
  }

  sent = send(sockfd, list.interfaces,
              sizeof(*list.interfaces) * list.device.bNumInterfaces, 0);
  if (sent != sizeof(*list.interfaces) * list.device.bNumInterfaces) {
    printf("send error : %s \n", strerror(errno));
    goto cleanup;
  }

cleanup:
  free(list.interfaces);
}

void create_op_rep_import(const USB_DEVICE_DESCRIPTOR& dev_dsc,
                          const USB_CONFIGURATION_DESCRIPTOR& config,
                          OP_REP_IMPORT *rep) {
  set_op_header(htons(273), htons(3), 0, &rep->header);
  set_op_rep_device(dev_dsc, config, &rep->device);
}

int handle_attach(const UsbPrinter& printer, int sockfd) {
  char busid[32];
  OP_REP_IMPORT rep;
  printf("attach device\n");
  ssize_t received = recv(sockfd, busid, 32, 0);
  if (received != 32) {
    printf("receive error : %s \n", strerror(errno));
    return 1;
  }
  create_op_rep_import(printer.device_descriptor(),
                       printer.configuration_descriptor(), &rep);
  ssize_t sent = send(sockfd, &rep, sizeof(rep), 0);
  if (sent != sizeof(rep)) {
    printf("send error : %s \n", strerror(errno));
    return 1;
  }
  return 0;
}

void swap(int *a, int *b) {
  int tmp = *a;
  *a = *b;
  *b = tmp;
}

void pack_usbip(int *data, size_t msg_size) {
  int size = msg_size / 4;
  for (int i = 0; i < size; i++) {
    data[i] = htonl(data[i]);
  }
  // Put |setup| into network byte order. Since |setup| is a 64-bit integer we
  // have to swap the final 2 int entries since they are both a part of |setup|.
  swap(&data[size - 1], &data[size - 2]);
}

void unpack_usbip(int *data, size_t msg_size) {
  int size = msg_size / 4;
  for (int i = 0; i < size; i++) {
    data[i] = ntohl(data[i]);
  }
  // Put |setup| into host byte order. Since |setup| is a 64-bit integer we
  // have to swap the final 2 int entries since they are both a part of |setup|.
  swap(&data[size - 1], &data[size - 2]);
}

void print_usbip_cmd_submit(const USBIP_CMD_SUBMIT& command) {
  printf("usbip cmd %u\n", command.command);
  printf("usbip seqnum %u\n", command.seqnum);
  printf("usbip devid %u\n", command.devid);
  printf("usbip direction %u\n", command.direction);
  printf("usbip ep %u\n", command.ep);
  printf("usbip flags %u\n", command.transfer_flags);
  printf("usbip number of packets %u\n", command.number_of_packets);
  printf("usbip interval %u\n", command.interval);
  printf("usbip setup %llu\n", command.setup);
  printf("usbip buffer length  %u\n", command.transfer_buffer_length);
}

void print_standard_device_request(const StandardDeviceRequest& request) {
  printf("  UC Request Type %u\n", request.bmRequestType);
  printf("  UC Request %u\n", request.bRequest);
  printf("  UC Value  %u[%u]\n", request.wValue1, request.wValue0);
  printf("  UC Index  %u-%u\n", request.wIndex1, request.wIndex0);
  printf("  UC Length %u\n", request.wLength);
}

// Creates a new USBIP_RET_SUBMIT which is initialized using the shared values
// from |command|.
USBIP_RET_SUBMIT create_usbip_ret_submit(const USBIP_CMD_SUBMIT *command) {
  USBIP_RET_SUBMIT usb_req;
  memset(&usb_req, 0, sizeof(usb_req));
  usb_req.seqnum = command->seqnum;
  usb_req.devid = command->devid;
  usb_req.direction = command->direction;
  usb_req.ep = command->ep;
  usb_req.setup = command->setup;
  return usb_req;
}

USBIP_RET_SUBMIT CreateUsbipRetSubmit(const USBIP_CMD_SUBMIT& request) {
  USBIP_RET_SUBMIT response;
  memset(&response, 0, sizeof(response));
  response.command = COMMAND_USBIP_RET_SUBMIT;
  response.seqnum = request.seqnum;
  response.devid = request.devid;
  response.direction = request.direction;
  response.ep = request.ep;
  return response;
}

void SendUsbRequest(int sockfd, const USBIP_CMD_SUBMIT& usb_request,
                    const char* data, unsigned int data_size,
                    unsigned int status) {
  USBIP_RET_SUBMIT response = CreateUsbipRetSubmit(usb_request);
  response.status = status;
  response.actual_length = data_size;
  response.start_frame = 0;
  // TODO(daviev): Figure out what this means.
  response.number_of_packets = 0;
  response.error_count = 0;

  if (data_size > 0) {
    printf("Sending buffer: \n");
    for (int i = 0; i < data_size; ++i) {
      printf("%u ", data[i]);
    }
    printf("\n");
  }

  size_t response_size = sizeof(response);
  pack_usbip((int*)&response, response_size);

  if (send(sockfd, (char*)&response, response_size, 0) != response_size) {
    printf("send error : %s \n", strerror(errno));
    exit(-1);
  }

  // Skip sending data if there isn't any.
  if (data_size == 0) {
    return;
  }

  if (send(sockfd, data, data_size, 0) != data_size) {
    printf("send error : %s \n", strerror(errno));
    exit(-1);
  }
}
