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
                printf("Please specify the file to get");
            }
            else {
                //TODO
            }
        }
        else if (strcasecmp(arg, "put") == 0) {
            arg = strtok(NULL, " \n");
            if (arg == NULL) {
                printf("Please specify the file to put");
            }
            else {
                //TODO
            }
        }
        else if (strcasecmp(arg, "delete") == 0) {
            arg = strtok(NULL, " \n");
            if (arg == NULL) {
                printf("Please specify the file to delete");
            }
            else {
                //TODO
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
            printf("Result: %s\n", buf2);
        }
        else if (strcasecmp(arg, "exit") == 0) {
            printf("\nGoodbye\n");
            return 0;
        }
        else {
            printf("Invalid Command\n\n");
        }
    }
}
