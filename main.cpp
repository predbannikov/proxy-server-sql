#include <iostream>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "proxyserver.h"

#define SA struct sockaddr
#define	MAXLINE		4096    /* max text line length */
#define C_PORT 13
#define ERROR_NUMER_RETURN  -1

int main(int argc, char **argv)
{
    int     sockfd, n;
    char    recvline[MAXLINE + 1];
    struct sockaddr_in servaddr;
    if(argc != 2) {
        std::cerr << "Error:" << strerror(errno);
        return ERROR_NUMER_RETURN;
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        std::cerr << "socket error" << std::endl;
        return ERROR_NUMER_RETURN;
    }
    bzero(&servaddr, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(C_PORT);
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        std::cerr << "inet_pton error for" << argv[1];
        return ERROR_NUMER_RETURN;
    }

    if(connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "connect error";
    }

    while ( (n = recv(sockfd, recvline, MAXLINE, 0)) > 0)
        {
            recvline[n] = 0;	/* null terminate */
            if (fputs(recvline, stdout) == EOF)
                std::cerr << "fputs error";
        }
        if (n == SOCKET_ERROR)
            errexit("recv error\n");

        return 0;
}
