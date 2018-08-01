#include "usbip.h"
#include "usbip-constants.h"
#include "device_descriptors.h"
#include "usb_printer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

void run_server(UsbPrinter printer) {
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

  while (1) {
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

        request.command = ntohs(request.command);
        printf("Header Packet\n");
        printf("command: 0x%02X\n", request.command);

        // Request is OP_REQ_DEVLIST 
        if (request.command == OP_REQ_DEVLIST_CMD) {
          handle_device_list(printer, connection);
        // Request is OP_REQ_IMPORT
        } else if (request.command == OP_REQ_IMPORT_CMD) {
          if (!handle_attach(printer, connection)) {
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

        unpack_usbip((int *)&command, sizeof(command));
        print_usbip_cmd_submit(command);

        if (command.command == 1) {
          printer.HandleUsbRequest(connection, command);
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
  }
}
