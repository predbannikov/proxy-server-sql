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


bool STOP_SERVER = false;

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


class TServer
{

public:
    int openSession(){
        int 		listenfd, connfd;
        socklen_t 	clilen;
        char		buff[MAXLINE];
        struct sockaddr_in cliaddr, servaddr;

        memset(&servaddr, 0, sizeof (servaddr));
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(SERV_PORT);
        if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            std::cout << "socket can't create" << std::endl;
            return ERROR_NUMER_RETURN;
        }
        if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
            if(errno == 13)
            std::cout << "listen port less 1024 assigned for root, you start server how user?" << std::endl;
            std::cout << "listen error " << errno << std::endl;
            return ERROR_NUMER_RETURN;
        }
        if(listen(listenfd, SOCK_MAX_CONN) < 0) {
            std::cout << "listen error" << std::endl;
            close(connfd);
            return ERROR_NUMER_RETURN;
        }
        while(!STOP_SERVER) {
            clilen = sizeof (cliaddr);
            connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
            time_t ticks = time(NULL);
            snprintf(buff, sizeof (buff), "%.24s\er\en", ctime(&ticks));
            write(connfd, buff, strlen(buff));
            close(connfd);
//            if((childpid = fork()) == 0) {
//                // Дочерний процесс
//                close(listenfd);
//                std::cout << "child process" << std::endl;
//                exit(0);
//            }
        }
        close(connfd);
        std::cout << "stop server" << std::endl;
        return 0;
    }
    void worker()  {

        std::cout << "worker server" << std::endl;
        openSession();
    }
    TServer() {
    }
    ~TServer() {
        std::cout << "detructor server" << std::endl;
    }
};





int main(int argc, char **argv)
{

    TServer server;
    std::thread t(&TServer::worker, &server);
    thread_guard g(t);

    return 0;
}
