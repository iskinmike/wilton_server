
#include "staticlib/pion/tcp_server.hpp"
#include "wilton_websocket_server.h"
#include "websocket_handler.hpp"
#include "wilton/wiltoncall.h"

//#include <boost/beast/websocket/ssl.hpp>

#include <vector>
#include <utility>
#include <memory>
#include <chrono>
#include <typeinfo>     // typeid

//std::vector<boost::beast::websocket::stream<asio::ip::tcp::socket>> connections;

//boost::beast::websocket::stream<asio::ip::tcp::socket>* ws_;
//int threads_max;



//void on_read(std::error_code ec, std::size_t bytes_written);
//void do_read();
//void on_accept(std::error_code ec)
//{
//    // Read a message
//    std::cout << "WS on_accept: [" << ec.message() << "]" << std::endl;
//    do_read();
//}

//void do_read() {
//    // Read a message into our buffer
//    boost::beast::multi_buffer cont_buf(1024);
//    ws_->async_read(
//        cont_buf,
//        [&] (std::error_code ec, std::size_t bytes_written)
//            {
//                on_read(ec, bytes_written);
//            }
//    );
//}

//void on_read(std::error_code ec, std::size_t bytes_written){
//    std::cout << "WS on_read: [" << ec.message() << "] [" << bytes_written << "]" << std::endl;
//    do_read();
//}

enum class methods {
    on_open, on_close, on_message, on_error
};

uint64_t wilton_websocket_server::get_id() const
{
    return id;
}

void wilton_websocket_server::send(uint64_t ws_id, string data) {
    std::cout << "ws server send: [" << data <<  "]" << std::endl;
    std::string msg = ws_handlers[ws_id]->gen_frame(WebSocketFrameType::TEXT_FRAME, data);
//    send(msg);
    ws_handlers[ws_id]->send(msg);
}

wilton_websocket_server::wilton_websocket_server(uint32_t number_of_threads, uint16_t port,
                                                 asio::ip::address_v4 ip_address, std::vector<ws_view>& views, uint64_t id) :
    staticlib::pion::tcp_server(asio::ip::tcp::endpoint(ip_address, port), number_of_threads), id(id),
    recieve_handler ([this] (uint64_t wss_id, uint64_t ws_id, std::string func, std::string res, std::string message){
        std::cout << "callback: [" << res << "][" << message<< "][" << wss_id <<"][" << ws_id<<"]" << std::endl;
        this->run_handler(wss_id, ws_id, func, res, message);
    }),
    views(std::move(views))
{
//    std::cout << "views size: [" << this->views.size() <<  "]" << std::endl;
//    std::cout << "views size: [" << views.size() <<  "]" << std::endl;
    for (ws_view& view : this->views) {
        add_handler(view.method, view.path, view.handler);
    }
}

void wilton_websocket_server::handle_connection(staticlib::pion::tcp_connection_ptr &tcp_conn)
{
    std::shared_ptr<websocket_handler> handler = make_shared<websocket_handler>(tcp_conn);
    ws_handlers[handler->get_uniq_id()] = handler;
    handler->set_server_id(id);

    std::string request = handler->recieve_blocking();
    std::string resource = request.substr(request.find("GET")+5 ,request.find("HTTP/1.1")-6);
    if (handler->check_handshake_request(request)) {
        handler->answer_handshake();
//        tcp_conn->set_lifecycle(staticlib::pion::tcp_connection::lifecycle_type::LIFECYCLE_KEEPALIVE);
        run_handler(id, handler->get_uniq_id(), "ONOPEN", resource, std::string{});
        std::thread([this, resource, handler](){
            handler->recieve(resource, this->recieve_handler);
            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            }
        }).detach();
    } else {
        std::cout << "dont understand client request" << std::endl;
    }

}

void wilton_websocket_server::add_handler(const string &method, const string &resource, websocket_handler_type websocket_handler){
    handlers_map_type* map = choose_map_by_method<handlers_map_type>(method, ononpen_handlers, onclose_handlers, onmessage_handlers, onerror_handlers);
    auto it = map->emplace(resource, websocket_handler);
    if (!it.second) {
        throw staticlib::pion::pion_exception("Invalid duplicate handler path: [" + resource + "], method: [" + method + "]");
    }
}

void wilton_websocket_server::run_handler(uint64_t wss_id, uint64_t ws_id, string method, string resource, string message){
    handlers_map_type* map = choose_map_by_method<handlers_map_type>(method, ononpen_handlers, onclose_handlers, onmessage_handlers, onerror_handlers);

    std::string engine("");
    char* out = nullptr;
    int out_len = 0;
    std::string json_in((*map)[resource]);

    sl::json::value json = sl::json::loads(json_in);
    std::vector<sl::json::value> args;
    args.emplace_back(wss_id); // id value
    args.emplace_back(ws_id); // id value
    if (!message.empty()) {
        args.emplace_back(message);
    }
    json.as_object_or_throw().emplace_back("args", std::move(args));
    json_in = json.dumps();
//    std::cout << "new json value: [" << json_in <<  "]" << std::endl;

    auto err = wiltoncall_runscript(engine.c_str(), engine.size(), json_in.c_str(), static_cast<int> (json_in.size()),
                         std::addressof(out), std::addressof(out_len));
    if (nullptr == err) {
        if (nullptr != out) {
            wilton_free(out);
        }
    } else {
        std::string msg = TRACEMSG(err);
        wilton_free(err);
        std::cout << "error run handler: [" << msg << "]" << std::endl;
    }
}
