#include "proxyserver.h"


int main(int argc, char **argv)
{
    ProxyServer::Server server;
    server.loop();
    return 0;
}
