/*
 * udpserver.c - A simple UDP echo server
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#define BUFSIZE 1024
#define TRUE 1
#define FAlSE 0

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

//gets all the files in the current directory and puts them into ls
int performLS(char * ls) {
    DIR *dp;
    struct dirent *ep;
    strcpy(ls, "");
    dp = opendir ("./");

    if (dp != NULL)
    {
        while (ep = readdir (dp)){
          strcat (ls, ep->d_name);
          strcat (ls, "\n");
        }

        (void) closedir (dp);
    }
    else return 0;
    return 1;
}

//sends a file to the client, mostly the same as the function on the client's side
void sendFile(char * fileName, int sockfd, struct sockaddr_in *serveraddr, int serverlen) {
    char buf[BUFSIZE];
    int n;
    FILE * file = fopen(fileName, "r");
    size_t byteCount = 0;
    int t = 0;
    printf("Got here!\n");
            printf("%s\n",fileName);
    bzero(buf, BUFSIZE);
    while ((byteCount = fread(buf + 1, 1, BUFSIZE - 1, file)) > 0) {

        buf[0] = 's';
        n = sendto(sockfd, buf, byteCount + 1, 0, serveraddr, serverlen);
        if (n < 0)
            error("ERROR in sendto");
        t++;
        printf("Sent Chunk %i\nSize %i\n", t, byteCount);
        bzero(buf, BUFSIZE);
    }
    bzero(buf, BUFSIZE);
    buf[0] = 'f';
    strcpy(buf + 1, fileName);
    n = sendto(sockfd, buf, strlen(buf), 0, serveraddr, serverlen);
    if (n < 0)
            error("ERROR in sendto 1");
}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char buf2[BUFSIZE];
  char *hostaddrp; /* dotted decimal host addr string */
  char * arg;
  char * fileName = malloc(BUFSIZE);
  FILE * file;
  char badMessage[] = "Error: Bad Message!";
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  strcpy(fileName, "NoFileName.txt");

  /*
   * check command line arguments
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /*
   * socket: create the parent socket
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /*
   * bind: associate the parent socket with a port
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr,
	   sizeof(serveraddr)) < 0)
    error("ERROR on binding");

  /*
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (TRUE) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /*
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n",
	   hostp->h_name, hostaddrp);
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);



    //read the first char of the buffer
    //the first char is used to detirmine what type of message it is
    //f = initiate file transfer
    if (buf[0] == 'f') {
        //fclose(file);
        strcpy(fileName, buf + 1);

        n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0)
          error("ERROR in sendto");
    }
    //s = chunk of a file
    else if (buf[0] == 's') {
        printf("Got here!\n");
        printf("%s\n", buf + 1);
        //append the file in the local filesystem
            file = fopen(fileName, "a");
            fwrite(buf + 1, 1, n - 1, file);
            fflush(file);
            fclose(file);
    }
    //l = perform ls
    else if (buf[0] == 'l') {
        bzero(buf2, BUFSIZE);
        performLS(buf2);
        n = sendto(sockfd, buf2, strlen(buf2), 0,
	       (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0)
          error("ERROR in sendto");
    }
    //h = ping the server and return the buffer
    else if (buf[0] == 'h') {
        n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
            if (n < 0)
              error("ERROR in sendto");
    }
    //r = send a file to the client
    else if (buf[0] == 'r') {
        sendFile(buf + 1, sockfd, &clientaddr, clientlen);
    }
    //d = delete file
    else if (buf[0] == 'd') {
        remove(buf + 1);
    }
    //e = exit
    else if (buf[0] == 'e') {
        printf("Goodbye!\n");
        return 1;
    }
    /*
     * sendto: echo the input back to the client
     */
    //n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
    //if (n < 0)
      //error("ERROR in sendto");
  }
}
