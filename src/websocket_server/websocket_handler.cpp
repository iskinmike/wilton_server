
#include "websocket_handler.hpp"


//uint64_t gen_uniq_id(){
//    static uint64_t id = 0;
//    return id++;
//}

//uint64_t websocket_handler::get_uniq_id() const
//{
//    return uniq_id;
//}

//void websocket_handler::set_uniq_id(const uint64_t &value)
//{
//    uniq_id = value;
//}

void websocket_handler::set_server_id(const uint64_t &value)
{
    server_id = value;
}

string websocket_handler::gen_frame(WebSocketFrameType type, string msg) {
    return ws.makeFrame(type, msg);
}

websocket_handler::websocket_handler(staticlib::pion::tcp_connection_ptr &tcp_conn)
        : tcp_conn(tcp_conn), ws_data() {
    this->tcp_conn->set_lifecycle(staticlib::pion::tcp_connection::LIFECYCLE_KEEPALIVE);
}

websocket_handler::websocket_handler(staticlib::pion::tcp_connection_ptr &tcp_conn, worker_data ws_data)
        : tcp_conn(tcp_conn), ws_data(ws_data) {
    this->tcp_conn->set_lifecycle(staticlib::pion::tcp_connection::LIFECYCLE_KEEPALIVE);
}

websocket_handler::~websocket_handler(){}

void websocket_handler::send(string msg){
//    std::cout << "ws async send msg: [" << msg << "]" << std::endl;
//    std::cout << "is conn open: [" << tcp_conn->is_open() << "]" << std::endl;
//    std::cout << "is io_service stopped: [" << tcp_conn->get_io_service().stopped() << "]" << std::endl;
//    std::cout << "is conn open: [" << tcp_conn->is_open() << "]" << std::endl;
    tcp_conn->async_write(asio::buffer(msg), [&] (const std::error_code& ec, std::size_t bytes_transferred ) {
        if (ec) {
#ifdef WS_DEBUG
            std::cout << "ws write error: [" << ec.message() << "] [" << bytes_transferred << "]" << std::endl;
#endif
            //                tcp_conn->close();
        }
        std::cout << "ws msg sended: [" << bytes_transferred << "]" << std::endl;
    });
}

void websocket_handler::send_blocking(string msg){
#ifdef WS_DEBUG
    std::cout << "ws send msg: [" << msg << "]" << std::endl;
#endif
    std::error_code ec;
    tcp_conn->write(asio::buffer(msg), ec);
#ifdef WS_DEBUG
    std::cout << "ws send error code: [" << ec.message() << "]" << std::endl;
#endif

}

void websocket_handler::send_data_async(const string &data){
    std::string msg = gen_frame(WebSocketFrameType::TEXT_FRAME, data);
    send(msg);
}

//void websocket_handler::recieve(std::string resource, recieve_handle_type& cb_handle){
//    tcp_conn->async_read_some(asio::buffer(input_data, 1024), [resource, &cb_handle, this] (const std::error_code& ec, std::size_t bytes_transferred) {
//        if (ec) {
//#ifdef WS_DEBUG
//            std::cout << "ws read error: [" << ec.message() << "] [" << bytes_transferred << "] [" << resource << "]" << std::endl;
//#endif
//            return;
//        }
//        std::string data(input_data, bytes_transferred);
//        std::string out_message{};
//        if (!cb_handle) {
//            std::cout << "handler corrupted" << std::endl;
//            return;
//        }

//        WebSocketFrameType type = ws.getFrame(data, out_message);
//        switch (type) {
//        case WebSocketFrameType::ERROR_FRAME:
//            std::cout << "Error frame recieved" << std::endl;
//        case WebSocketFrameType::CLOSING_FRAME:{
//            cb_handle(this->server_id, this->uniq_id, "ONCLOSE", resource, std::string{});
//            this->close();
//            break;
//        }
//        case WebSocketFrameType::TEXT_FRAME:
//        case WebSocketFrameType::BINARY_FRAME:{
//            cb_handle(this->server_id, this->uniq_id,"ONMESSAGE", resource, out_message);
//            this->recieve(resource, cb_handle);
//            break;
//        }
//        default:
//            break;
//        }
////        std::cout << "ws recieve: [" << out_message <<  "] [" << std::hex << static_cast<uint64_t>(type) << "]" << std::endl;
////        std::cout << "frame code: [" << std::hex << static_cast<uint64_t>(type) << "]" << std::endl;
//    });
//}

void websocket_handler::recieve(){
    if (!tcp_conn->is_open()) {
        std::cout << "connection closed" << std::endl;
    }

    tcp_conn->async_read_some(asio::buffer(input_data, 1024), [this] (const std::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            std::cout << "tcp error" << std::endl;
            return;
        }
        std::string recieved_data(input_data, bytes_transferred);
        std::string out_message{};

//        std::cout << "recieve usr data" << std::endl;
//        std::cout << "usr_data:    [" << *(static_cast<int64_t*> (ws_data.user_data)) << "]" << std::endl;

        WebSocketFrameType type = ws.getFrame(recieved_data, out_message);
        switch (type) {
        case WebSocketFrameType::ERROR_FRAME:
            std::cout << "Error frame recieved" << std::endl;
//            ws_data.error_handler(ws_data.error_data, ws_data.user_data);
        case WebSocketFrameType::CLOSING_FRAME:{
            ws_data.close_handler(ws_data.close_data, ws_data.user_data);
            this->close();
            break;
        }
        case WebSocketFrameType::TEXT_FRAME:
        case WebSocketFrameType::BINARY_FRAME:{
            ws_data.message_handler(ws_data.message_data, ws_data.user_data, out_message);
            this->recieve();
            break;
        }
        default:
            break;
        }
    });
}

string websocket_handler::recieve_blocking(){
    std::error_code ec;
    int res = tcp_conn->get_socket().receive(asio::buffer(input_data, 1024), 0, ec);
//#ifdef WS_DEBUG
//    std::cout << "ws recieve errore code result: [" << std::string(input_data, res) << "]" << std::endl;
//#endif
    return std::string(input_data, res);
}

void websocket_handler::answer_handshake(){
    send(ws.answerHandshake());
}

void websocket_handler::close(){
//    send_blocking(gen_frame(WebSocketFrameType::CLOSING_FRAME));
    send(gen_frame(WebSocketFrameType::CLOSING_FRAME, std::string{}));
//    tcp_conn->finish();
    tcp_conn->close();
}

bool websocket_handler::check_handshake_request(const string& msg)
{
    return (WebSocketFrameType::OPENING_FRAME == ws.parseHandshake(msg));
}

uint64_t websocket_handler::current_id = 0;
