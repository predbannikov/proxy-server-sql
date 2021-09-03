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
#include "proxyserver.h"
#include "stdio.h"
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <time.h>

#define	MAXLINE					4096    /* max text line length */
#define ERROR_NUMER_RETURN  	-1
#define SOCK_MAX_CONN			2048

#define SERVER_IP4_ADDR 		"192.168.0.101"
#define SERV_PORT 				1500
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



class Server
{
    std::thread thr_loop;
    std::thread thr_connect_handler;
    std::thread thr_main;
    std::queue<int> connections;
    struct sockaddr_in cliaddr, servaddr;
    int	listenfd;
public:
    Server();
    void loop();
    void connectionsHandler();
    void mainThread();
};

}



#endif // PROXYSERVER_H
