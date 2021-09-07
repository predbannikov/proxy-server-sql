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



struct SocketInfo {
    SocketInfo() {
        clear();
    }
    int initPackSeq(char *buff, int offset);
    void clear() {
        buffer.clear();
        buffer[0] = new std::list<std::pair<char*, int>>;
        size = 0;
        id = 0;
        current_size = 0;
    }
    std::string name;
    std::map<int, std::list<std::pair<char*, int>>*> buffer;
    std::queue<int> ready_send_nums;
    std::stack<int> stack_test_recv;
    std::stack<int> stack_test_send;
    uint8_t id;                 // Командная фаза, сбрасывается в нуль при новой фазе
    uint32_t size;
    uint32_t current_size = 0;
    std::string sql_version;
    int socket = 0;
};

struct Connection {
    SocketInfo client;
    SocketInfo server;
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
    std::list<std::pair<char*, int>>* readData(SocketInfo &sockInfo);
    int readMessage(SocketInfo &sockInfo);
    int sendMessage(SocketInfo &sockInfo, int sock_to);
    void debug_traffic(std::string name, char *buff, int len);
    void parseSendError(int ret);
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
