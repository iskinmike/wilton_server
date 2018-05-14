
#include "websocket_handler.hpp"

string websocket_handler::gen_frame(WebSocketFrameType type, string msg) {
    return ws.makeFrame(type, msg);
}

websocket_handler::websocket_handler(staticlib::pion::tcp_connection_ptr &tcp_conn)
        : tcp_conn(tcp_conn){
    this->tcp_conn->set_lifecycle(staticlib::pion::tcp_connection::LIFECYCLE_KEEPALIVE);
}

websocket_handler::~websocket_handler(){}

void websocket_handler::send(string msg){
    tcp_conn->async_write(asio::buffer(msg), async_send_handler);
}

void websocket_handler::send_blocking(string msg){
    std::error_code ec;
    tcp_conn->write(asio::buffer(msg), ec);
}

void websocket_handler::recieve(){
    if (!tcp_conn->is_open()) {
        std::cout << "connection closed" << std::endl;
    }
    tcp_conn->async_read_some(asio::buffer(input_data, 1024), async_recieve_handler);
}

string websocket_handler::recieve_blocking(){
    std::error_code ec;
    int res = tcp_conn->get_socket().receive(asio::buffer(input_data, 1024), 0, ec);
    return std::string(input_data, res);
}

string websocket_handler::get_recieved_data(size_t size){
    return std::string{input_data, size};
}

WebSocketFrameType websocket_handler::get_message_frame(string message, string &out_message){
    return ws.getFrame(message, out_message);
}

void websocket_handler::answer_handshake(){
    send(ws.answerHandshake());
}

void websocket_handler::close(){
    send(gen_frame(WebSocketFrameType::CLOSING_FRAME, std::string{}));
//    tcp_conn->finish();
    tcp_conn->close();
}

bool websocket_handler::check_handshake_request(const string& msg)
{
    return (WebSocketFrameType::OPENING_FRAME == ws.parseHandshake(msg));
}

void websocket_handler::setup_async_send_handler(async_send_handler_type handler){
    async_send_handler = handler;
}

void websocket_handler::setup_async_recieve_handler(async_recieve_handler_type handler){
    async_recieve_handler = handler;
}
