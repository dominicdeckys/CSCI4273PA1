/*
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFSIZE 1024
#define TRUE 1
#define FAlSE 0

/*
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

void sayPrompt() {
    printf("Welcome, please enter one of the following commands:\n");
    printf("get [file_name]\nput [file_name]\ndelete [file_name]\nls\nexit\n");
}

void readFile(const char * fileName) {
    FILE * file = fopen(fileName, "r");
    size_t byteCount = 0;
    unsigned char buf[BUFSIZE];
    int t = 0;
    while ((byteCount = fread(buf, 1, sizeof(buf), file)) > 0) {
        t++;
        printf("Chunk %i\nSize %i\n", t, byteCount);
    }
}

void awaitConfirmation(char value, int sockfd) {
    char buf[BUFSIZE];
    int n;
    while (TRUE) {
        n = recvfrom(sockfd, buf, BUFSIZE, 0, NULL, NULL);
        if (n < 0)
          error("ERROR is recvfrom");
        if (buf[0] == value) break;
    }
}

//void buildHeader(char * header, int type, size_t size, )

void sendFile(char * fileName, int sockfd, struct sockaddr_in *serveraddr, int serverlen) {
    char buf[BUFSIZE];
    int n;
    FILE * file = fopen(fileName, "r");
    size_t byteCount = 0;
    int t = 0;
    bzero(buf, BUFSIZE);
    buf[0] = 'f';
    strcpy(buf + 1, fileName);
    n = sendto(sockfd, buf, strlen(buf), 0, serveraddr, serverlen);
    if (n < 0)
            error("ERROR in sendto 1");
    awaitConfirmation('f', sockfd);

    bzero(buf, BUFSIZE);
    while ((byteCount = fread(buf + 1, 1, BUFSIZE - 1, file)) > 0) {
        buf[0] = 's';
        n = sendto(sockfd, buf, byteCount + 1, 0, serveraddr, serverlen);
        if (n < 0)
            error("ERROR in sendto");
        t++;
        printf("Sent Chunk %i\nSize %i\n", t, byteCount, buf);
        bzero(buf, BUFSIZE);
    }
}

void recieveFile(char * fileName, int sockfd, struct sockaddr_in *serveraddr, int serverlen) {
    char buf[BUFSIZE];
    int n;
    FILE * file;
    bzero(buf, BUFSIZE);
    buf[0] = 'r';
    strcpy(buf + 1, fileName);
    n = sendto(sockfd, buf, strlen(buf), 0, serveraddr, serverlen);
    if (n < 0)
            error("ERROR in sendto 2");
    while (TRUE) {
        bzero(buf, BUFSIZE);
        n = recvfrom(sockfd, buf, BUFSIZE, 0, NULL, NULL);
        if (n < 0)
          error("ERROR in recvfrom");
        if (buf[0] == 's') {
            file = fopen(fileName, "a");
            fwrite(buf + 1, 1, n - 1, file);
            fflush(file);
            fclose(file);
        }
        else break;
    }
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char * arg;
    char buf[BUFSIZE];
    char buf2[BUFSIZE];
    char test[] = "hello";
    char message[BUFSIZE];

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* send test message to the server */
    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, test, strlen(test), 0, &serveraddr, serverlen);
    if (n < 0)
      error("ERROR in sendto");

    bzero(buf, BUFSIZE);
    /* get server's reply */
    n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
    if (n < 0)
      error("ERROR is recvfrom");

    if (strcmp(buf, test) != 0) {
        printf("Two values: buf:%s test:%s\n", buf, test);
        error("Error in pinging the server");
    }


    while (TRUE) {
        /* get a message from the user */
        bzero(buf, BUFSIZE);
        sayPrompt();
        fgets(buf, BUFSIZE, stdin);
        arg = strtok(buf, " \n");

        if (strcasecmp(arg, "get") == 0) {
            arg = strtok(NULL, " \n");
            if (arg == NULL) {
                printf("Please specify the file to get\n");
            }
            else {
                recieveFile(arg, sockfd, &serveraddr, serverlen);
            }
        }
        else if (strcasecmp(arg, "put") == 0) {
            arg = strtok(NULL, " \n");
            if (arg == NULL) {
                printf("Please specify the file to put\n");
            }
            else {
                sendFile(arg, sockfd, &serveraddr, serverlen);
            }
        }
        else if (strcasecmp(arg, "delete") == 0) {
            arg = strtok(NULL, " \n");
            if (arg == NULL) {
                printf("Please specify the file to delete\n");
            }
            else {
                bzero(buf2, BUFSIZE);
                buf2[0] = 'd';
                strcpy(buf2 + 1, arg);
                n = sendto(sockfd, buf2, strlen(buf2), 0, &serveraddr, serverlen);
                if (n < 0)
                  error("ERROR in sendto");
                }
        }
        else if (strcasecmp(arg, "ls") == 0) {
            n = sendto(sockfd, arg, strlen(arg), 0, &serveraddr, serverlen);
            if (n < 0)
              error("ERROR in sendto");
            bzero(buf2, BUFSIZE);
            /* get server's reply */
            n = recvfrom(sockfd, buf2, BUFSIZE, 0, &serveraddr, &serverlen);
            if (n < 0)
              error("ERROR is recvfrom");
            printf("Result:\n%s\n", buf2);
        }
        else if (strcasecmp(arg, "exit") == 0) {
            bzero(buf, BUFSIZE);
            buf[0] = 'e';
            n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
            if (n < 0)
              error("ERROR in sendto");
            printf("\nGoodbye\n");
            return 0;
        }
        else {
            printf("Invalid Command\n\n");
        }
    }
}
