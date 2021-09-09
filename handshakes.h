#ifndef HANDSHAKES_H
#define HANDSHAKES_H
#include <iostream>
#include <string>
#include <vector>

namespace ProxyServer {


#define     LEN_VERSIGON_SQL                        6
#define     LEN_CONN_ID                             4
#define     LEN_PLUGINT_DATA                        8
#define     LEN_RESERVED                            10

#define     LEN_CLIENT_RESERVER_23                  23


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



class Handshakes
{   
    enum STATE_PROTOCOL_VERSION {STATE_HANDSHAKEV10, STATE_HANDSHAKEV9};

    enum STATE_HANDSHAKE {STATE_HANDSHAKE_SERVER, STATE_HANDSHAKE_CLIENT};

    enum STATE_SERVER {STATE_SERVER_PARSE_HEADER, STATE_HANDSHAKE_VERSION, STATE_SQL_VERSION, STATE_CONNECTION_ID, STATE_PLUGIN_DATA, STATE_FILTER,
                STATE_CAPACITY_FLAGS_LOWER, STATE_CHARACTER_SET, STATE_STATUS_FLAGS, STATE_CAPACITY_FLAGS_UPPER,
                STATE_CONSTANTE_ZEERO, STATE_AUTH_PLUGIN_DATA_LEN, STATE_RESERVERD,
                STATE_PLUGIN_DATA_2, STATE_AUTH_PLUGIT_NAME};

    enum STATE_CLIENT {CLIENT_PARSE_HEADER, CLIENT_FLAGS, CLIENT_MAX_PACKET_SIZE, CLIENT_PROTO_CHAR_SET, CLIENT_23_ZERO,
                      CLIENT_USERNAME, CLIENT_AUTH_RESPONSE, CLIENT_AUTH_RESPONSE_LENGTH, CLIENT_IF_CLIENT_CONNECT_WITH_DB,
                      CLIENT_IF_CLIENT_PLUGIN_AUTH, CLIENT_IF_CLIENT_CONNECT_ATTRS };

    STATE_HANDSHAKE state_handshake = STATE_HANDSHAKE_SERVER;
    STATE_SERVER state_server = STATE_SERVER_PARSE_HEADER;
    STATE_CLIENT state_client = CLIENT_PARSE_HEADER;

    STATE_PROTOCOL_VERSION handshake_version;

    uint32_t clie_client_flag;
    uint32_t clie_max_packet_size;
    uint8_t clie_character_set;
    std::vector<uint8_t> clie_23_zero;
    std::string clie_username;
    std::vector<uint8_t> clie_auth_response;
    uint8_t clie_auth_response_length;

    std::string sql_version;
    // variable of server
    uint32_t serv_connection_id;
    std::vector<uint8_t> serv_plugin_data;
    uint8_t serv_filter;                        // 0x00
    uint32_t capabilities_flags;
    uint16_t serv_status_flags;
    uint8_t serv_character_set;    /* mysql> SELECT CHARACTER_SET_NAME, COLLATION_NAME
                                        FROM INFORMATION_SCHEMA.COLLATIONS
                                        WHERE ID = 255; */
    int serv_auth_plugin_data_len;           // length of the combined auth_plugin_data
    std::string serv_auth_plugin_name;       // name of the auth_method that the auth_plugin_data belongs to
    std::vector<uint8_t> serv_reserverd;
    uint8_t serv_const_zeero;
    int serv_length_rest_plugin_part;

    void secure_password(std::vector<uint8_t> &v);
public:
    Handshakes();
    int parsePackage(char *buff, int len, int id);
};

}
#endif // HANDSHAKES_H
