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

// Attempts to create the socket used for accepting connections on the server,
// and if successful returns the file descriptor of the socket.
int setup_server_socket();

// Binds the server socket described by |fd| to an address and returns the
// resulting sockaddr_in struct which contains the address.
struct sockaddr_in bind_server_socket(int fd);

// Accepts a new connection to the server described by |fd| and returns the file
// descriptor of the connection.
int accept_connection(int fd);

// Runs a simple server which processes USBIP requests.
void run_server(UsbPrinter printer);
