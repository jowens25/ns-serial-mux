#include "main.h"
#include "socket_serial.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include "circular_buffer.h"
//
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

struct termios tty;
int clients[MAX_CONNECTIONS] = {-1, -1, -1, -1};
int max_fd;
int debug = false;

char SERIAL_PORT[SERIAL_PORT_LEN] = {0};




// socket is ready, read from it
void handle_client_data(int client, int listener, fd_set *master, int fdmax)
{
    printf("before read socket \n");

    readSocket(client, master);


    char tx[BUFFER_SIZE];
    int bytes_read = cb_read_chunk(&ser_cb, tx, BUFFER_SIZE);
    int n = write(client, tx, bytes_read);

    
}



void handle_new_connection(int listener, fd_set *master, int *fdmax)
{
	int newfd;        // newly accept()ed socket descriptor
    newfd = accept(listener, NULL, NULL);

	if (newfd == -1) {
		perror("accept");
	} else {
		FD_SET(newfd, master); // add to master set
		if (newfd > *fdmax) {  // keep track of the max
			*fdmax = newfd;
		}
		printf("selectserver: new connection on socket %d\n", newfd);
	}
}

// serial port is ready, read from it, write to it.
void handle_serial(int serial_fd, fd_set *master, int fdmax)
{

    //printf("handle serial\n");
    // pull data into the buffer to be sent to the sockets
    readSerial(serial_fd);

    // pull data from the socket buffer and send it to the serial port
    writeSerial(serial_fd);
}



int main(int argc, char *argv[])
{

    if (argv[1] == NULL)
    {
        printf("Please include a serial port\r\n");
        printf("example: %s /dev/ttymxc2\r\n", argv[0]);
        exit(-1);
    }

    if (argv[2] != NULL) {

        if ( strncmp(argv[2], "debug", 5) == 0) {
            debug = true;
            printf("DEBUGGING\r\n");
        }

    }


    fd_set read_fds;
    fd_set master;
    FD_ZERO(&read_fds);
    FD_ZERO(&master);
    int fdmax;
    int listener_fd;
    int serial_fd;



    strncpy(SERIAL_PORT, argv[1], strlen(argv[1]));
    int ser = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_SYNC);
    if (ser < 0)
    {
        perror("unable to open serial port");
        return -1;
    }

    serial_fd = serialSetup(ser);
    FD_SET(serial_fd, &master);


    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("unable to create socket");
        close(sock);
        return -1;
    }
    listener_fd = socketSetup(sock);
    FD_SET(listener_fd, &master);


    fdmax = listener_fd > serial_fd ? listener_fd : serial_fd;

    char tx[BUFFER_SIZE];
    cb_init(&ser_cb);
    cb_init(&sock_cb);

    while (1)
    {
        read_fds = master; // copy it
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}

		// run through the existing connections looking for data
		// to read
		for(int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) { // we got one!!
				if (i == listener_fd) { // new connection available
					handle_new_connection(i, &master, &fdmax);
                }
                
                else if (i == serial_fd) { // serial ready!
                    handle_serial(serial_fd, &master, fdmax);
                }
				
                else { // a socket file descriptor is ready!
                    printf("running for: %d\n", i);
					handle_client_data(i, listener_fd, &master, fdmax);



                }
			}
		}
	}



    close(ser);
    close(sock);
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (clients[i] != -1)
            close(clients[i]);
    }


    return 0;
}
