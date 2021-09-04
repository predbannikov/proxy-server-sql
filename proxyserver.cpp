#include "proxyserver.h"
#include "urls.h"

using namespace ProxyServer;

Urls url;

void Server::pushConnectionSafeThr(int conn)
{
    std::unique_lock<std::mutex> lock(mtx_connections);
    connections.push(conn);
}

int Server::popConnectSafeThr()
{
    std::unique_lock<std::mutex> lock(mtx_connections);
    if(!connections.empty()) {
        int conn = connections.front();
        connections.pop();
        return conn;
    }
    return -1;
}

void Server::procMapRequests() {

    // переделать поиск и удаление на лямбду
    for (auto it = mapRequests.begin(); it != mapRequests.end(); ++it) {
        if (it->second.done) {
            //std::cout << "change request" << std::endl;
            //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            close(it->second.sock);
            mapRequests.erase(it);
            break;
        }
    }


    while (mapRequests.size() < COUNT_REST_REQUEST) {
        if (createRequest())
            std::cout << "don't create request" << std::endl;// Нужно что-то для контроля, если не будет удаваться создать!!!
    }


}

int Server::createRequest()
{
    std::string srcREST;
    if (ONLYLOCALHOST)
        srcREST = url.at(0);
    else
        srcREST = url.getSrcRNDRESTRequest();

    Request req;
    int ret = 0;
    ret = setRequest(srcREST, req);
    if (!ret) {
        ret = execRequest(req);
        if (!ret) {
            mapRequests.insert(std::pair<SOCKET, Request>(req.sock, req));
            return 0;
        }
        else {
            std::cerr << "error ret execRequest != 0 " << ret << std::endl;
            close(req.sock);
        }
    }
    else {
        std::cerr << "error ret setRequest != 0 " << ret << std::endl;

        close(req.sock);
    }
    return 1;
}

int Server::setRequest(std::string a_sSrcREST, Request &req)
{
    SOCKET sock;
    req.sSrcREST = a_sSrcREST;
    parseUrl(a_sSrcREST.c_str(), req.sServer, req.sPath, req.sFileName);
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        std::cout << "create socket failed with INVALID_SOCKET" << std::endl;
        return errno;
    }
    req.sock = sock;
    struct hostent *hp;
    unsigned int addr;

    const char *serverName = (char *)req.sServer.c_str();
    if (inet_addr(serverName) == INADDR_NONE)
    {
        hp = gethostbyname(serverName);
    }
    else
    {
        addr = inet_addr(serverName);
        hp = gethostbyaddr((char*)&addr, sizeof(addr), AF_INET);

    }

    if (hp == NULL)
    {
        std::cout << "get ip address host failed " << std::endl;
        //closesocket(sock);
        return errno;
    }
    sockaddr_in internetAddr;
    internetAddr.sin_family = AF_INET;
    internetAddr.sin_port = htons(SERV_PORT);
    internetAddr.sin_addr.s_addr = *((unsigned long*)hp->h_addr);
    req.sockaddr = internetAddr;

    return 0;
}

int Server::execRequest(Request &req)
{
    if (connect(req.sock, (struct sockaddr*) &req.sockaddr, sizeof(req.sockaddr)))
    {
        std::cout << "connect failed with error " << std::endl;
        return errno;
    }

    req.sReqHTTP = "GET " + req.sPath + " HTTP/1.0\r\n" + "Host: " + req.sServer + " \r\n\r\n";

    // TODO сенд возвращает число меньшее числа размера сообщения, нужно доотправить в цикле
    if (send(req.sock, req.sReqHTTP.c_str(), (int)strlen(req.sReqHTTP.c_str()), 0) < 0)
    {
        std::cout << "send failed with error" << std::endl;
        return errno;
    }
    return 0;
}

void Server::parseUrl(const char *mUrl, std::string &serverName, std::string &filepath, std::string &filename)
{
    std::string::size_type n;
    std::string url = mUrl;
    if (url.substr(0, 7) == "http://")
        url.erase(0, 7);

    if (url.substr(0, 8) == "https://")
        url.erase(0, 8);

    n = url.find('/');
    if (n != std::string::npos)
    {
        serverName = url.substr(0, n);
        filepath = url.substr(n);
        n = filepath.rfind('/');
        filename = filepath.substr(n + 1);
    }
    else {
        serverName = url;
        filepath = "/";
        filename = "";
    }
}



Server::Server() {
    //инициализируем начальные значения

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
        connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
        printf("connect from %s, port %d \en", inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)), ntohs(cliaddr.sin_port));
        int len;
        if((len = recv(connfd, (char *)&buff, MAXLINE, 0)) < 0) {
            return ;
        }
//        char *data = new char[len];
//        memcpy(data, buff, len);
        std::cout << buff << std::endl;
//        delete []data;
        std::cout << std::endl;
        time_t ticks;
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%24s\er\en", ctime(&ticks));
        write(connfd, buff, strlen(buff));
        // сокет кладём в очередь обработки
        //pushConnectionSafeThr(connfd);
        // Если в очереди меньше одного элемента
        if(connections.size() < 2) {
            // даём сисгнал на пробуждение

        }

    }
}

void Server::connectionsHandler() {
    int ret = 0;
    char response[] = "HTTP/1.0 200 Connection established\nProxy-agent: ProxyApp/1.1";
    while(true) {
        // Будим поток
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        int len = 0;
//        if((len = recv()))
        while(!connections.empty()) {
            std::cout << "is connection " << std::endl;
            int conn = popConnectSafeThr();

            write(conn, response, sizeof (response));


            if(conn < 0)
                break;
            FD_ZERO(&fdread);


            for (auto it = mapRequests.begin(); it != mapRequests.end(); ++it) {
                if (it->second.done) {

                    continue;
                }
            }
            FD_SET(conn, &fdread);




            ret = 0;

            //if ((ret = select(0, &fdread, NULL, NULL, &time_out)) == SOCKET_ERROR)
            timeval timeout = {1};
            if ((ret = select(0, &fdread, NULL, NULL, &timeout)) <= 0)
            {
                std::cout << "select error stop" << errno << std::endl;
                continue;
            }
            // Если в каких то каналах есть данные для чтения
            if (ret != 0)
            {
                std::cout << "Есть данные для чтения" << std::endl;
//                ret = readSock();
//                if (ret) {
//                    printError(ret);
//                    continue;
//                }
            }
//            if (countRequest == PACK_SIZE_RESPONSE) {
//                countRequest = 0;
//                shareMem->writeData(g_vec);
//                g_vec = new std::vector<char *>(PACK_SIZE_RESPONSE, NULL);
//            }
        }
//        std::cout << "exit cycle, need sleep thread" << std::endl;
        // усыпляем поток
    }
}

void Server::mainThread() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // получить запрос sql
    // создать сокет для отправки пакета
    // отправить запрос дальше
    // получили ответ, возвращаем получателю
}
