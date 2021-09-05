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
#include <unordered_map>
#include <netdb.h>
#include <time.h>
#include <unordered_set>
#include <list>

#define MAX_PACKET_SIZE         65535		// Размер буфера для чтения recv
#define	MAXLINE					4096    /* max text line length */
#define ERROR_NUMER_RETURN  	-1
#define SOCK_MAX_CONN			2048
#define COUNT_REST_REQUEST		50
#define SERVER_IP4_ADDR 		"192.168.0.101"
#define SERV_PORT 				1500
#define	ONLYLOCALHOST			0

namespace ProxyServer {


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

enum STATE_PARS {STATE_PARS_INIT, STATE_PARS_NEXT};

struct Package {
    union HeaderPack {
        uint32_t _header;
        struct __header{
            uint32_t size: 24;
            uint8_t id: 8;           // Командная фаза, сбрасывается в нуль при новой фазе
        } head;
    } package;
    std::list<std::pair<char*, int>> buffer;
    int8_t current_size;
};

struct InfoConnect {
    Package client;
    Package server;
//    int index_package;  // номер пакета
    std::string sql_version;
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
    std::unordered_map<int, Request> connections;
    std::unordered_set<int> set;

    void pushConnectionSafeThr(Request &req);
    Request popConnectSafeThr();

    struct sockaddr_in cliaddr, servaddr;
    int	listenfd;


    void forwardRequest(int sockfd);

    // Сетевые

    fd_set fdread;

public:
    Server();
    void loop();
    void connectionsHandler();
};

}



#endif // PROXYSERVER_H
