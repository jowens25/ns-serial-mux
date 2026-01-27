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
//#include "circular_buffer.h"
//

#define SOCKET_PATH "/tmp/serial.sock"

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
//
struct termios tty;
//int clients[MAX_CONNECTIONS] = {-1, -1, -1, -1};
//int max_fd;
int debug = false;
char SERIAL_PORT[SERIAL_PORT_LEN] = {0};




void broadcast(char *buf, int nbytes, int serial, int listener, int s, fd_set *master, int fdmax)
{
    for(int j = 0; j <= fdmax; j++) {
        // send to everyone!
        if (FD_ISSET(j, master)) {
            // except the listener and ourselves
            if (j != listener && j != s && j != serial) {
                if (send(j, buf, nbytes, 0) == -1) {
                    perror("send");
                }
            }
        }
    }
}











int get_listener_socket(void)
{

    struct sockaddr_un addr;
    int yes=1;
    int rv;
    int listener;

    unlink(SOCKET_PATH);

    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;

    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    listener = socket(AF_UNIX, SOCK_STREAM, 0);

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
    {
        close(listener);
        perror("bind");
    }

    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    return listener;

}


int get_listener_serial(void)
{


    int listener = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_SYNC);
    if (listener < 0)
    {
        perror("unable to open serial port");
        return -1;
    }


     // printf("setup termios\n");
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(listener, &tty) != 0)
    {
        printf("tcgetattr");
        close(listener);
        return -1;
    }

    cfsetospeed(&tty, B38400);
    cfsetispeed(&tty, B38400);

    // 8N1 configuration
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    // tty.c_cflag &= ~CRTSCTS; // Disable hardware flow control
    tty.c_cflag |= (CLOCAL | CREAD);

    // Input processing - disable all special processing
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Output processing - raw output
    tty.c_oflag &= ~OPOST;

    // CRITICAL FIX: Disable canonical mode for character-by-character reading
    tty.c_lflag &= ~ICANON; // Raw mode - read character by character
    tty.c_lflag &= ~(ECHO | ECHONL | ISIG | IEXTEN);

    // Timeout settings for raw mode
    tty.c_cc[VMIN] = 8;   // wait for some chars
    tty.c_cc[VTIME] = 15; // Timeout in deciseconds (1.5 seconds)

    if (tcsetattr(listener, TCSANOW, &tty) != 0)
    {
        printf("tcsetattr error? \n");
        perror("tcsetattr");

        close(listener);
        return -1;
    }

    tcflush(listener, TCIOFLUSH);
    
    return listener;

}



int main(int argc, char *argv[]) 
{


    fd_set master; // master file descriptor list
    fd_set read_fds; // temp file descriptor list
    int fdmax;
    int listener_fd;
    int serial_fd;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);



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

    strncpy(SERIAL_PORT, argv[1], strlen(argv[1]));


    listener_fd = get_listener_socket();

    FD_SET(listener_fd, &master);

    serial_fd = get_listener_serial();

    FD_SET(serial_fd, &master);

    fdmax = MAX(listener_fd, serial_fd);

    //fdmax = listener;

    for(;;) {
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(4);
        }

        char buf[256];
        int nbytes;

        for(int fd=0; fd <= fdmax; fd++) {
            if (FD_ISSET(fd, &read_fds)) {
                // handle new connections
                if (fd==listener_fd) 
                {
                    int newfd;

                    newfd = accept(listener_fd, NULL, NULL);
                    if (newfd == -1) {
                        perror("accept");
                    } else{
                    
                        FD_SET(newfd, &master);
                    
                        if(newfd > fdmax) {
                            fdmax = newfd;
                        }
                    
                        printf("select server: new connection on %d\n", newfd);
                    }

                }
                
                if (fd==serial_fd) 
                { 


                    if((nbytes = read(serial_fd, buf, sizeof(buf))) <= 0)
                    {
                        if (nbytes == 0) {
                            printf("serial no bytes");
                        } else {
                            perror("read");
                        }
                    
                    } 
                    else 
                    {
                        // for clients 
                        for(int j = 0; j <= fdmax; j++) {
                        // send to everyone!
                            if (FD_ISSET(j, &master)) {
                                // except the listener and serial
                                if (j != listener_fd && j != serial_fd) {
                                    if (write(j, buf, nbytes) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                    }
                }
                // read then write to clients
                else 
                {
                    if ((nbytes = read(fd, buf, sizeof buf)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", fd);
                        } else {
                            perror("recv");
                        }
                        close(fd); // bye!
                        FD_CLR(fd, &master); // remove from master set
                    } else {
                        // we got some data from a client
                        broadcast(buf, nbytes, listener_fd, serial_fd, fd, &master, fdmax);
                    }



                }
                //for(int j = 0; j <= fdmax; j++) {
                //    // send to everyone!
                //    if (FD_ISSET(j, &master)) {
                //        // except the listener and ourselves
                //        if (j != listener_fd && j != serial_fd) {
                //            if (send(j, buf, nbytes, 0) == -1) {
                //                perror("send");
                //            }
                //        }
                //    }
                //    }
                //}
            }
        }
    }

    close(serial_fd);

    return 0;
}


