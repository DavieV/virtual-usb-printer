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

const char kUsbPath[] = "/sys/devices/pci0000:00/0000:00:01.2/usb1/1-1";
const char kBusId[] = "1-1";

#ifdef _DEBUG
void print_recv(char *buff, int size, const char *desc) {
  int i, j;

  printf("----------recv  %s (%i)-----------\n", desc, size);

  j = 1;
  for (i = 0; i < size; i++) {
    printf("0x%02X ", (unsigned char)buff[i]);
    if (j > 7) {
      printf("\n");
      j = 0;
    }
    j++;
  }

  printf("\n-------------------------\n");
}
#endif

// Sets the OP_HEADER |header| using the given values.
void set_op_header(word version, word command, int status, OP_HEADER *header) {
  header->version = version;
  header->command = command;
  header->status = status;
}

// Sets the OP_REP_DEVLIST_HEADER |devlist_header| using the given values.
void set_op_rep_devlist_header(word version, word command, int status,
                               int numExportedDevices,
                               OP_REP_DEVLIST_HEADER *devlist_header) {
  set_op_header(version, command, status, &devlist_header->header);
  devlist_header->numExportedDevices = numExportedDevices;
}

// Sets the OP_REP_DEVICE |device| using the corresponding values in
// |dev_dsc| and |config|.
void set_op_rep_device(const USB_DEVICE_DESCRIPTOR *dev_dsc,
                       const USB_CONFIGURATION_DESCRIPTOR *config,
                       OP_REP_DEVICE *device) {
  // Set constants.
  memset(device->usbPath, 0, 256);
  strcpy(device->usbPath, kUsbPath);
  memset(device->busID, 0, 32);
  strcpy(device->busID, kBusId);

  device->busnum=htonl(1);
  device->devnum=htonl(2);
  device->speed=htonl(2);

  // Set values using |dev_dsc|.
  device->idVendor = htons(dev_dsc->idVendor);
  device->idProduct = htons(dev_dsc->idProduct);
  device->bcdDevice = htons(dev_dsc->bcdDevice);
  device->bDeviceClass = dev_dsc->bDeviceClass;
  device->bDeviceSubClass = dev_dsc->bDeviceSubClass;
  device->bDeviceProtocol = dev_dsc->bDeviceProtocol;
  device->bNumConfigurations = dev_dsc->bNumConfigurations;

  // Set values using |config|.
  device->bConfigurationValue = config->bConfigurationValue;
  device->bNumInterfaces = config->bNumInterfaces;
}

// Assigns the values from |interfaces| into |rep_interfaces|.
void set_op_rep_devlist_interfaces(const USB_INTERFACE_DESCRIPTOR *interfaces[],
                                   OP_REP_DEVLIST_INTERFACE **rep_interfaces,
                                   byte num_interfaces) {
  *rep_interfaces = malloc(num_interfaces * sizeof(OP_REP_DEVLIST_INTERFACE));
  for (int i = 0; i < num_interfaces; i++) {
    (*rep_interfaces)[i].bInterfaceClass = interfaces[i]->bInterfaceClass;
    (*rep_interfaces)[i].bInterfaceSubClass = interfaces[i]->bInterfaceSubClass;
    (*rep_interfaces)[i].bInterfaceProtocol = interfaces[i]->bInterfaceProtocol;
    (*rep_interfaces)[i].padding = 0;
  }
}

// Creates the OP_REP_DEVLIST message used to respond to a request to list the
// host's exported USB devices.
void create_op_rep_devlist(const USB_DEVICE_DESCRIPTOR *dev_dsc,
                           const USB_CONFIGURATION_DESCRIPTOR *config,
                           const USB_INTERFACE_DESCRIPTOR *interfaces[],
                           OP_REP_DEVLIST *list) {
  set_op_rep_devlist_header(htons(273), htons(5), 0, htonl(1), &list->header);
  set_op_rep_device(dev_dsc, config, &list->device);
  set_op_rep_devlist_interfaces(interfaces, &list->interfaces,
                                config->bNumInterfaces);
}

// Handles an OP_REQ_DEVLIST request by sending an OP_REP_DEVLIST message which
// describes the virtual USB device along |sockfd|.
void handle_device_list(const USB_DEVICE_DESCRIPTOR *dev_dsc, int sockfd) {
  OP_REP_DEVLIST list;
  printf("list devices\n");

  CONFIG_GEN *conf = (CONFIG_GEN *)configuration;
  create_op_rep_devlist(dev_dsc, &conf->dev_conf, interfaces, &list);

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

// Creates the OP_REP_IMPORT message used to respond to a request to attach a
// host USB device.
void create_op_rep_import(const USB_DEVICE_DESCRIPTOR *dev_dsc,
                          const USB_CONFIGURATION_DESCRIPTOR *config,
                          OP_REP_IMPORT *rep) {
  set_op_header(htons(273), htons(3), 0, &rep->header);
  set_op_rep_device(dev_dsc, config, &rep->device);
}

// Handles and OP_REQ_IMPORT request by sending an OP_REP_IMPORT message which
// describes the virtual USB device along |sockfd|.
int handle_attach(const USB_DEVICE_DESCRIPTOR *dev_dsc, int sockfd) {
  char busid[32];
  OP_REP_IMPORT rep;
  printf("attach device\n");
  ssize_t received = recv(sockfd, busid, 32, 0);
  if (received != 32) {
    printf("receive error : %s \n", strerror(errno));
    return 1;
  }
#ifdef _DEBUG
  print_recv(busid, 32, "Busid");
#endif
  CONFIG_GEN *conf = (CONFIG_GEN *)configuration;
  create_op_rep_import(dev_dsc, &conf->dev_conf, &rep);
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

// Converts the contents of either a USBIP_CMD_SUBMIT or USB_RET_SUBMIT message
// into network byte order.
void pack(int *data, size_t msg_size) {
  int size = msg_size / 4;
  for (int i = 0; i < size; i++) {
    data[i] = htonl(data[i]);
  }
  // Put |setup| into network byte order. Since |setup| is a 64-bit integer we
  // have to swap the final 2 int entries since they are both a part of |setup|.
  swap(&data[size - 1], &data[size - 2]);
}

// Converts the contents of either a USBIP_CMD_SUBMIT or USB_RET_SUBMIT message
// into host byte order.
void unpack(int *data, size_t msg_size) {
  int size = msg_size / 4;
  for (int i = 0; i < size; i++) {
    data[i] = ntohl(data[i]);
  }
  // Put |setup| into host byte order. Since |setup| is a 64-bit integer we
  // have to swap the final 2 int entries since they are both a part of |setup|.
  swap(&data[size - 1], &data[size - 2]);
}

void print_usbip_cmd_submit(const USBIP_CMD_SUBMIT *command) {
  printf("usbip cmd %u\n", command->command);
  printf("usbip seqnum %u\n", command->seqnum);
  printf("usbip devid %u\n", command->devid);
  printf("usbip direction %u\n", command->direction);
  printf("usbip ep %u\n", command->ep);
  printf("usbip flags %u\n", command->transfer_flags);
  printf("usbip number of packets %u\n", command->number_of_packets);
  printf("usbip interval %u\n", command->interval);
  printf("usbip setup %llu\n", command->setup);
  printf("usbip buffer length  %u\n", command->transfer_buffer_length);
}

void print_standard_device_request(const StandardDeviceRequest *request) {
  printf("  UC Request Type %u\n", request->bmRequestType);
  printf("  UC Request %u\n", request->bRequest);
  printf("  UC Value  %u[%u]\n", request->wValue1, request->wValue0);
  printf("  UC Index  %u-%u\n", request->wIndex1, request->wIndex0);
  printf("  UC Length %u\n", request->wLength);
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

// Sends a USBIP_RET_SUBMIT message. |usb_req| contains the metadata for the
// message and |data| contains the actual URB data bytes.
void send_usb_req(int sockfd, USBIP_RET_SUBMIT *usb_req, char *data,
                  unsigned int data_size, unsigned int status) {
  usb_req->command = 0x3;
  // TODO(daviev): Figure out why seqnum isn't set.
  usb_req->devid = 0x0;
  usb_req->direction = 0x0;
  usb_req->ep = 0x0;
  usb_req->status = status;
  usb_req->actual_length = data_size;
  usb_req->start_frame = 0x0;
  usb_req->number_of_packets = 0x0;
  // TODO(daviev): Figure out why error count isn't set.
  usb_req->setup = 0x0;

  size_t request_size = sizeof(*usb_req);
  pack((int *)usb_req, request_size);

  if (send(sockfd, (char *)usb_req, request_size, 0) != request_size) {
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

// Handles the USB request for the string descriptor.
void handle_get_string_descriptor(int sockfd,
                                  StandardDeviceRequest *control_request,
                                  USBIP_RET_SUBMIT *usb_request) {
  if (control_request->wValue0 != 0) {
    char str[255];
    memset(str, 0, 255);
    int string_length = (*strings[control_request->wValue0] / 2) - 1;
    for (int i = 0; i < string_length; ++i) {
      str[i] = strings[control_request->wValue0][i * 2 + 2];
    }
    printf("String (%s)\n", str);
  }
  send_usb_req(sockfd, usb_request, (char *)strings[control_request->wValue0],
               *strings[control_request->wValue0], 0);
}

void handle_get_descriptor(int sockfd, StandardDeviceRequest *control_request,
                           USBIP_RET_SUBMIT *usb_request) {
  printf("handle_get_descriptor %u[%u]\n", control_request->wValue1,
         control_request->wValue0);

  // TODO(daviev): Provide better documentation for the special class-based
  // requests.

  switch (control_request->wValue1) {
    case USB_DESCRIPTOR_DEVICE:
      printf("Device\n");
      send_usb_req(sockfd, usb_request, (char *)&dev_dsc,
                   control_request->wLength, 0);
      break;
    case USB_DESCRIPTOR_CONFIGURATION:
      printf("Configuration\n");
      send_usb_req(sockfd, usb_request, (char *)configuration,
                   control_request->wLength, 0);
      break;
    case USB_DESCRIPTOR_STRING:
      handle_get_string_descriptor(sockfd, control_request, usb_request);
      break;
    case USB_DESCRIPTOR_DEVICE_QUALIFIER:
      printf("Qualifier\n");
      send_usb_req(sockfd, usb_request, (char *)&dev_qua,
                   control_request->wLength, 0);
    default:
      printf("Unknown\n");
      send_usb_req(sockfd, usb_request, "", 0, 1);
      break;
  }
}

void handle_get_interface(int sockfd, StandardDeviceRequest *control_request,
                          USBIP_RET_SUBMIT *usb_request) {
  printf("handle_get_interface %u[%u]\n", control_request->wValue1,
         control_request->wValue0);
  send_usb_req(sockfd, usb_request, (char *)interfaces[0],
               control_request->wLength, 0);
}

// Responds to a GET_STATUS request.
void handle_get_status(int sockfd, StandardDeviceRequest *control_request,
                       USBIP_RET_SUBMIT *usb_request) {
  // TODO(daviev): This function needs to switch on |control_request->wIndex| in
  // order to determine whether to return the status for device, interface, or
  // endpoint.
  printf("handle_get_status %u[%u]\n", control_request->wValue1,
         control_request->wValue0);
  char data[2];
  data[0] = 0x01;
  data[1] = 0x00;
  send_usb_req(sockfd, usb_request, data, 2, 0);
}

void handle_set_configuration(int sockfd, StandardDeviceRequest *control_req,
                             USBIP_RET_SUBMIT *usb_req) {
  printf("handle_set_configuration %u[%u]\n", control_req->wValue1,
         control_req->wValue0);
  send_usb_req(sockfd, usb_req, "", 0, 0);
}

void handle_set_interface(int sockfd, StandardDeviceRequest *control_request,
                          USBIP_RET_SUBMIT *usb_request) {
  printf("handle_set_interface %u[%u]\n", control_request->wValue1,
         control_request->wValue0);
  send_usb_req(sockfd, usb_request, "", 0, 1);
}

//http://www.usbmadesimple.co.uk/ums_4.htm

// Fills the corresponding StandardDeviceRequest values of |request| using
// |setup| which represents a USB SETUP packet.
void create_standard_device_request(long long setup,
                                    StandardDeviceRequest *request) {
  request->bmRequestType = (setup & 0xFF00000000000000) >> 56;
  request->bRequest = (setup & 0x00FF000000000000) >> 48;
  request->wValue0 = (setup & 0x0000FF0000000000) >> 40;
  request->wValue1 = (setup & 0x000000FF00000000) >> 32;
  request->wIndex0 = (setup & 0x00000000FF000000) >> 24;
  request->wIndex1 = (setup & 0x0000000000FF0000) >> 16;
  request->wLength = ntohs(setup & 0x000000000000FFFF);
}

int is_hid_request(const StandardDeviceRequest *control_request) {
  // For a HID request, |control_request->bmRequestType| should always have bit
  // 5 and bit 0 set.
  // 00100001d = 0x21
  return (control_request->bmRequestType & 0x21) == 0x21;
}

void handle_usb_control(int sockfd, USBIP_RET_SUBMIT *usb_request) {
  StandardDeviceRequest control_request;
  // Convert |usb_request->setup| into a StandardDeviceRequest.
  create_standard_device_request(usb_request->setup, &control_request);
  print_standard_device_request(&control_request);

  if (is_hid_request(&control_request)) {
    handle_hid_request(sockfd, &control_request, usb_request);
    return;
  }

  // Special HID GET_DESCRIPTOR request.
  if (control_request.bmRequestType == 0x81 &&
      control_request.bRequest == GET_DESCRIPTOR) {
    printf("Special Request\n");
    handle_hid_get_descriptor(sockfd, &control_request, usb_request);
    return;
  }

  switch (control_request.bRequest) {
    case GET_DESCRIPTOR:
      handle_get_descriptor(sockfd, &control_request, usb_request);
      break;
    case GET_INTERFACE:
      handle_get_interface(sockfd, &control_request, usb_request);
      break;
    case GET_STATUS:
      handle_get_status(sockfd, &control_request, usb_request);
      break;
    case SET_CONFIGURATION:
      handle_set_configuration(sockfd, &control_request, usb_request);
      break;
    case SET_INTERFACE:
      handle_set_interface(sockfd, &control_request, usb_request);
    case GET_CONFIGURATION:
    default:
      printf("Unknown control request\n");
      break;
  }
}

void handle_usb_request(int sockfd, USBIP_RET_SUBMIT *ret, int bl) {
  if (ret->ep == 0) {
    printf("#control requests\n");
    handle_usb_control(sockfd, ret);
  } else {
    printf("#data requests\n");
    handle_data(sockfd, ret, bl);
  }
}

// Attempts to create the socket used for accepting connections on the server,
// and if successful returns the file descriptor of the socket.
int setup_server_socket() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    printf("socket error : %s\n", strerror(errno));
    exit(1);
  }

  int reuse = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    perror("setsockopt(SO_REUSEADDR) failed");
  }

  return fd;
}

// Binds the server socket described by |fd| to an address and returns the
// resulting sockaddr_in struct which contains the address.
struct sockaddr_in bind_server_socket(int fd) {
  struct sockaddr_in server;
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(TCP_SERV_PORT);

  if (bind(fd, (sockaddr *)&server, sizeof(server)) < 0) {
    printf("bind error : %s \n", strerror(errno));
    exit(1);
  }

  return server;
}

// Accepts a new connection to the server described by |fd| and returns the file
// descriptor of the connection.
int accept_connection(int fd) {
  struct sockaddr_in client;
  socklen_t client_length = sizeof(client);
  int connection = accept(fd, (struct sockaddr *)&client, &client_length);
  if (connection < 0) {
    printf("accept error : %s\n", strerror(errno));
    exit(1);
  }
  printf("Connection address:%s\n", inet_ntoa(client.sin_addr));
  return connection;
}

// Simple TCP server.
void usbip_run(const USB_DEVICE_DESCRIPTOR *dev_dsc) {
  int listenfd = setup_server_socket();
  struct sockaddr_in server = bind_server_socket(listenfd);
  char address[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &server.sin_addr, address, INET_ADDRSTRLEN)) {
    printf("Bound server to address %s:%hu\n", address, server.sin_port);
  } else {
    printf("inet_ntop error : %s\n", strerror(errno));
    exit(1);
  }

  if (listen(listenfd, SOMAXCONN) < 0) {
    printf("listen error : %s\n", strerror(errno));
    exit(1);
  }

  for (;;) {
    int connection = accept_connection(listenfd);
    int attached = 0;

    while (1) {
      if (!attached) {
        // Read in the header first in order to determine whether the request is
        // an OP_REQ_DEVLIST or an OP_REQ_IMPORT.
        OP_HEADER request;
        ssize_t received = recv(connection, &request, sizeof(request), 0);
        if (received != sizeof(request)) {
          printf("receive error : %s\n", strerror (errno));
          break;
        }
#ifdef _DEBUG
        print_recv((char *)&request, sizeof(request), "OP_REQ_DEVLIST");
#endif
        request.command = ntohs(request.command);
        printf("Header Packet\n");
        printf("command: 0x%02X\n", request.command);

        // Request is OP_REQ_DEVLIST 
        if (request.command == OP_REQ_DEVLIST_CMD) {
          handle_device_list(dev_dsc, connection);
        // Request is OP_REQ_IMPORT
        } else if (request.command == OP_REQ_IMPORT_CMD) {
          if (!handle_attach(dev_dsc, connection)) {
            attached = 1;
          }
        }
      } else {
        printf("------------------------------------------------\n");
        printf("handles requests\n");
        USBIP_CMD_SUBMIT command;
        ssize_t received = recv(connection, &command, sizeof(command), 0);
        if (received != sizeof(command)) {
          printf("receive error : %s\n", strerror(errno));
          break;
        }
#ifdef _DEBUG
        print_recv((char *)&command, sizeof(command), "USBIP_CMD_SUBMIT");
#endif
        unpack((int *)&command, sizeof(command));
        print_usbip_cmd_submit(&command);
        USBIP_RET_SUBMIT usb_req = create_usbip_ret_submit(&command);

        if (command.command == 1) {
          handle_usb_request(connection, &usb_req,
                             command.transfer_buffer_length);
        }

        // Unlink URB
        if (command.command == 2) {
          printf("####################### Unlink URB %u  (not working!!!)\n",
                 command.transfer_flags);
          // FIXME
          /*
           USBIP_RET_UNLINK ret;
           printf("####################### Unlink URB %u\n",cmd.transfer_flags);
           ret.command=htonl(0x04);
           ret.devid=htonl(cmd.devid);
           ret.direction=htonl(cmd.direction);
           ret.ep=htonl(cmd.ep);
           ret.seqnum=htonl(cmd.seqnum);
           ret.status=htonl(1);

           if (send (sockfd, (char *)&ret, sizeof(USBIP_RET_UNLINK), 0) !=
           sizeof(USBIP_RET_UNLINK))
           {
             printf ("send error : %s \n", strerror (errno));
             exit(-1);
           }
          */
        }

        if (command.command > 2) {
          printf("Unknown USBIP cmd!\n");
          close(connection);
          return;
        }
      }
    }
    close(connection);
  }
}
