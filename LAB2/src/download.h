#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <termios.h>

/* Parser output */
struct URL
{
    char host[500];     // 'ftp.up.pt'
    char resource[500]; // 'parrot/misc/canary/warrant-canary-0.txt'
    char file[500];     // 'warrant-canary-0.txt'
    char user[500];     // 'username'
    char password[500]; // 'password'
    char ip[500];       // 193.137.29.15
};

/* Machine states that receives the response from the server */
typedef enum
{
    START,
    SINGLE,
    MULTIPLE,
    END
} responseStatus;

/*
 * Parser that transforms user input in url parameters
 * @param inputURL, a string containing the user input
 * @param url, a struct that will be filled with the url parameters
 * @return 0 if there is no parseURL error or -1 otherwise
 */
int parseURL(char *inputURL, struct URL *url);

/*
 * Create socket file descriptor based on given server ip and port
 * @param ip, a string containing the server ip
 * @param port, an integer value containing the server port
 * @return socket file descriptor if there is no error or -1 otherwise
 */
int establishSocketConnection(char *ip, int port);

/*
 * Authenticate connection
 * @param socket, server connection file descriptor
 * @param user, a string containing the username
 * @param pass, a string containing the password
 * @return server response code obtained by the operation
 */
int authenticateConnection(const int fd, const char *user, const char *pass);

/*
 * Read server response
 * @param socket, server connection file descriptor
 * @param buffer, string that will be filled with server response
 * @return server response code obtained by the operation
 */
int readSocketResponse(const int fd, char *buffer);

/*
 * Enter in passive mode
 * @param socket, server connection file descriptor
 * @param ip, string that will be filled with data connection ip
 * @param port, string that will be filled with data connection port
 * @return server response code obtained by the operation
 */
int passiveMode(const int fd, char *ip, int *port);

/*
 * Request resource
 * @param socket, server connection file descriptor
 * @param resource, string that contains the desired resource
 * @return server response code obtained by the operation
 */
int sendFileRequest(const int fd, char *resource);

/*
 * Get resource from server and download it in current directory
 * @param socketA, server connection file descriptor
 * @param socketB, server connection file descriptor
 * @param filename, string that contains the desired file name
 * @return server response code obtained by the operation
 */
int transferFileFromServer(const int fd1, const int fd2, char *filename);

/*
 * Closes the server connection and the socket itself
 * @param socketA, server connection file descriptor
 * @param socketB, server connection file descriptor
 * @return 0 if there is no close error or -1 otherwise
 */
int closeSocketConnection(const int socketA, const int socketB);
