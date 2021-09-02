#include <iostream>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "proxyserver.h"
#include "stdio.h"

#define	MAXLINE		4096    /* max text line length */
#define C_PORT 13
#define ERROR_NUMER_RETURN  -1

#define SERVER_IP4_ADDR "206.168.112.96"

int main(int argc, char **argv)
{
    int     sockfd, n;
    char    recvline[MAXLINE + 1];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        std::cerr << "socket error" << std::endl;
        return ERROR_NUMER_RETURN;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(C_PORT);
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP4_ADDR);
    if(inet_pton(AF_INET, SERVER_IP4_ADDR, &servaddr.sin_addr) <= 0) {
        std::cerr << "inet_pton error for" << SERVER_IP4_ADDR;
        return ERROR_NUMER_RETURN;
    }

    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "connect error " << errno;
    }

    while ( (n = recv(sockfd, recvline, MAXLINE, 0)) > 0)
    {
        recvline[n] = 0;	/* null terminate */
        if (fputs(recvline, stdout) == EOF)
            std::cerr << "fputs error";
    }
    if (n < 0)
        std::cerr << "read error";

    return 0;
}
