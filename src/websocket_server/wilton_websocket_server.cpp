
#include "staticlib/pion/tcp_server.hpp"
#include "wilton_websocket_server.h"
#include "websocket_handler.hpp"
#include "wilton/wiltoncall.h"


#include <vector>
#include <utility>
#include <memory>
#include <chrono>

enum class methods {
    on_open, on_close, on_message, on_error
};

//void wilton_websocket_server::send(uint64_t ws_id, string data) {
//    std::cout << "ws server send: [" << data <<  "]" << std::endl;
//    std::string msg = ws_handlers[ws_id]->gen_frame(WebSocketFrameType::TEXT_FRAME, data);
////    send(msg);
//    ws_handlers[ws_id]->send(msg);
//}

wilton_websocket_server::wilton_websocket_server(uint32_t number_of_threads, uint16_t port,
                                                 asio::ip::address_v4 ip_address/*, std::vector<std::shared_ptr<worker_data>> paths, std::vector<ws_views> views*/) :
    staticlib::pion::tcp_server(asio::ip::tcp::endpoint(ip_address, port), number_of_threads)/*,
    recieve_handler ([this] (uint64_t wss_id, uint64_t ws_id, std::string func, std::string res, std::string message){
        std::cout << "callback: [" << res << "][" << message<< "][" << wss_id <<"][" << ws_id<<"]" << std::endl;
        this->run_handler(wss_id, ws_id, func, res, message);
    })*/
//  , paths(std::move(paths)), views(std::move(views))
{
//    std::cout << "views size: [" << this->views.size() <<  "]" << std::endl;
//    std::cout << "views size: [" << views.size() <<  "]" << std::endl;

//    for (worker_data& data : this->views) {
//        add_handler(vievieww.method, view.path, view.handler);
//        add_websocket_handler(data.resource, data);
//    }

//    for (ws_view<websocket_handler_type>& view : this->views) {
//        add_handler(view.method, view.path, view.handler);
//    }
//    paths = create_websocket_paths(real_views);
//    for (auto& path: paths){
//        this->add_websocket_handler(path->resource, *(path.get()));
//    }
}


//std::vector<std::shared_ptr<ws_worker>> storage;
void wilton_websocket_server::handle_connection(staticlib::pion::tcp_connection_ptr &tcp_conn)
{

    // На самом деле здесь нужно получить данные из tcp соединения разобрать их и если
    std::error_code ec;
    char input_data[1024];
    int res = tcp_conn->get_socket().receive(asio::buffer(input_data, 1024), 0, ec);
    std::string message(input_data, res);
    // далее необходимо узнать к какому ресурсу идет запрос
    std::string resource = message.substr(message.find("GET")+5, message.find("HTTP/1.1")-6);
    // И если это запрос на работу с вебсокетами создать ws_worker
    // При этом засунув его в специальный контейнер
    websocket_worker_data proxy_ws_data = websocket_data_storage[resource];
    websocket_worker* tmp  = new websocket_worker(tcp_conn, proxy_ws_data);
//    websocket_worker_storage[resource] = tmp;// Нуно по другому так работать не будет
    // Теперь нужно запустить prepare handler
//    auto prepare_handler = websocket_prepare_handler_storage[resource];
//    if (prepare_handler == nullptr) {
//        std::cout << "wrong prepare_handler for " << resource << std::endl;
//        std::cout << "websocket_prepare_handler_storage.size(): "
//                  << websocket_prepare_handler_storage.size() << std::endl;
//        for (auto& el : websocket_prepare_handler_storage) {
//            std::cout << "el.first: " << el.first << std::endl;
//            std::cout << "el.second is nul: " << (el.second == nullptr) << std::endl;
//        }
//    } else {
//        prepare_handler(tmp);
        // Затем запустить его передав туда сообщение
//        storage.push_back(tmp);
        tmp->start_with_message(message);
//    }
}

//void wilton_websocket_server::add_handler(const string &method, const string &resource, websocket_handler_type websocket_handler){
//    handlers_map_type* map = choose_map_by_method<handlers_map_type>(method, ononpen_handlers, onclose_handlers, onmessage_handlers, onerror_handlers);
//    auto it = map->emplace(resource, websocket_handler);
//    if (!it.second) {
//        throw staticlib::pion::pion_exception("Invalid duplicate handler path: [" + resource + "], method: [" + method + "]");
//    }
//}

//void wilton_websocket_server::run_handler(uint64_t wss_id, uint64_t ws_id, string method, string resource, string message){
//    handlers_map_type* map = choose_map_by_method<handlers_map_type>(method, ononpen_handlers, onclose_handlers, onmessage_handlers, onerror_handlers);

//    std::string engine("");
//    char* out = nullptr;
//    int out_len = 0;
//    std::string json_in((*map)[resource]);

//    sl::json::value json = sl::json::loads(json_in);
//    std::vector<sl::json::value> args;
//    args.emplace_back(wss_id); // id value
//    args.emplace_back(ws_id); // id value
//    if (!message.empty()) {
//        args.emplace_back(message);
//    }
//    json.as_object_or_throw().emplace_back("args", std::move(args));
//    json_in = json.dumps();
////    std::cout << "new json value: [" << json_in <<  "]" << std::endl;

//    auto err = wiltoncall_runscript(engine.c_str(), engine.size(), json_in.c_str(), static_cast<int> (json_in.size()),
//                         std::addressof(out), std::addressof(out_len));
//    if (nullptr == err) {
//        if (nullptr != out) {
//            wilton_free(out);
//        }
//    } else {
//        std::string msg = TRACEMSG(err);
//        wilton_free(err);
//        std::cout << "error run handler: [" << msg << "]" << std::endl;
//    }
//}
