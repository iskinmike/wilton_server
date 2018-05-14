
#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H


#include <string>
#include <vector>
#include "web_socket/WebSocket.h"
#include "staticlib/pion/tcp_server.hpp"
#include "asio.hpp"

//#define WS_DEBUG
//#ifdef WS_DEBUG
#include <iostream>
//#endif

using async_recieve_handler_type = std::function<void(const std::error_code&, std::size_t)>;
using async_send_handler_type = std::function<void(const std::error_code&, std::size_t)>;

class websocket_handler
{
    WebSocket ws;
    staticlib::pion::tcp_connection_ptr tcp_conn; // как-то надо ссылку хранить либо хранить самим указатель.
    char input_data[1024];

    async_send_handler_type async_send_handler;
    async_recieve_handler_type async_recieve_handler;
public:
    websocket_handler(staticlib::pion::tcp_connection_ptr& tcp_conn);
    ~websocket_handler();
    std::string gen_frame(WebSocketFrameType type, std::string msg = std::string{});

    // отправить сообщение
    void send(std::string msg);
    void send_blocking(std::string msg);

//    void send_data_async(const std::string& data);

    // принять данные TODO, надо понять как с этим взаимодействовать
    // скорее всего просто внутри хапускать обращение к сокету типа async_recieve
    void recieve();
    std::string recieve_blocking();

    std::string get_recieved_data(std::size_t size);
    WebSocketFrameType get_message_frame(std::string message, std::string& out_message);

    // ответить на запрос подключения
    void answer_handshake();
    // закрыть соединение. Точнее послать сообщение с фреймом закрыть.
    void close();
    // дождаться соединения

    // всякие статические методы - типа проверить сообщение
    bool check_handshake_request(const string &msg);

    void setup_async_send_handler(async_send_handler_type handler);
    void setup_async_recieve_handler(async_recieve_handler_type handler);
};

// ===========================================================================================



#endif /* WEBSOCKET_HANDLER_H */

