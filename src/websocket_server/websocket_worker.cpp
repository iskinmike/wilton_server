

#include "websocket_worker.hpp"

websocket_worker::websocket_worker(staticlib::pion::tcp_connection_ptr tcp_conn, websocket_worker_data ws_data)
    : ws_handler(tcp_conn), ws_data(ws_data){

    async_recieve_handler_type recieve_handler = [this] (const std::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            std::cout << "tcp error" << std::endl;
            return;
        }
        std::string recieved_data = this->ws_handler.get_recieved_data(bytes_transferred);
        std::string out_message{};

        WebSocketFrameType type = this->ws_handler.get_message_frame(recieved_data, out_message);
        switch (type) {
        case WebSocketFrameType::INCOMPLETE_FRAME:
        case WebSocketFrameType::INCOMPLETE_BINARY_FRAME:
        case WebSocketFrameType::INCOMPLETE_TEXT_FRAME:
        case WebSocketFrameType::ERROR_FRAME:
            std::cout << "Error frame recieved" << std::endl;
            this->run_error_handler();
        case WebSocketFrameType::CLOSING_FRAME:{
            this->run_close_handler();
            this->ws_handler.close();
            break;
        }
        case WebSocketFrameType::TEXT_FRAME:
        case WebSocketFrameType::BINARY_FRAME:{
            this->run_message_handler(out_message);
            this->ws_handler.recieve();
            break;
        }
        default:
            break;
        }
    };
    async_send_handler_type send_handler = [this] (const std::error_code& ec, std::size_t bytes_transferred ) {
        if (ec) {
        }
        std::cout << "ws msg sended: [" << bytes_transferred << "]" << std::endl;
    };

    ws_handler.setup_async_recieve_handler(recieve_handler);
    ws_handler.setup_async_send_handler(send_handler);
}

void websocket_worker::start_with_message(string message){
    ws_data.websocket_prepare_handler(this);
    // Сначала нужно сделать парсинг сообщения чтобы WebSocket бибилотека записала получила данные запроса
    bool res = ws_handler.check_handshake_request(message);
    if (res) {
        // если это handshake сообщение - тогда мы начинаем работать
        ws_handler.answer_handshake();
        // И нужно запустить onconnect метод:
        //            std::cout << "start usr data" << std::endl;
        //            std::cout << "usr_data:    [" << *(static_cast<int64_t*> (ws_data.user_data)) << "]" << std::endl;

        //            ws_data.open_handler(ws_data.open_data, ws_data.user_data);
        run_open_handler();
        // Теперь нужно запустить чтение данных из сокета
        std::thread([this](){
            this->ws_handler.recieve();
        }).detach();
    }
}

void websocket_worker::setup_data(const websocket_worker_data &input_data){
    ws_data = input_data;
}

void websocket_worker::setup_user_data(void *data) {
    ws_data.user_data = data;
}

void websocket_worker::stop(){
    ws_handler.close();
}

void websocket_worker::send(string message){
    std::string frame = ws_handler.gen_frame(WebSocketFrameType::TEXT_FRAME, message);
    ws_handler.send(frame);
}

void websocket_worker::run_open_handler(){
    if (nullptr != ws_data.open_handler)
        ws_data.open_handler(ws_data.open_data, ws_data.user_data);
}

void websocket_worker::run_error_handler(){
    if (nullptr != ws_data.error_handler)
        ws_data.error_handler(ws_data.error_data, ws_data.user_data);
}

void websocket_worker::run_close_handler(){
    if (nullptr != ws_data.close_handler)
        ws_data.close_handler(ws_data.close_data, ws_data.user_data);
}

void websocket_worker::run_message_handler(string &out_message){
    if (nullptr != ws_data.message_handler)
        ws_data.message_handler(ws_data.message_data, ws_data.user_data, out_message);
}
