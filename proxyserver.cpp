#include "proxyserver.h"
#include <algorithm>
#define SQL_SERVER_ADDRESS "127.0.0.1"
#define SQL_SERVER_PORT    3306

#define OFFSET_DATA_HEADER  4

using namespace ProxyServer;

Server::Server() {

}

void Server::pushConnectionSafeThr(Request &req)
{
    std::unique_lock<std::mutex> lock(mtx_connections);
    connections.insert({req.sockfd, req});
}

Request Server::popConnectSafeThr()
{
    std::unique_lock<std::mutex> lock(mtx_connections);

    return connections.at(0);
}

void printError(int sock1, int sock2, int error, std::string str) {
    close(sock1);
    close(sock2);
    std::cout << std::endl << "error:[" << error << "] " << std::strerror(error) << " : " << str  << std::endl;
}


void Server::forwardRequest(int sockfd)
{
    int sockfd_sql = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd_sql < 0) {
        std::cout << "create socket failed with INVALID_SOCKET" << errno << std::endl;
        close(sockfd);
        return;
    }
    struct hostent *hp;
    unsigned int addr;

    if (inet_addr(SQL_SERVER_ADDRESS) == INADDR_NONE)
        hp = gethostbyname(SQL_SERVER_ADDRESS);
    else {
        addr = inet_addr(SQL_SERVER_ADDRESS);
        hp = gethostbyaddr((char*)&addr, sizeof(addr), AF_INET);
    }

    if (hp == NULL)
    {
        printError(sockfd, sockfd_sql, errno, "get ip address host");
        return ;
    }
    sockaddr_in internetAddr;
    internetAddr.sin_family = AF_INET;
    internetAddr.sin_port = htons(SQL_SERVER_PORT);
    internetAddr.sin_addr.s_addr = *((unsigned long*)hp->h_addr);
    if (connect(sockfd_sql, (struct sockaddr*) &internetAddr, sizeof(internetAddr)))
    {
        printError(sockfd, sockfd_sql, errno, "sql-server connect failed");
        return ;
    }

    int n = 0, ret = 0;
    fd_set fdset;
    InfoConnect infoConnect;
    char buff[MAX_PACKET_SIZE];
    STATE_PARS state = STATE_PARS_INIT;
    while (true) {
        FD_ZERO(&fdset);
        FD_SET(sockfd, &fdset);
        FD_SET(sockfd_sql, &fdset);


        std::cout << "new cycle socks=" << sockfd << std::endl;


        timeval timeout {15};
        n = select(std::max(sockfd, sockfd_sql)+1, &fdset, NULL, NULL, &timeout);
        if (n < 0) {
            // Возможно можно выбросить этот url из запросов
            printError(sockfd, sockfd_sql, errno, "select fdset");
            return;
        } else if (n == 0) {
            std::cout << "time waiting out " << std::endl;
            printError(sockfd, sockfd_sql, errno, "time waiting out");
            return;
        } else {
            if(FD_ISSET(sockfd, &fdset)) {
                int len = recv(sockfd, buff, MAX_PACKET_SIZE, 0);
                if(len < 0) {
                    printError(sockfd, sockfd_sql, errno, "recv from client");
                } else {
                    infoConnect.client.package._header = *reinterpret_cast<int32_t*>(buff);
                    infoConnect.client.buffer.push_back({buff, len});
                    std::cout << std::endl;
                    if(len == infoConnect.client.package.head.size + OFFSET_DATA_HEADER) {
                        size_t idx = 0;
                        std::for_each(infoConnect.client.buffer.begin(), infoConnect.client.buffer.end(), [&idx, &infoConnect] (const std::pair<char*, int> &pair) {
                            for(size_t i = OFFSET_DATA_HEADER; i < pair.second; i++) {
                                std::cout << pair.first[i] << ":" << std::hex << (0xFF&static_cast<uint32_t>(pair.first[i])) << " - ";
                                idx++;
                            }
                        });
                    }

                    std::cout << buff << std::endl;
                    if(send(sockfd_sql, buff, len, 0) < 0) {
                        printError(sockfd, sockfd_sql, errno, "send to client");
                        return;
                    }

                }
                std::cout << "CLIENT: size=" <<  infoConnect.client.package.head.size << " id=" << static_cast<uint32_t>(infoConnect.server.package.head.id) << std::endl;
            }
            if(FD_ISSET(sockfd_sql, &fdset)) {
                int len = recv(sockfd_sql, buff, MAX_PACKET_SIZE, 0);
                if(len < 0) {
                    printError(sockfd, sockfd_sql, errno, "recv from sql-server");
                } else {
                    infoConnect.server.package._header = *reinterpret_cast<int32_t*>(buff);
                    infoConnect.server.buffer.push_back({buff, len});
                    infoConnect.server.current_size += len - OFFSET_DATA_HEADER;
                    std::cout << std::endl;
                    switch (state) {
                    case STATE_PARS_INIT:       // Узнать версию
                        if(len == infoConnect.server.package.head.size + OFFSET_DATA_HEADER) {
                            size_t idx = 0;
                            std::for_each(infoConnect.server.buffer.begin(), infoConnect.server.buffer.end(), [&idx, &infoConnect] (const std::pair<char*, int> &pair) {
                                for(size_t i = OFFSET_DATA_HEADER; i < pair.second; i++) {
                                    idx++;
                                    if(idx > 1 && idx < 8)
                                        infoConnect.sql_version.push_back(pair.first[i]);
                                }
                            });
                        }
                        state = STATE_PARS_NEXT;
                        break;
                    case STATE_PARS_NEXT:
                        if(len == infoConnect.server.package.head.size + OFFSET_DATA_HEADER) {
                            size_t idx = 0;
                            std::for_each(infoConnect.server.buffer.begin(), infoConnect.server.buffer.end(), [&idx, &infoConnect] (const std::pair<char*, int> &pair) {
                                for(size_t i = OFFSET_DATA_HEADER; i < pair.second; i++) {
                                    std::cout << pair.first[i] << ":" << std::hex << (0xFF&static_cast<uint32_t>(pair.first[i])) << " - ";
                                    idx++;
                                }
                            });
                        }
                        break;
                    }

                    std::cout << std::endl;
                    if(send(sockfd, buff, len, MSG_NOSIGNAL) < 0) {
                        printError(sockfd, sockfd_sql, errno, "send to sql-server");
                        return;
                    }
                    std::cout << "SERVER: size=" <<  infoConnect.server.package.head.size << " id=" << static_cast<uint32_t>(infoConnect.server.package.head.id) << std::endl;

                }
            }
        }
    }
}

void Server::loop() {
    thr_connect_handler = std::thread(&Server::connectionsHandler, this);
    thread_guard guard_conn_hand(thr_connect_handler);

    socklen_t 	clilen;
    int      	connfd;
    char		buff[MAXLINE];
    memset(&servaddr, 0, sizeof (servaddr));
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "socket can't create" << std::endl;
        return ;
    }
    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        std::cout << "listen error " << errno << std::endl;
        return ;
    }
    if(listen(listenfd, SOCK_MAX_CONN) < 0) {
        std::cout << "listen error" << std::endl;
        close(listenfd);
        return ;
    }
    while(true) {
        char response[] = "HTTP/1.0 200 Connection established\nProxy-agent: ProxyApp/1.1";
        // ждём соединения
        clilen = sizeof (cliaddr);
        Request req;
        req.sockfd = (SOCKET)accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
        if(set.find(req.sockfd) == set.end()) {
            set.insert(req.sockfd);
            std::thread thread(&Server::forwardRequest, this, req.sockfd);
            thread.detach();
        }
        req.sockaddr = cliaddr;
        printf("connect from %s, port %d \en", inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)), ntohs(cliaddr.sin_port));

    }
}

void Server::connectionsHandler()
{

}
