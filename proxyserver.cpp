#include "proxyserver.h"
#include <algorithm>


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
                if((ret = readMessage(conn.client)) <= 0)
                    return printError(conn, ret, "client readMessage");

                conn.parseHandShake(Connection::STATE_RECV_CLIENT);

                if((ret = sendMessage(conn.client, conn.server.socket)) < 0)
                    return printError(conn, ret, "client sendMessage");

            }
            if(FD_ISSET(conn.server.socket, &fdset)) {

                if((ret = readMessage(conn.server)) <= 0)
                    return printError(conn, ret, "server readMessage");

                conn.parseHandShake(Connection::STATE_RECV_SERVER);

                if((ret = sendMessage(conn.server, conn.client.socket)) < 0)
                    return printError(conn, ret, "server sendMessage");

            }
        }
    }
}

int Server::readMessage(SocketInfo &sockInfo)
{
    static int debug_int;
    char *buff = (char*)malloc(MAX_PACKET_SIZE);
    int len;
    int offset = 0;
    try {
        len = recv(sockInfo.socket, buff, MAX_PACKET_SIZE, 0);
        if(len == 0) {
            std::cout << "stop" << std::endl;
            return 0;
        }
        if(len < 0) {
            std::cout << "recv error" << std::endl;
            throw ;
        }
        if(len > 3000) {
            debug_int++;
        }
        if(sockInfo.name == "client") {
            std::cout << "stop" << std::endl;
        }
//        debug_traffic(sockInfo.name, "recv", buff, len);
        while (len > 0) { // если длина всё ещё больше ожидаемого следующего размера запускаем снова

            sockInfo.initPackSeq(buff, offset, len);            // узнаём данные следующего пакета

            // кладём буфер в список полученных кадров и указываем длину буфера
            sockInfo.buffer->second->push_back({&buff[offset], sockInfo.remaind_length_buff});
            len -= sockInfo.remaind_length_buff;                                        // уменьшаем длину полученного буфера на длину записанного пакета
            offset += sockInfo.remaind_length_buff;                                     // записываем наше смещение в буфере
            sockInfo.current_size += sockInfo.remaind_length_buff;

            if(sockInfo.current_size == sockInfo.size + OFFSET_DATA_HEADER) {
                // в очередь на отправку добавляем готовый пакет
                sockInfo.date_to_send.push(sockInfo.buffer);
                // создаём сразу память для следующей последовательности данных
                sockInfo.buffer = new std::pair<int, std::list<std::pair<char*, int>>*>;
                sockInfo.current_size = 0;
            }
        }

    }  catch (...) {
        std::cout << "ERROR: int Server::readMessage(SocketInfo&)" << std::endl;
        std::cerr << strerror(errno) << std::endl;
        return -1;
    }
    return 1;
}

int Server::sendMessage(SocketInfo &sockInfo, int sock_to)
{
    static int debug_int;
    try {
        while(!sockInfo.date_to_send.empty()) {
            int ret = 0;
            std::pair<int, std::list<std::pair<char*, int>>*> *buffer = sockInfo.date_to_send.front();
            sockInfo.date_to_send.pop();
            for(auto it = buffer->second->begin(); it != buffer->second->end(); it++) {
                if((ret = send(sock_to, it->first, it->second, MSG_NOSIGNAL)) < 0) {
                    std::cerr << "send error " << std::strerror(errno) << std::endl;
                    throw ;
                }
                debug_traffic(sockInfo.name, "send", it->first, it->second);
//              TODO  delete []it->first;
            }
//            sockInfo.parseData(buffer);
            sockInfo.hystory.push_back(buffer);
        }
    }  catch (...) {
        std::cout << "ERROR: int Server::sendMessage(SocketInfo&)" << std::endl;
        return errno;
    }
    return 0;
}

void Server::debug_traffic(std::string name, std::string func, const char *buff, int len)
{
    std::cout << name << " " << func << "=" << len << " " << "\t";
    for(size_t i = 0; i < 4; i++)
        std::cout << (static_cast<uint>(buff[i])&0xFF) << " ";
    std::cout << "\t";
    std::string tmp = "SELECT";
    int idx = 0;

    for(int i=4; i < len; i++) {
        if(buff[i] == tmp[idx]) {
            idx++;
            if(idx==6) {
                std::cout << "found" << std::endl;
                for(int i = 0; i < len; i++) {
                    std::cout << std::hex << (0xFF&buff[i]) << ":";
                }
                std::cout << std::endl << "--------------------------------------------------" << std::endl;
            }
        } else{
            idx = 0;
        }
        CHAR_OUT_LAMBDA(buff[i]);
    }
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
    static int debug_int = 0;
    while(true) {
        // ждём соединения
        clilen = sizeof (cliaddr);
        Request req;
        req.sockfd = (SOCKET)accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
//        if(debug_int > 0)
//            continue;
        if(set.find(req.sockfd) == set.end()) {
            printf("connect from %s, port %d \en \n", inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)), ntohs(cliaddr.sin_port));
            set.insert(req.sockfd);
            std::thread thread(&Server::forwardRequest, this, req.sockfd);
            thread.detach();
            debug_int++;
        } else {
            printf("alredy connect from %s, port %d \en \n", inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)), ntohs(cliaddr.sin_port));
        }
//        req.sockaddr = cliaddr;
    }
}

void Server::connectionsHandler()
{

}

int SocketInfo::initPackSeq(char *buff, int offset, int len)
{
    if(current_size == 0) {
        int id_tmp = (*reinterpret_cast<uint32_t*>(buff) >> 24);
        if(id_tmp == 0) {
            newPhase();
        } else {
            id = (*reinterpret_cast<uint32_t*>(&buff[offset]) >> 24);        // offset тут не нужен, так как этот блок else выполняется только в случае
//            id = (*reinterpret_cast<uint32_t*>(buff) >> 24);        // offset тут не нужен, так как этот блок else выполняется только в случае
            buffer->first = id;
            buffer->second = new std::list<std::pair<char*, int>>;
        }
        size = (*reinterpret_cast<uint32_t*>(&buff[offset]) & 0x00FFFFFF);
        remaind_length_buff = len < size + OFFSET_DATA_HEADER ? len : size + OFFSET_DATA_HEADER;
    } else {
        remaind_length_buff = size - remaind_length_buff + OFFSET_DATA_HEADER;
    }
    return 0;
}

int Connection::parseHandShake(STATE state)
{
    try {
        std::pair<int, std::list<std::pair<char*, int>>*> *buffer;
        switch (state) {
        case STATE_RECV_SERVER:
            if(server.date_to_send.empty())
                return -1;
            buffer = server.date_to_send.front();
            break;
        case STATE_RECV_CLIENT:
            if(client.date_to_send.empty())
                return -1;
            buffer = client.date_to_send.front();
            break;
        case STATE_SEND_SERVER:
            break;
        case STATE_SEND_CLIENT:
            break;
        }
        for(auto it = buffer->second->begin(); it != buffer->second->end(); it++)
            handshake.parsePackage(it->first, it->second, buffer->first);

    }  catch (...) {
        std::cout << "ERROR: int Server::sendMessage(SocketInfo&)" << std::endl;
        return errno;
    }
    return 0;

    return 0;
}
