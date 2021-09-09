#include "handshakes.h"
#include <openssl/sha.h>
#include <openssl/objects.h>
#include <algorithm>
#include <codecvt>
#include <locale>
#include <string>
#include <cassert>

using namespace ProxyServer;

Handshakes::Handshakes()
{

}

int Handshakes::parsePackage(char *buff, int len, int id) {
    int tmp = 0;
    unsigned int utmp = 0;
    try {
        for(int i = 0; i < len; i++) {
            char ch = buff[i];
            switch (state_handshake) {
            // ***********************       STATE_HANDSHAKE_SERVER       ************************
            case STATE_HANDSHAKE_SERVER:
                switch (state_server) {
                case STATE_SERVER_PARSE_HEADER:
                    if(id != *reinterpret_cast<uint32_t*>(buff) >> 24) {
                        std::cerr << "id of pack not matched this header" << std::endl;
                        throw ;
                    }
                    i += 3;
                    state_server = STATE_HANDSHAKE_VERSION;
                    break;
                case STATE_HANDSHAKE_VERSION:
                    if(ch == 0x0a)
                        handshake_version = STATE_HANDSHAKEV10;
                    else
                        handshake_version = STATE_HANDSHAKEV9;
                    state_server = STATE_SQL_VERSION;
                    break;
                case STATE_SQL_VERSION:
                    tmp++;
                    sql_version.push_back(ch);
                    if(tmp == LEN_VERSIGON_SQL) {
                        tmp = 0;
                        state_server = STATE_CONNECTION_ID;
                    }
                    break;
                case STATE_CONNECTION_ID:
                    serv_connection_id = *reinterpret_cast<uint32_t*>(&buff[i]);
                    i += LEN_CONN_ID;
                    state_server = STATE_PLUGIN_DATA;
                    break;
                case STATE_PLUGIN_DATA:
                    tmp++;
                    serv_plugin_data.push_back(ch);
                    if(tmp == LEN_PLUGINT_DATA) {
                        tmp = 0;
                        state_server = STATE_FILTER;
                    }
                    break;
                case STATE_FILTER:
                    serv_filter = ch;
                    state_server = STATE_CAPACITY_FLAGS_LOWER;
                    break;
                case STATE_CAPACITY_FLAGS_LOWER:
                    capabilities_flags = *reinterpret_cast<uint16_t*>(&buff[i]);
                    i++;
                    state_server = STATE_CHARACTER_SET;
                    break;
                case STATE_CHARACTER_SET:
                    serv_character_set = ch;
                    state_server = STATE_STATUS_FLAGS;
                    break;
                case STATE_STATUS_FLAGS:
                    serv_status_flags = *reinterpret_cast<uint16_t*>(&buff[i]);
                    i++;
                    state_server = STATE_CAPACITY_FLAGS_UPPER;
                    break;
                case STATE_CAPACITY_FLAGS_UPPER:
                    utmp = *reinterpret_cast<uint16_t*>(&buff[i]);
                    capabilities_flags |= utmp << 16;
                    i++;
                    if(capabilities_flags & CLIENT_PLUGIN_AUTH)
                        state_server = STATE_AUTH_PLUGIN_DATA_LEN;
                     else
                        state_server = STATE_CONSTANTE_ZEERO;                    // std::cout << "STATE_AUTH_PLUGIN_DATA_LEN not support" << std::endl;
                    break;
                case STATE_AUTH_PLUGIN_DATA_LEN:
                    serv_auth_plugin_data_len = ch;
                    state_server = STATE_RESERVERD;
                    break;
                case STATE_CONSTANTE_ZEERO:
                    serv_const_zeero = ch;
                    state_server = STATE_RESERVERD;
                    break;
                case STATE_RESERVERD:
                    serv_reserverd.push_back(ch);
                    tmp++;
                    if(tmp == LEN_RESERVED) {
                        tmp = 0;
                        state_server = STATE_PLUGIN_DATA_2;
                    }
                    break;
                case STATE_PLUGIN_DATA_2:
                    if(++tmp == std::max(13, serv_auth_plugin_data_len-8)) {
                        state_server = STATE_AUTH_PLUGIT_NAME;
                        tmp = 0;
                    } else
                        serv_plugin_data.push_back(ch);
                    break;
                case STATE_AUTH_PLUGIT_NAME:
                    if(i == len - 1)
                        state_handshake = STATE_HANDSHAKE_CLIENT;
                    else
                        serv_auth_plugin_name.push_back(ch);
                    break;
                }

                break;
            // ***********************       STATE_HANDSHAKE_CLIENT       ************************
            case STATE_HANDSHAKE_CLIENT:
                switch (state_client) {
                case CLIENT_PARSE_HEADER:
                    if(id != *reinterpret_cast<uint32_t*>(buff) >> 24) {
                        std::cerr << "id of pack not matched this header" << std::endl;
                        throw ;
                    }
                    i += 3;
                    state_client = CLIENT_FLAGS;
                    break;
                case CLIENT_FLAGS:
                    clie_client_flag = *reinterpret_cast<uint32_t*>(&buff[i]);
                    i += 3;
                    state_client = CLIENT_MAX_PACKET_SIZE;
                    break;
                case CLIENT_MAX_PACKET_SIZE:
                    clie_max_packet_size = *reinterpret_cast<uint32_t*>(&buff[i]);
                    i += 3;
                    state_client = CLIENT_PROTO_CHAR_SET;
                    break;
                case CLIENT_PROTO_CHAR_SET:
                    clie_character_set = buff[i];
                    state_client = CLIENT_23_ZERO;
                    break;
                case CLIENT_23_ZERO:
                    tmp++;
                    clie_23_zero.push_back(buff[i]);
                    if(tmp == LEN_CLIENT_RESERVER_23) {
                        tmp = 0;
                        state_client = CLIENT_USERNAME;
                    }
                    break;
                case CLIENT_USERNAME:
                    if(buff[i] != 0)
                        clie_username.push_back(buff[i]);
                    else {
                        i++;
                        if(clie_client_flag & CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA)
                            state_client = CLIENT_AUTH_RESPONSE;
                        else
                            state_client = CLIENT_AUTH_RESPONSE_LENGTH;
                    }
                    break;
                case CLIENT_AUTH_RESPONSE:
                    tmp++;
                    clie_auth_response.push_back(buff[i]);          // Длина неизвестна
                    if(tmp == 20) {
                        tmp = 0;
                        std::cout << std::endl;
                        std::wstring password(L"555555");

                        std::vector<uint8_t> pass {'5','5','5','5','5','5'};
                        std::vector<uint8_t> binary_hex_vec(pass.size()*2);
                        char binary_hex[13];
                        std::cout << "stop" << std::endl;
                        for(int j = 0; j < pass.size(); j++)
                        {
                            sprintf(binary_hex + (i * 2), "%02x", pass[j]);
                            binary_hex_vec[j*2] = binary_hex[j*2];
                            binary_hex_vec[j*2+1] = binary_hex[j*2+1];
                        }
                        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
                        std::string utf8 = convert.to_bytes(0x5e9);
                        assert(utf8.length() == 2);
                        assert(utf8[0] == '\xD7');
                        assert(utf8[1] == '\xA9');


                        std::cout << "stop" << std::endl;


                        std::vector<uint8_t> hash_pass(SHA_DIGEST_LENGTH);
                        SHA1(binary_hex_vec.data(), binary_hex_vec.size(), hash_pass.data());
                        std::cout << "stop" << std::endl;

//                        std::vector<uint8_t> hash(SHA_DIGEST_LENGTH);
//                        SHA1(hash_pass.data(), hash_pass.size(), hash.data());

////                        std::cout << binary_hex << std::endl;

//                        serv_plugin_data.insert(serv_plugin_data.end(), hash.begin(), hash.end());
//                        SHA1(serv_plugin_data.data(), serv_plugin_data.size(), hash.data());

//                        std::cout << std::endl;
//                        std::transform(hash.begin(), hash.end(), hash_pass.begin(), hash_pass.end(),
//                                       [](auto e1, auto e2) {
//                            return e1^e2;
//                        });
////                        std::transform(a.begin(), a.end(), b.begin(),
////                            a.begin(), std::bit_xor<uint8_t>());
//                        for(int j = 0; j < 20; j++)
//                            std::cout << (0xFF & hash[j]) << ":";
//                        std::cout << std::endl;


//                        for(int j = 0; j < 20; j++)
//                            std::cout << (0xFF & clie_auth_response[j]) << ":";

//                        std::cout << std::endl;
                        state_client = CLIENT_IF_CLIENT_CONNECT_WITH_DB;
                    }
                    break;
                case CLIENT_AUTH_RESPONSE_LENGTH:
                    clie_auth_response_length = buff[i];
                    state_client = CLIENT_AUTH_RESPONSE;
                    break;
                case CLIENT_IF_CLIENT_CONNECT_WITH_DB:
                    std::cout << "stop" << std::endl;
                    break;
                }

                break;
            }
        }
    }  catch (...) {
        std::cerr << "ERROR: int Handshakes::parsePackage(char *buff, int len, int id) - " << errno << std::endl;
        return -1;
    }
    return tmp;
}

void Handshakes::secure_password(std::vector<uint8_t> &pass)
{
    std::vector<uint8_t> hash_pass(SHA_DIGEST_LENGTH);
    std::vector<uint8_t> duble_hash_pass(SHA_DIGEST_LENGTH);
    SHA1(pass.data(), pass.size(), hash_pass.data());
    SHA1(hash_pass.data(), hash_pass.size(), duble_hash_pass.data());
    serv_plugin_data.insert(serv_plugin_data.end(), duble_hash_pass.begin(), duble_hash_pass.end());

    std::vector<uint8_t> hash(SHA_DIGEST_LENGTH);
    SHA1(serv_plugin_data.data(), serv_plugin_data.size(), hash.data());

    std::vector<uint8_t> uv(SHA_DIGEST_LENGTH);
    SHA1(pass.data(), pass.size(), uv.data());
    std::cout << std::endl;
    std::transform(uv.begin(), uv.end(), hash.begin(), hash.end(),
                   [](auto e1, auto e2) {
        return e1^e2;
    });
    for(int j = 0; j < 20; j++)
        std::cout << (0xFF & uv[j]) << ":";
    std::cout << std::endl;
    uv.clear();
    return;
}
