#ifndef PROXYSERVER_H
#define PROXYSERVER_H
#include <queue>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cstring>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <netdb.h>
#include <time.h>
#include <unordered_set>
#include <map>
#include <list>
#include <stack>
#include <vector>
#include "handshakes.h"

#define MAX_PACKET_SIZE         4096		// Размер буфера для чтения recv
#define	MAXLINE					4096    /* max text line length */
#define ERROR_NUMER_RETURN  	-1
#define SOCK_MAX_CONN			2048
#define SERVER_IP4_ADDR 		"192.168.0.101"
#define SERV_PORT 				1500
#define SQL_SERVER_ADDRESS      "127.0.0.1"
#define SQL_SERVER_PORT         3306
#define OFFSET_DATA_HEADER      4


#define LOCAL_INFILE_REQ        0xFB
#define ERR_Packet              0xFF


namespace ProxyServer {


enum COM_STATE {
    COM_SLEEP               = 0x00,
    COM_QUIT                = 0x01,
    COM_INIT_DB             = 0x02,
    COM_QUERY               = 0x03,
    COM_FIELD_LIST          = 0x04,
    COM_CREATE_DB           = 0x05,
    COM_DROP_DB             = 0x06,
    COM_REFRESH             = 0x07,
    COM_SHUTDOWN            = 0x08,
    COM_STATISTICS          = 0x09,
    COM_PROCESS_INFO        = 0x0a,
    COM_CONNECT             = 0x0b,
    COM_PROCESS_KILL        = 0x0c,
    COM_DEBUG               = 0x0d,
    COM_PING                = 0x0e,
    COM_TIME                = 0x0f,
    COM_DELAYED_INSERT      = 0x10,
    COM_CHANGE_USER         = 0x11,
    COM_BINLOG_DUMP         = 0x12,
    COM_TABLE_DUMP          = 0x13,
    COM_CONNECT_OUT         = 0x14,
    COM_REGISTER_SLAVE      = 0x15,
    COM_STMT_PREPARE        = 0x16,
    COM_STMT_EXECUTE        = 0x17,
    COM_STMT_SEND_LONG_DATA = 0x18,
    COM_STMT_CLOSE          = 0x19,
    COM_STMT_RESET          = 0x1a,
    COM_SET_OPTION          = 0x1b,
    COM_STMT_FETCH          = 0x1c,
    COM_DAEMON              = 0x1d,
    COM_BINLOG_DUMP_GTID    = 0x1e,
    COM_RESET_CONNECTION    = 0x1f
};

class thread_guard {
    std::thread& t;
public:
    explicit thread_guard(std::thread &t_) : t(t_) {}
    ~thread_guard() {
        if(t.joinable()) {
            t.join();
        }
    }
    thread_guard(thread_guard const&) = delete;
    thread_guard& operator=(thread_guard const&) = delete;
};

class scoped_thread{
    std::thread t;
public:
    explicit scoped_thread(std::thread t_) : t(std::move(t_)) {
        if(!t.joinable())
            throw std::logic_error("No thread");
    }
    ~scoped_thread() {
        t.join();
    }
    scoped_thread(scoped_thread const&)=delete;
    scoped_thread& operator=(scoped_thread const&)=delete;
};

struct SocketInfo {
    SocketInfo() {
        buffer = new std::pair<int, std::list<std::pair<char*, int>>*> {0, new std::list<std::pair<char*, int>> };
        size = 0;
        id = 0;
        current_size = 0;
    }
    int initPackSeq(char *buff, int offset, int len);
    void newPhase() {
        size = 0;
        id = 0;
        buffer = new std::pair<int, std::list<std::pair<char*, int>>*> {id, new std::list<std::pair<char*, int>> };
        current_size = 0;
    }
    std::string name;
    std::pair<int, std::list<std::pair<char*, int>>*> *buffer;
    std::queue<std::pair<int, std::list<std::pair<char*, int>>*>*> date_to_send;
    std::list<std::pair<int, std::list<std::pair<char*, int>>*>*> hystory;
    uint8_t id = 1;                 // Командная фаза, сбрасывается в нуль при новой фазе
    uint32_t size;
    uint32_t current_size = 0;
    uint32_t remaind_length_buff = 0;
    int socket = 0;
};

struct Connection {
    enum STATE {STATE_RECV_SERVER, STATE_RECV_CLIENT, STATE_SEND_SERVER, STATE_SEND_CLIENT}; // Последнее событие
    SocketInfo client;
    SocketInfo server;

    int parseHandShake(STATE state);

    Handshakes handshake;
};


using SOCKET = int;
struct Request {
    char *data = NULL;          // Sql пакет
    std::string sSrcREST;
    std::string sReqHTTP;
    std::string sServer;
    std::string sPath;
    std::string sFileName;
    sockaddr_in sockaddr;
    SOCKET		sockfd;
    int totalLength = 0;
    bool done = false;
};

class Server
{
    std::thread thr_connect_handler;
    std::mutex mtx_connections;
    std::unordered_set<int> set;


    struct sockaddr_in cliaddr, servaddr;
    int	listenfd;


    int forwardRequest(int sockfd);

    // Сетевые

    fd_set fdread;
    int readMessage(SocketInfo &sockInfo);
    int sendMessage(SocketInfo &sockInfo, int sock_to);
    void debug_traffic(std::string name, std::string func, const char *buff, int len);
public:
    Server();
    void loop();
    void connectionsHandler();
private:
    void get_version(SocketInfo &packag);
    void printBuffer(std::list<std::pair<char*, int>> &buffer);
};

}



#endif // PROXYSERVER_H
