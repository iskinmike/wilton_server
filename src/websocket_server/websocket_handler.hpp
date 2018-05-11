
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

using basic_handler_type = std::function<void(void*, void* user_data)>; // only data
using onmessage_handler = std::function<void(void*, void* user_data, std::string)>; // data, message

class ws_worker;
using real_websocket_handler_type = std::function<void(ws_worker*)>;
//using onopen_handler = std::function<void(void*, void* user_data)>; // only data
typedef basic_handler_type onopen_handler;
typedef basic_handler_type onclose_handler;
typedef basic_handler_type onerror_handler;

//namespace {
//    uint64_t gen_uniq_id();
//}
struct worker_data {
    onmessage_handler message_handler;
    onopen_handler    open_handler;
    onclose_handler   close_handler;
    onerror_handler   error_handler;

    real_websocket_handler_type websocket_handler;

    void* message_data;
    void* open_data;
    void* close_data;
    void* error_data;

    std::string resource;
    void* user_data;

    worker_data(const worker_data& data){ // copy constructor
        this->message_handler = data.message_handler;
        this->open_handler    = data.open_handler;
        this->close_handler   = data.close_handler;
        this->error_handler   = data.error_handler;
        this->message_data    = data.message_data;
        this->open_data       = data.open_data;
        this->close_data      = data.close_data;
        this->error_data      = data.error_data;
        this->resource        = data.resource;
        this->user_data       = data.user_data;
        this->websocket_handler       = data.websocket_handler;
    }

    worker_data& operator =(const worker_data& data){
        this->message_handler = data.message_handler;
        this->open_handler    = data.open_handler;
        this->close_handler   = data.close_handler;
        this->error_handler   = data.error_handler;
        this->message_data    = data.message_data;
        this->open_data       = data.open_data;
        this->close_data      = data.close_data;
        this->error_data      = data.error_data;
        this->resource        = data.resource;
        this->user_data       = data.user_data;
        this->websocket_handler       = data.websocket_handler;
        return *this;
    }

    worker_data(){}
    // Деструктор не нужен все это будет убиваться когда умрет сервер, который это запустил
};

class websocket_handler
{
    WebSocket ws;
    staticlib::pion::tcp_connection_ptr tcp_conn; // как-то надо ссылку хранить либо хранить самим указатель.
//    std::string input_data;
    char input_data[1024];

    uint64_t server_id;
    static uint64_t current_id;


public:
    worker_data ws_data;
    websocket_handler(staticlib::pion::tcp_connection_ptr& tcp_conn);
    websocket_handler(staticlib::pion::tcp_connection_ptr& tcp_conn, worker_data ws_data);
    ~websocket_handler();
    std::string gen_frame(WebSocketFrameType type, std::string msg = std::string{});

    // отправить сообщение
//    void send(std::vector<unsigned char> msg);
    void send(std::string msg);
    void send_blocking(std::string msg);

    void send_data_async(const std::string& data);

    // принять данные TODO, надо понять как с этим взаимодействовать
    // скорее всего просто внутри хапускать обращение к сокету типа async_recieve
//    void recieve(string resource, recieve_handle_type &cb_handle);
    // 
    // void recieve(void* data, onmessage_handler &cb_handle);
    void recieve();

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



class ws_worker {
    websocket_handler ws_handler;
    // Задаем лбработчики для событий
    worker_data ws_data;

public:
    // Создаем воркер который будет обрабатывать запросы через websocket так как это
    // Сейчас реализовано в wilton_websocket_server
    // Нужно задать для него хэндлеры и данные 
    ws_worker(staticlib::pion::tcp_connection_ptr tcp_conn, worker_data ws_data)
    : ws_handler(tcp_conn, ws_data), ws_data(ws_data){

//        std::cout << "ws_handler.ws_data.open_data:    [" << static_cast<char*>(ws_handler.ws_data.open_data)    << "]" << std::endl;
//        std::cout << "ws_handler.ws_data.close_data:   [" << static_cast<char*>(ws_handler.ws_data.close_data)   << "]" << std::endl;
//        std::cout << "ws_handler.ws_data.error_data:   [" << static_cast<char*>(ws_handler.ws_data.error_data)   << "]" << std::endl;
//        std::cout << "ws_handler.ws_data.message_data: [" << static_cast<char*>(ws_handler.ws_data.message_data) << "]" << std::endl;

//        std::cout << "ws_worker.ws_data.open_data:    [" << static_cast<char*>(this->ws_data.open_data)    << "]" << std::endl;
//        std::cout << "ws_worker.ws_data.close_data:   [" << static_cast<char*>(this->ws_data.close_data)   << "]" << std::endl;
//        std::cout << "ws_worker.ws_data.error_data:   [" << static_cast<char*>(this->ws_data.error_data)   << "]" << std::endl;
//        std::cout << "ws_worker.ws_data.message_data: [" << static_cast<char*>(this->ws_data.message_data) << "]" << std::endl;

//        std::cout << "ws_handler.ws_data.usr_data:    [" << *(static_cast<int64_t*> (ws_handler.ws_data.user_data)) << "]" << std::endl;
//        std::cout << "ws_worker.ws_data.usr_data:    [" << *(static_cast<int64_t*> (this->ws_data.user_data)) << "]" << std::endl;
    }

    void start_with_message(std::string message){
        ws_data.websocket_handler(this);
        // Сначала нужно сделать парсинг сообщения чтобы WebSocket бибилотека записала получила данные запроса
        bool res = ws_handler.check_handshake_request(message);

//        std::cout << "open_data:    [" << static_cast<char*>(ws_data.open_data)    << "]" << std::endl;
//        std::cout << "close_data:   [" << static_cast<char*>(ws_data.close_data)   << "]" << std::endl;
//        std::cout << "error_data:   [" << static_cast<char*>(ws_data.error_data)   << "]" << std::endl;
//        std::cout << "message_data: [" << static_cast<char*>(ws_data.message_data) << "]" << std::endl;

        if (res) {
            // если это handshake сообщение - тогда мы начинаем работать
            ws_handler.answer_handshake();
            // И нужно запустить onconnect метод:
            std::cout << "start usr data" << std::endl;
            std::cout << "usr_data:    [" << *(static_cast<int64_t*> (ws_data.user_data)) << "]" << std::endl;

            ws_data.open_handler(ws_data.open_data, ws_data.user_data);
            // Теперь нужно запустить чтение данных из сокета
            std::thread([this](){
                this->ws_handler.recieve();
            }).detach();
        }
    }

    void setup_data(const worker_data& input_data){
        ws_data = input_data;
    }

    void setup_user_data(void* data) {
        ws_data.user_data = data;
        ws_handler.ws_data.user_data = data;
    }

    worker_data* get_data(){
        return &ws_data;
    }

    int stop(){
        int i = 0;
        i++;
        return i;
    }

    void send(std::string message){
        std::string frame = ws_handler.gen_frame(WebSocketFrameType::TEXT_FRAME, message);
        ws_handler.send(frame);
    }
};


#endif /* WEBSOCKET_HANDLER_H */

