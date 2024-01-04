#include "download.h"

int parseURL(char *inputURL, struct URL *url)
{
    struct hostent *h;

    regex_t regex;
    regcomp(&regex, "/", 0);

    if (regexec(&regex, inputURL, 0, NULL, 0))
    {
        return -1;
    }

    regcomp(&regex, "@", 0);

    if (regexec(&regex, inputURL, 0, NULL, 0) != 0)
    { // ftp://<host>/<url-path>

        sscanf(inputURL, "%*[^/]//%[^/]", url->host);
        strcpy(url->user, "anonymous");
        strcpy(url->password, "password");
    }

    else
    { // ftp://[<user>:<password>@]<host>/<url-path>

        sscanf(inputURL, "%*[^/]//%*[^@]@%[^/]", url->host);
        sscanf(inputURL, "%*[^/]//%[^:/]", url->user);
        sscanf(inputURL, "%*[^/]//%*[^:]:%[^@\n$]", url->password);
    }

    sscanf(inputURL, "%*[^/]//%*[^/]/%s", url->resource);
    strcpy(url->file, strrchr(inputURL, '/') + 1);

    if (strlen(url->host) == 0)
    {
        return -1;
    }

    if ((h = gethostbyname(url->host)) == NULL)
    {
        printf("Invalid hostname '%s'\n", url->host);
        exit(-1);
    }

    strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));

    return !(strlen(url->host) && strlen(url->user) && strlen(url->password) && strlen(url->resource) && strlen(url->file));
}

int establishSocketConnection(char *ip, int port)
{

    int socket_fd;
    struct sockaddr_in server_addr;

    bzero((char *)&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error in socket()");
        exit(-1);
    }
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error in connect()");
        exit(-1);
    }

    return socket_fd;
}

int authenticateConnection(const int fd, const char *user, const char *pass)
{

    char userCommand[5 + strlen(user) + 1];
    char passCommand[5 + strlen(pass) + 1];
    char answer[500];

    sprintf(userCommand, "user %s\n", user);
    sprintf(passCommand, "pass %s\n", pass);

    write(fd, userCommand, strlen(userCommand));

    if (readSocketResponse(fd, answer) != 331)
    {
        printf("Error in username: '%s'. Operation aborted.\n", user);
        exit(-1);
    }

    write(fd, passCommand, strlen(passCommand));

    return readSocketResponse(fd, answer);
}

int passiveMode(const int fd, char *ip, int *port)
{
    char answer[500];
    int ip1;
    int ip2;
    int ip3;
    int ip4;
    int port1;
    int port2;

    write(fd, "pasv\n", 5);

    if (readSocketResponse(fd, answer) != 227)
    {
        return -1;
    }

    sscanf(answer, "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]", &ip1, &ip2, &ip3, &ip4, &port1, &port2);

    *port = port1 * 256 + port2;

    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

    return 227;
}

int readSocketResponse(const int fd, char *buffer)
{
    char byte;

    int index = 0;
    int status_code;

    responseStatus state = START;
    memset(buffer, 0, 500);

    while (state != END)
    {

        read(fd, &byte, 1);
        switch (state)
        {
        case START:
            if (byte == ' ')
            {
                state = SINGLE;
            }
            else if (byte == '-')
            {
                state = MULTIPLE;
            }
            else if (byte == '\n')
            {
                state = END;
            }
            else
            {
                buffer[index++] = byte;
            }
            break;
        case SINGLE:
            if (byte == '\n')
            {
                state = END;
            }
            else
            {
                buffer[index++] = byte;
            }
            break;
        case MULTIPLE:
            if (byte == '\n')
            {
                memset(buffer, 0, 500);
                state = START;
                index = 0;
            }
            else
            {
                buffer[index++] = byte;
            }
            break;
        case END:
            break;
        default:
            break;
        }
    }

    sscanf(buffer, "%d", &status_code);

    return status_code;
}

int sendFileRequest(const int fd, char *resource)
{
    char fileCommand[5 + strlen(resource) + 1], answer[500];
    sprintf(fileCommand, "retr %s\n", resource);
    write(fd, fileCommand, sizeof(fileCommand));

    return readSocketResponse(fd, answer);
}

int transferFileFromServer(const int fd1, const int fd2, char *filename)
{
    FILE *fd = fopen(filename, "wb");
    char buffer[500];
    int bytes;

    if (fd == NULL)
    {
        printf("Error opening or creating file '%s'\n", filename);
        exit(-1);
    }

    do
    {
        bytes = read(fd2, buffer, 500);
        if (fwrite(buffer, bytes, 1, fd) < 0)
        {
            return -1;
        }
    } while (bytes);

    fclose(fd);

    return readSocketResponse(fd1, buffer);
}

int closeSocketConnection(const int fd1, const int fd2)
{
    char answer[500];
    write(fd1, "quit\n", 5);

    if (readSocketResponse(fd1, answer) != 221)
    {
        return -1;
    }

    return close(fd1) || close(fd2);
}

int main(int argc, char *argv[])
{
    struct URL url;
    char answer[500];
    int fd1;
    int fd2;
    int port;
    char ip[500];

    if (argc != 2)
    {
        printf("INVALID! Use the following format: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    memset(&url, 0, sizeof(url));
    if (parseURL(argv[1], &url) != 0)
    {
        printf("Parse error. INVALID! Use the following format: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    printf("Host: %s\nResource: %s\nFile: %s\nUser: %s\nPassword: %s\nIP Address: %s\n", url.host, url.resource, url.file, url.user, url.password, url.ip);

    fd1 = establishSocketConnection(url.ip, 21);
    if (fd1 < 0 || readSocketResponse(fd1, answer) != 220)
    {
        printf("Failed to create socket for '%s' on port %d\n", url.ip, 21);
        exit(-1);
    }

    if (authenticateConnection(fd1, url.user, url.password) != 230)
    {
        printf("Incorrect username: '%s'\nIncorrect password: '%s'.\n", url.user, url.password);
        exit(-1);
    }

    if (passiveMode(fd1, ip, &port) != 227)
    {
        printf("Failed to switch to passive mode\n");
        exit(-1);
    }

    fd2 = establishSocketConnection(ip, port);
    if (fd2 < 0)
    {
        printf("Failed to create socket to '%s:%d'\n", ip, port);
        exit(-1);
    }

    if (sendFileRequest(fd1, url.resource) != 150)
    {
        printf("Unidentified resource '%s' at '%s:%d'\n", url.resource, ip, port);
        exit(-1);
    }

    if (transferFileFromServer(fd1, fd2, url.file) != 226)
    {
        printf("Failed to transfer file '%s' from '%s:%d'\n", url.file, ip, port);
        exit(-1);
    }

    if (closeSocketConnection(fd1, fd2) != 0)
    {
        printf("Error closing sockets\n");
        exit(-1);
    }

    return 0;
}