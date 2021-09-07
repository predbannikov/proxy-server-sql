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




#define         LEN_VERSIGON_SQL    6
#define         LEN_CONN_ID         4
#define         LEN_PLUGINT_DATA    8
#define         LEN_RESERVED        10


/* Status Flags */
#define     SERVER_STATUS_IN_TRANS                  0x0001	//a transaction is active
#define     SERVER_STATUS_AUTOCOMMIT                0x0002	//auto-commit is enabled
#define     SERVER_MORE_RESULTS_EXISTS              0x0008	//
#define     SERVER_STATUS_NO_GOOD_INDEX_USED        0x0010	//
#define     SERVER_STATUS_NO_INDEX_USED             0x0020	//
#define     SERVER_STATUS_CURSOR_EXISTS             0x0040	//Used by Binary Protocol Resultset to signal that COM_STMT_FETCH must be used to fetch the row-data.
#define     SERVER_STATUS_LAST_ROW_SENT             0x0080	//
#define     SERVER_STATUS_DB_DROPPED                0x0100	//
#define     SERVER_STATUS_NO_BACKSLASH_ESCAPES      0x0200	//
#define     SERVER_STATUS_METADATA_CHANGED          0x0400	//
#define     SERVER_QUERY_WAS_SLOW                   0x0800	//
#define     SERVER_PS_OUT_PARAMS                    0x1000	//
#define     SERVER_STATUS_IN_TRANS_READONLY         0x2000	//in a read-only transaction
#define     SERVER_SESSION_STATE_CHANGED            0x4000	//connection state information has changed

/*  Capability Flags */
#define     CLIENT_LONG_PASSWORD                    0x00000001
#define     CLIENT_FOUND_ROWS                       0x00000002
#define     CLIENT_LONG_FLAG                        0x00000004
#define     CLIENT_CONNECT_WITH_DB                  0x00000008
#define     CLIENT_NO_SCHEMA                        0x00000010
#define     CLIENT_COMPRESS                         0x00000020
#define     CLIENT_ODBC                             0x00000080
#define     CLIENT_IGNORE_SPACE                     0x00000100
#define     CLIENT_PROTOCOL_41                      0x00000200
#define     CLIENT_INTERACTIVE                      0x00000400
#define     CLIENT_SSL                              0x00000800
#define     CLIENT_IGNORE_SIGPIPE                   0x00001000
#define     CLIENT_TRANSACTIONS                     0x00002000
#define     CLIENT_RESERVED                         0x00004000
#define     CLIENT_SECURE_CONNECTION                0x00008000
#define     CLIENT_MULTI_STATEMENTS                 0x00010000
#define     CLIENT_MULTI_RESULTS                    0x00020000
#define     CLIENT_PS_MULTI_RESULTS                 0x00040000
#define     CLIENT_PLUGIN_AUTH                      0x00080000
#define     CLIENT_CONNECT_ATTRS                    0x00100000
#define     CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA   0x00200000
#define     CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS     0x00400000
#define     CLIENT_SESSION_TRACK                    0x00800000
#define     CLIENT_DEPRECATE_EOF                    0x01000000


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
    uint8_t id;                 // Командная фаза, сбрасывается в нуль при новой фазе
    uint32_t size;
    uint32_t current_size = 0;
    uint32_t remaind_length_buff = 0;
    int socket = 0;

    enum STATE {STATE_HANDSHAKE, STATE_VERSET_VERSION, STATE_CONNECTION_ID, STATE_PLUGIN_DATA, STATE_FILTER,
                STATE_CAPACITY_FLAGS_LOWER, STATE_CHARACTER_SET, STATE_STATUS_FLAGS, STATE_CAPACITY_FLAGS_UPPER,
                STATE_CONSTANTE_ZEERO, STATE_AUTH_PLUGIN_DATA_LEN, STATE_RESERVERD,
                STATE_PLUGIN_DATA_2, STATE_AUTH_PLUGIT_NAME} state = STATE_HANDSHAKE;
    uint8_t handshake;
    std::string sql_version;
    uint32_t connection_id;
    std::vector<uint8_t> plugin_data;
    uint8_t filter;                        // 0x00
    uint32_t capability_flags;
    uint16_t status_flags;
    uint8_t character_set;    /* mysql> SELECT CHARACTER_SET_NAME, COLLATION_NAME
                                        FROM INFORMATION_SCHEMA.COLLATIONS
                                        WHERE ID = 255; */
    int auth_plugin_data_len;       // length of the combined auth_plugin_data
    std::string auth_plugin_name;       // name of the auth_method that the auth_plugin_data belongs to
    std::vector<uint8_t> reserverd;
    uint8_t const_zeero;
    int length_rest_plugin_part;
    int parseData(std::pair<int, std::list<std::pair<char*, int>>*> *buffer);
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
    int readMessage(SocketInfo &sockInfo);
    int sendMessage(SocketInfo &sockInfo, int sock_to);
    void debug_traffic(std::string name, std::string func, const char *buff, int len);
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
