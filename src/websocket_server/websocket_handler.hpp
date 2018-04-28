
#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H


#include <string>
#include <vector>
#include "web_socket/WebSocket.h"
#include "staticlib/pion/tcp_server.hpp"
#include "asio.hpp"

#define WS_DEBUG

#ifdef WS_DEBUG
#include <iostream>
#endif

using recieve_handle_type = std::function<void(uint64_t, uint64_t, std::string,std::string,std::string)>;

//namespace {
//    uint64_t gen_uniq_id();
//}

class websocket_handler
{
    WebSocket ws;
    staticlib::pion::tcp_connection_ptr tcp_conn; // как-то надо ссылку хранить либо хранить самим указатель.
//    std::string input_data;
    char input_data[1024];

    uint64_t uniq_id;
    uint64_t server_id;
    static uint64_t current_id;


public:
    std::string gen_frame(WebSocketFrameType type, std::string msg = std::string{});
    websocket_handler(staticlib::pion::tcp_connection_ptr& tcp_conn);
    ~websocket_handler();

    // отправить сообщение
//    void send(std::vector<unsigned char> msg);
    void send(std::string msg);
    void send_blocking(std::string msg);

    void send_data_async(const std::string& data);

    // принять данные TODO, надо понять как с этим взаимодействовать
    // скорее всего просто внутри хапускать обращение к сокету типа async_recieve
    void recieve(string resource, recieve_handle_type &cb_handle);
    std::string recieve_blocking();
    // ответить на запрос подключения
    void answer_handshake();
    // закрыть соединение. Точнее послать сообщение с фреймом закрыть.
    void close();
    // дождаться соединения

    // всякие статические методы - типа проверить сообщение
    bool check_handshake_request(const string &msg);
    uint64_t get_uniq_id() const;
    void set_uniq_id(const uint64_t &value);
    void set_server_id(const uint64_t &value);
};


#endif /* WEBSOCKET_HANDLER_H */

