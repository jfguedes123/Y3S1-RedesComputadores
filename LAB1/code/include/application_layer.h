// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include <stdio.h>

 //This function constructs a control packet for transmission based on the specified type
 //(0 for Start, 1 for End),file name(*f), and size. Control packets are used to mark the start or end of a file transfer.
 //returns the total size of the constructed control packet.
 
int createControlPacket(unsigned char* buf, int type, char* f, int size);

 // This function reads and parses a control packet stored in the input buffer `buf`,
 // extracting the filename and file size information. Control packets are used to mark
 // the start or end of a file transfer.
 // filesz - A pointer to an integer where the extracted file size will be stored.
 // returns 0 on success, -1 if the input buffer does not contain a valid control packet.
 
int readControlPacket(unsigned char* buf, char* filename, int* filesz);


 // This function constructs a data packet for transmission by populating the
 // provided buffer with the necessary information and data. The data packet
 // structure is as follows:
 // Byte 0: Packet type (0x01 for data packets).
 // Byte 1: Sequence number.
 // Byte 2-3: Data size (big-endian representation).
 // Byte 4 and onwards: Actual data content.
 // return The total size of the constructed data packet in bytes.
 
int createDataPacket(unsigned char* buf, int aux, int size, unsigned char* data);


 // This function reads and parses a data packet stored in the input buffer `buf`,
 // extracting the sequence number and data. Data packets are used to transmit file
 // content during a file transfer ~ data is a buffer where dat will be stored.
 // ~seq  pointer to an integer where the extracted sequence number will be stored.
 // returns The size of the extracted data on success, -1 if the input buffer does not contain a valid data packet.

int readDataPacket(unsigned char* data, unsigned char* buf, int* seq);


 // This function is the main entry point for transmitting or receiving files over a serial connection
 // using the specified application layer configuration.

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

#endif // _APPLICATION_LAYER_H_