#include "proxyserver.h"
#include <algorithm>
#define SQL_SERVER_ADDRESS      "127.0.0.1"
#define SQL_SERVER_PORT         3306
#define OFFSET_DATA_HEADER      4

#define CHAR_OUT_LAMBDA [](const unsigned char & byte) { if(byte > 31 && byte ) {std::cout << byte << std::flush; } }


using namespace ProxyServer;

Server::Server() {

}

int printError(Connection &conn, int error, std::string str) {
    if(conn.server.socket != 0)
        close(conn.server.socket);
    if(conn.client.socket != 0)
        close(conn.client.socket);
    std::cerr << std::endl << "error:[" << error << "] " << std::strerror(error) << " : " << str  << std::endl;
    return error;
}

/*
void ProxyServer::Server::get_version(SocketInfo &packag)
{
    size_t idx = 0;
    std::for_each(packag.buffer[packag.id].begin(), packag.buffer[packag.id].end(), [&idx, &packag] (const std::pair<char*, int> &pair) {
        for(size_t i = OFFSET_DATA_HEADER; i < pair.second; i++) {
            idx++;
            if(idx > 1 && idx < 8)
                packag.sql_version.push_back(pair.first[i]);
        }
    });
}

void ProxyServer::Server::printBuffer(std::list<std::pair<char *, int> > &buffer)
{
    std::cout << std::endl;
    size_t idx = 0;
    std::for_each(buffer.begin(), buffer.end(), [&idx, &buffer] (const std::pair<char*, int> &pair) {
        for(size_t i = OFFSET_DATA_HEADER; i < pair.second; i++) {
            std::cout << pair.first[i];
            idx++;
        }
    });
    std::cout << std::hex  << std::endl;
    idx = 0;
    std::for_each(buffer.begin(), buffer.end(), [&idx, &buffer] (const std::pair<char*, int> &pair) {
        for(size_t i = OFFSET_DATA_HEADER; i < pair.second; i++) {
            std::cout<< (0xFF&static_cast<uint32_t>(pair.first[i])) << " ";
            idx++;
        }
    });
    std::cout << std::dec << std::endl;

}
*/

int Server::forwardRequest(int sockfd)
{
    Connection conn;
    conn.client.name = "client";
    conn.server.name = "server";
    conn.server.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (conn.server.socket < 0)
        return printError(conn, errno, "create socket failed with INVALID_SOCKET");

    conn.client.socket = sockfd;

    struct hostent *hp;
    unsigned int addr;

    if (inet_addr(SQL_SERVER_ADDRESS) == INADDR_NONE)
        hp = gethostbyname(SQL_SERVER_ADDRESS);
    else {
        addr = inet_addr(SQL_SERVER_ADDRESS);
        hp = gethostbyaddr((char*)&addr, sizeof(addr), AF_INET);
    }

    if (hp == NULL)        
        return printError(conn, errno, "get ip address host");

    sockaddr_in internetAddr;
    internetAddr.sin_family = AF_INET;
    internetAddr.sin_port = htons(SQL_SERVER_PORT);
    internetAddr.sin_addr.s_addr = *((unsigned long*)hp->h_addr);
    if (connect(conn.server.socket, (struct sockaddr*) &internetAddr, sizeof(internetAddr)))
        return printError(conn, errno, "sql-server connect failed");

    int n = 0, ret = 0;
    fd_set fdset;
    STATE_PARS state = STATE_PARS_INIT;
    while (true) {
        FD_ZERO(&fdset);
        FD_SET(conn.client.socket, &fdset);
        FD_SET(conn.server.socket, &fdset);
        timeval timeout {10};
        n = select(std::max(conn.client.socket, conn.server.socket)+1, &fdset, NULL, NULL, &timeout);
        if (n < 0)
            return printError(conn, errno, "select fdset");
        else if (n == 0)
            return printError(conn, errno, "time waiting out");
        else {
            if(FD_ISSET(conn.client.socket, &fdset)) {
                if((ret = readMessage(conn.client)) < 0)
                    return printError(conn, ret, "client readMessage");

                if(!conn.client.ready_send_nums.empty())
                    if((ret = sendMessage(conn.client, conn.server.socket)) < 0)
                        return printError(conn, ret, "client sendMessage");

            }
            if(FD_ISSET(conn.server.socket, &fdset)) {

                if((ret = readMessage(conn.server)) < 0)
                    return printError(conn, ret, "server readMessage");

                if(!conn.server.ready_send_nums.empty())
                    if((ret = sendMessage(conn.server, conn.client.socket)) < 0)
                        return printError(conn, ret, "server sendMessage");

            }
        }
    }
}

std::list<std::pair<char*, int>>* Server::readData(SocketInfo &sockInfo)
{

    return nullptr;
}

int Server::readMessage(SocketInfo &sockInfo)
{
    char buff[MAX_PACKET_SIZE];
    int len;
    int offset = 0;
    try {
        len = recv(sockInfo.socket, buff, MAX_PACKET_SIZE, 0);
        if(*reinterpret_cast<uint32_t*>(buff) == 0x0000003a) {
            //                std::cout << "stop" << std::endl;
        }
        if(len == 0) {
            std::cout << "stop" << std::endl;
            return 0;
        }
        if(len < 0) {
            std::cout << "recv error" << std::endl;
            throw ;
        }
        int lenght = len;
        while (len > 0) { // если длина всё ещё больше ожидаемого следующего размера запускаем снова
            sockInfo.initPackSeq(buff, offset);            // узнаём данные следующего пакета

            size_t remaind_length_buff = len < sockInfo.size + OFFSET_DATA_HEADER ? len : sockInfo.size + OFFSET_DATA_HEADER;
            // кладём буфер в список полученных кадров и указываем длину буфера
            sockInfo.buffer[sockInfo.id]->push_back({&buff[offset], remaind_length_buff});
            len -= remaind_length_buff;                                        // уменьшаем длину полученного буфера на длину записанного пакета
            offset += remaind_length_buff;                                     // записываем наше смещение в буфере
            sockInfo.current_size += remaind_length_buff;
            if(sockInfo.current_size == sockInfo.size + OFFSET_DATA_HEADER) {
                sockInfo.ready_send_nums.push(sockInfo.id);
                sockInfo.current_size = 0;
            }
            if(offset >= lenght)    //
                break;
        }

    }  catch (...) {
        std::cout << "ERROR: int Server::readMessage(SocketInfo&)" << std::endl;
        std::cerr << strerror(errno) << std::endl;
        return -1;
    }
    return 0;
}

int Server::sendMessage(SocketInfo &sockInfo, int sock_to)
{
    try {
        while(!sockInfo.ready_send_nums.empty()) {
            int ret = 0;
            size_t idx = sockInfo.ready_send_nums.front();
            sockInfo.stack_test_send.push(idx);
            sockInfo.ready_send_nums.pop();
            for(auto it = sockInfo.buffer.at(idx)->begin(); it != sockInfo.buffer.at(idx)->end(); it++) {
                if((ret = send(sock_to, it->first, it->second, MSG_NOSIGNAL)) < 0) {
                    this->parseSendError(ret);
                    throw ;
                }
                debug_traffic(sockInfo.name, it->first, it->second);
//              TODO  delete []it->first;
            }
//            sockInfo.buffer.erase(sockInfo.buffer.begin());
        }

    }  catch (...) {
        std::cout << "ERROR: int Server::sendMessage(SocketInfo&)" << std::endl;
        return errno;
    }
    return 0;
}

void Server::debug_traffic(std::string name, char *buff, int len)
{
    std::cout << name << " send=" << len << " " << "\t";
    for(size_t i = 0; i < 4; i++)
        std::cout << (static_cast<uint>(buff[i])&0xFF) << " ";
    std::cout << "\t";
    for(int i=3; i < len; i++) CHAR_OUT_LAMBDA(buff[i]);
    std::cout << std::endl;

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
        // ждём соединения
        clilen = sizeof (cliaddr);
        Request req;
        req.sockfd = (SOCKET)accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
        if(set.find(req.sockfd) == set.end()) {
            printf("connect from %s, port %d \en \n", inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)), ntohs(cliaddr.sin_port));
            set.insert(req.sockfd);
            std::thread thread(&Server::forwardRequest, this, req.sockfd);
            thread.detach();
        }
        req.sockaddr = cliaddr;
    }
}

void Server::connectionsHandler()
{

}

void Server::parseSendError(int ret)
{
    switch (ret) {
    case EBADF:
        std::cerr << "An invalid descriptor was specified" << std::endl;
        break;
    case ENOTSOCK:
        std::cerr << "The argument sockfd is not a socket" << std::endl;
        break;
    case EFAULT:
        std::cerr << "An invalid user space address was specified for an argument" << std::endl;
        break;
    case EMSGSIZE:
        std::cerr << "The socket type requires that message be sent atomically, and the size of the message to be sent made this impossible" << std::endl;
        break;
    case EAGAIN:
        std::cerr << "The socket is marked nonblocking and the requested operation would block. POSIX.1-2001 allows either error to be returned for this case, and does not require these constants to have the same value, so a portable application should check for both possibilities" << std::endl;
        break;
    case ENOBUFS:
        std::cerr << "The output queue for a network interface was full. This generally indicates that the interface has stopped sending, but may be caused by transient congestion. (Normally, this does not occur in Linux. Packets are just silently dropped when a device queue overflows.)" << std::endl;
        break;
    case EINTR:
        std::cerr << "A signal occurred before any data was transmitted; see signal(7)" << std::endl;
        break;
    case ENOMEM:
        std::cerr << "No memory available" << std::endl;
        break;
    case EINVAL:
        std::cerr << "Invalid argument passed" << std::endl;
        break;
    case EPIPE:
        std::cerr << "The local end has been shut down on a connection oriented socket. In this case the process will also receive a SIGPIPE unless MSG_NOSIGNAL is set" << std::endl;
        break;
    }
}

int SocketInfo::initPackSeq(char *buff, int offset)
{
    // узнаём данные следующего пакета
    uint8_t old_id = id;
    id = (*reinterpret_cast<uint32_t*>(buff) >> 24);
    if(id == 0) {
        clear();
    }
    if(id != old_id)
        buffer[id] = new std::list<std::pair<char*, int>>;
    size = (*reinterpret_cast<uint32_t*>(&buff[offset]) & 0x00FFFFFF);
    return 0;
}
