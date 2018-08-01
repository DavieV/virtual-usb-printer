CC=g++ -std=c++14
CFLAGS= -Wall -DLINUX 

all: main

main: usbip.o usb_printer.o server.o main.cc
	${CC} ${CFLAGS} usbip.o usb_printer.o server.o main.cc -o main

usbip.o: usbip.cc
	${CC} ${CFLAGS} -c usbip.cc

usb_printer.o: usbip.o usb_printer.cc
	${CC} ${CFLAGS} -c usb_printer.cc

server.o: usbip.o usb_printer.o server.cc
	${CC} ${CFLAGS} -c server.cc

clean:
	rm -f ${PROGS} core core.* *.o temp.* *.out typescript*
