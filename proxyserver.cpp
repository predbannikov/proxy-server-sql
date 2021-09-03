#include "proxyserver.h"

using namespace ProxyServer;


Server::Server() {
    //инициализируем начальные значения

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
    // передаём управление в loop
    thr_loop = std::thread(&Server::loop, this);
    thread_guard guard_loop(thr_loop);
    // запустить поток обработки очереди
    thr_connect_handler = std::thread(&Server::connectionsHandler, this);
    thread_guard guard_conn_hand(thr_connect_handler);
    // запустить поток обработки соединений
    thr_main = std::thread(&Server::mainThread, this);
    thread_guard guard_main(thr_main);
}

void Server::loop() {
    socklen_t 	clilen;
    int 		connfd;
    char		buff[MAXLINE];
    while(true) {
        // ждём соединения
        clilen = sizeof (cliaddr);
        connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
        // сокет кладём в очередь обработки
        connections.push(connfd);
        // Если в очереди меньше одного элемента
        if(connections.size() < 2) {
            // даём сисгнал на пробуждение

        }
    }
}

void Server::connectionsHandler() {
    while(true) {
        // Будим поток
        while(!connections.empty()) {
            // Запускаем поток на выполнение задачи и передачи на следующий адрессный путь
            // Удаляем из очереди
        }
        // усыпляем поток
    }
}

void Server::mainThread() {
    // получить запрос sql
    // создать сокет для отправки пакета
    // отправить запрос дальше
    // получили ответ, возвращаем получателю
}
