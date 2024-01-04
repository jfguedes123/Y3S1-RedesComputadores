// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

#define BUF_SIZE 256
#define FALSE 0
#define TRUE 1

// MISC
#define FALSE 0
#define TRUE 1

//Data Packet
#define DATA_PACKET 0x01

//Delimitation constants (byte stuffing purpose)
#define FRAME_SIZE 5
#define A_SET 0x03
#define A_UA 0x01
#define FLAG 0x7E
#define ESC 0x7D
#define ESCE 0x5E
#define ESCD 0x5D
#define TR 0x03
#define REC 0x01
#define IFCTRL_ON 0x40
#define IFCTRL_OFF 0x00

//Control packet constants
#define C_START 2
#define C_END 3
#define C_FILE_SIZE 0
#define C_FILE_NAME 1
#define C_SET 0x03
#define C_DISC 0x0B
#define C_UA 0x07
#define C_REJ0 0x01 //rejected
#define C_REJ1 0x81
#define C_RR0 0x05 //received
#define C_RR1 0x85


typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef enum
{
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC1_RCV,
    STOP,
    C_INF,
    REJ
} LinkLayerState;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

void stateMachine(LinkLayerState* status, unsigned char byte, int type);

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);


//This function is called when the alarm signal is received.
//It sets the `alarmFlag` to `TRUE` and increments the `alarmCounter`.

void alarmController(int signal);

// Send data in buf with size bufSize.
//fd is a file descriptor (serial port)
// Return number of chars written, or "-1" on error.
int llwrite(int fd, const unsigned char *buf, int bufSize);


// This function constructs and sends a REJ (REJection, 5 bytes) packet to indicate that an error
// was detected in the received data. The specific packet format and content are determined
// based on the value of the `nr` variable, indicating the current frame number.

int sendReplyPacket();

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char *packet);


// This function handles link layer closing procedures based on the role of the connection. It can either send a
// DISC (Disconnect) command and wait for a UA (Unnumbered Acknowledgment) response or receive a DISC command and
// send a UA response. The function also restores the serial port settings and closes the port.

int llclose(int showStatistics);

#endif // _LINK_LAYER_H_
