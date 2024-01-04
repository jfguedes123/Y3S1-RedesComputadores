// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>

extern int alarmFlag;
extern int alarmCounter;
extern int nr;
extern int valid;


int getFileSize(FILE* f)
{
    int size; // Variable to store the size of the file.

    // Seek to the end of the file to determine its size.
    fseek(f, 0, SEEK_END);
    size = (int)ftell(f);
    // Return to the beginning of the file.
    fseek(f, 0, SEEK_SET);
    return size; // Return the size of the file in bytes.
}

int getNumberBytes(int size)
{
    int counter = 0; // Initialize a counter to keep track of the number of bytes.

    // Continuously divide the size by 256 until it becomes zero.
    while(size > 0) {
        size /= 256;
        counter++;
    }

    return counter; // Return the number of bytes required.
}


//This function constructs a control packet for transmission based on the specified type
 //(0 for Start, 1 for End),file name(*f), and size. Control packets are used to mark the start or end of a file transfer.
 //returns the total size of the constructed control packet.

int createControlPacket(unsigned char* buf, int type, char* f, int size) {
    if (type == 0) {
        buf[0] = C_START; // Start control packet type (0x02).
    } else if (type == 1) {
        buf[0] = C_END; // End control packet type (0x03).
    }

    unsigned L1 = getNumberBytes(size);
    unsigned L2 = strlen(f);
    unsigned tam = L1 + L2 + 5;

    // Byte 1: Reserved (always 0).
    buf[1] = 0;

    // Byte 2: L1 - Number of bytes required to represent the file size.
    buf[2] = L1;

    // Extract L1 bytes to represent the file size (big-endian).
    for (int i = 0; i < L1; i++) {
        int tmp = (size & 0x0000FFFF) >> 8;
        buf[3 + i] = tmp;
        size = size << 8;
    }

    // Byte L1+3: Reserved (always 1).
    int memo = L1 + 3;
    buf[memo] = 1;

    // Byte L1+4: L2 - Number of bytes required to represent the file name.
    buf[memo + 1] = L2;

    // Copy the file name into the packet.
    memcpy(&buf[memo + 2], f, L2);

    // Log the bytes of the control packet for debugging.
    for (int i = 4; i < 4 + tam; i++) {
        fprintf(stderr, "0x%02X, ", buf[i]);
    }

    return tam;
}


// This function reads and parses a control packet stored in the input buffer `buf`,
 // extracting the filename and file size information. Control packets are used to mark
 // the start or end of a file transfer.
 // filesz - A pointer to an integer where the extracted file size will be stored.
 // returns 0 on success, -1 if the input buffer does not contain a valid control packet.

int readControlPacket(unsigned char* buf, char* filename, int* filesz) {
    *filesz = 0; // Initialize the file size.

    // Check if the packet is not a valid control packet (neither Start nor End).
    if (buf[0] != 0x02 && buf[0] != 0x03) {
        fprintf(stderr, "Not a valid control packet.\n");
        return -1; // Return an error code.
    }

    // If it's a Start control packet (0x02).
    if (buf[1] == 0x00) {
        int fst = buf[2];
        for (int i = 0; i < fst; i++) {
            *filesz = *filesz * 256 + buf[3 + i]; // Extract and compute the file size.
        }
    } else {
        return -1; // Return an error code for an invalid control packet.
    }

    int snd = 0;
    int nxt = 5 + *filesz;

    // If it's a valid control packet with a filename.
    if (buf[nxt - 2] == 0x01) {
        snd = buf[nxt - 1];
        for (int j = 0; j < snd; j++) {
            filename[j] = buf[j + nxt]; // Extract the filename.
        }
    } else {
        return -1; // Return an error code for an invalid control packet.
    }

    return 0; // Return success.
}


 // This function constructs a data packet for transmission by populating the
 // provided buffer with the necessary information and data. The data packet
 // structure is as follows:
 // Byte 0: Packet type (0x01 for data packets).
 // Byte 1: Sequence number.
 // Byte 2-3: Data size (big-endian representation).
 // Byte 4 and onwards: Actual data content.
 // return The total size of the constructed data packet in bytes.

int createDataPacket(unsigned char* buf, int aux, int size, unsigned char* data) {
    // Byte 0: Packet type (0x01 for data)
    buf[0] = DATA_PACKET;

    // Byte 1: Sequence number (limited to 0-255)
    buf[1] = aux % (BUF_SIZE - 1);

    // Byte 2-3: Data size
    buf[2] = size / BUF_SIZE;
    buf[3] = size % BUF_SIZE;

    //Printing the data content values
    for(int i = 4; i < 4 + size; i++) {

    }
    printf("Packet sent ");

    // Copy the data into the packet starting from Byte 4
    memcpy(buf + 4, data, size);

    // Calculate and return the total size of the constructed data packet
    return 4 + size;
}


// This function reads and parses a data packet stored in the input buffer `buf`,
 // extracting the sequence number and data. Data packets are used to transmit file
 // content during a file transfer ~ data is a buffer where dat will be stored.
 // ~seq  pointer to an integer where the extracted sequence number will be stored.
 // returns The size of the extracted data on success, -1 if the input buffer does 
 // not contain a valid data packet.

int readDataPacket(unsigned char* data, unsigned char* buf, int* seq)
{
    // Check if the packet is not a valid data packet.
    if (buf[0] != 0x01)
    {
        printf("Not a valid data packet.\n");
        return -1; // Return an error code.
    }

    *seq = buf[1]; // Extract and store the sequence number.
    int l2 = buf[2];
    int l1 = buf[3];
    int sz = (256 * l2) + l1; // Calculate the size of the data.

    for(int i = 0; i < sz; i++)
    {
        data[i] = buf[i + 4]; // Extract and store the data.
    }
    return sz; // Return the size of the extracted data.
}



void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer llObject;
    strcpy(llObject.serialPort, serialPort);

    // roles
    if (strcmp(role, "tx") == 0)
    {
        llObject.role = LlTx; // transmitter
    }
    else if (strcmp(role, "rx") == 0)
    {
        llObject.role = LlRx; // receiver
    }
    else
    {
        perror("Invalid role\n");
        exit(-1);
    }

    llObject.baudRate = baudRate;
    llObject.nRetransmissions = nTries;
    llObject.timeout = timeout;

    int fd = open(llObject.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0) {
        perror("Connection error\n");
        exit(-1);
    }

    llopen(llObject); // Connection using the link layer struct.
    alarmReset(); //resets the alarm count and flag
    if (llObject.role == LlTx) {
        unsigned char control[MAX_PAYLOAD_SIZE];
        unsigned char buf[MAX_PAYLOAD_SIZE];

        // Open the file
        FILE* file = fopen(filename, "r");
        if (file == NULL) {
            perror("Error: file is not available.\n");
            exit(-1);
        }

        int size = getFileSize(file);

        int control_packet_size = createControlPacket(&control, 0, filename, size);
        llwrite(fd, &control, control_packet_size);

        int bytes_r = 0;
        int sq = 0;
        alarmReset();
        while (bytes_r = fread(buf, 1, MAX_PAYLOAD_SIZE - 4, file)) {
            // Reading data from the file and creating data packets.
            unsigned char data[MAX_PAYLOAD_SIZE];
            int mount = createDataPacket(&data, sq, bytes_r, buf);
            llwrite(fd, &data, mount);
            sleep(3); //test with virtual cable to delay transmission and allow to change cable status
            alarmReset();
            fprintf(stderr, "the size of the data packet sent is: %d\n", mount);
            sq++;
        }

        fprintf(stderr, "Control Packet End sent:\n");
        control_packet_size = createControlPacket(&control, 1, filename, size);
        llwrite(fd, &control, control_packet_size);
        alarmReset();
    }
    else if (llObject.role == LlRx) {
        int size;
        char sizec;
        unsigned char control[MAX_PAYLOAD_SIZE + 7];
        unsigned char data[MAX_PAYLOAD_SIZE + 7];

        int sz = llread(&control);
        printf("First llread");
        FILE* file2 = fopen(filename, "w");
        int ns = 0;
        
        while (1) {
            int sq;
            int lastnr = nr;

            do {
                sz = llread(&control);
            } while (sz == -1);

            fprintf(stderr, "New Packet created:\n");

            if (control[0] == 0x03) { //SET frame : sent by transmitter to initiate connection
                fprintf(stderr, "Control Pack End received:\n");
                break;
            }
            else if (control[0] == 0x01) { //REJ0 frame : detects an error by the receiver in the frame 0
                sz = readDataPacket(&data, &control, &sq);
                printf("\n\n");
                for (int i = 0; i < sz; i++) {
                    fprintf(stderr, "\\%02x", data[i]);
                }
                fwrite(data, 1, sz, file2);
            }
        }
    }
    llclose(0);
}
