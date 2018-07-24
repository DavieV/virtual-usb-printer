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

#include "device_descriptors.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#define min(a, b) ((a) < (b) ? (a) : (b))

// system headers independent
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Define constants
#define TCP_SERV_PORT 3240
#define OP_REQ_DEVLIST_CMD 0x8005
#define OP_REQ_IMPORT_CMD 0x8003

typedef struct sockaddr sockaddr;

/*
 * Structures used by the USBIP protocol for communication.
 * Documentation for these structs can be found in doc/usbip_protocol.txt
 */

// USBIP data struct

// Contains the header values that are contained within all of the "OP" messages
// used by usbip.
typedef struct __attribute__((__packed__)) _OP_HEADER {
  word version;
  word command;
  int status;
} OP_HEADER;

// Generic device descriptor used by OP_REP_DEVLIST and OP_REP_IMPORT.
typedef struct __attribute__((__packed__)) _OP_REP_DEVICE {
  char usbPath[256];
  char busID[32];
  int busnum;
  int devnum;
  int speed;
  word idVendor;
  word idProduct;
  word bcdDevice;
  byte bDeviceClass;
  byte bDeviceSubClass;
  byte bDeviceProtocol;
  byte bConfigurationValue;
  byte bNumConfigurations;
  byte bNumInterfaces;
} OP_REP_DEVICE;

// The OP_REQ_DEVLIST message contains the same information as OP_HEADER.
typedef OP_HEADER OP_REQ_DEVLIST; 

typedef struct __attribute__((__packed__)) _OP_REP_DEVLIST_HEADER {
  OP_HEADER header;
  int numExportedDevices;
} OP_REP_DEVLIST_HEADER;

//================= for each device
typedef OP_REP_DEVICE OP_REP_DEVLIST_DEVICE;

//================== for each interface
typedef struct __attribute__((__packed__)) _OP_REP_DEVLIST_INTERFACE {
  byte bInterfaceClass;
  byte bInterfaceSubClass;
  byte bInterfaceProtocol;
  byte padding;
} OP_REP_DEVLIST_INTERFACE;

typedef struct __attribute__((__packed__)) _OP_REP_DEVLIST {
  OP_REP_DEVLIST_HEADER header;
  OP_REP_DEVLIST_DEVICE device;  // only one!
  OP_REP_DEVLIST_INTERFACE *interfaces;
} OP_REP_DEVLIST;

typedef struct __attribute__((__packed__)) _OP_REQ_IMPORT {
  OP_HEADER header;
  char busID[32];
} OP_REQ_IMPORT;

typedef struct __attribute__((__packed__)) _OP_REP_IMPORT {
  OP_HEADER header;
  //------------- if not ok, finish here
  OP_REP_DEVICE device;
} OP_REP_IMPORT;

typedef struct __attribute__((__packed__)) _USBIP_CMD_SUBMIT {
  int command;
  int seqnum;
  int devid;
  int direction;
  int ep;
  int transfer_flags;
  int transfer_buffer_length;
  int start_frame;
  int number_of_packets;
  int interval;
  long long setup;  // Contains a USB SETUP packet.
} USBIP_CMD_SUBMIT;

/*
+  Allowed transfer_flags  | value      | control | interrupt | bulk     |
isochronous
+
-------------------------+------------+---------+-----------+----------+-------------
+  URB_SHORT_NOT_OK        | 0x00000001 | only in | only in   | only in  | no
+  URB_ISO_ASAP            | 0x00000002 | no      | no        | no       | yes
+  URB_NO_TRANSFER_DMA_MAP | 0x00000004 | yes     | yes       | yes      | yes
+  URB_NO_FSBR             | 0x00000020 | yes     | no        | no       | no
+  URB_ZERO_PACKET         | 0x00000040 | no      | no        | only out | no
+  URB_NO_INTERRUPT        | 0x00000080 | yes     | yes       | yes      | yes
+  URB_FREE_BUFFER         | 0x00000100 | yes     | yes       | yes      | yes
+  URB_DIR_MASK            | 0x00000200 | yes     | yes       | yes      | yes
*/

typedef struct __attribute__((__packed__)) _USBIP_RET_SUBMIT {
  int command;
  int seqnum;
  int devid;
  int direction;
  int ep;
  int status;
  int actual_length;
  int start_frame;
  int number_of_packets;
  int error_count;
  long long setup;
} USBIP_RET_SUBMIT;

typedef struct __attribute__((__packed__)) _USBIP_CMD_UNLINK {
  int command;
  int seqnum;
  int devid;
  int direction;
  int ep;
  int seqnum_urb;
} USBIP_CMD_UNLINK;

typedef struct __attribute__((__packed__)) _USBIP_RET_UNLINK {
  int command;
  int seqnum;
  int devid;
  int direction;
  int ep;
  int status;
} USBIP_RET_UNLINK;

// Represents a USB SETUP packet.
typedef struct __attribute__((__packed__)) _StandardDeviceRequest {
  byte bmRequestType;
  byte bRequest;
  byte wValue0;
  byte wValue1;
  byte wIndex0;
  byte wIndex1;
  word wLength;
} StandardDeviceRequest;

// Utility functions
void set_op_rep_devlist_header(word version, word command, int status,
                               int numExportedDevices,
                               OP_REP_DEVLIST_HEADER *header);

void send_usb_req(int sockfd, USBIP_RET_SUBMIT *usb_req, char *data,
                  unsigned int size, unsigned int status);
void usbip_run(const USB_DEVICE_DESCRIPTOR *dev_dsc);

// implemented by user
extern const USB_DEVICE_DESCRIPTOR dev_dsc;
extern const USB_DEVICE_QUALIFIER_DESCRIPTOR dev_qua;
extern const char *configuration;
extern const USB_INTERFACE_DESCRIPTOR *interfaces[];
extern const unsigned char *strings[];

void handle_data(int sockfd, USBIP_RET_SUBMIT *usb_req, int bl);
void handle_hid_request(int sockfd,
                        const StandardDeviceRequest *control_request,
                        USBIP_RET_SUBMIT *usb_request);
void handle_hid_get_descriptor(int sockfd,
                               const StandardDeviceRequest *control_request,
                               USBIP_RET_SUBMIT *usb_request);
