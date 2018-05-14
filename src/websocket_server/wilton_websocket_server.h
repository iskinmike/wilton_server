

#ifndef WILTON_WEBSOCKET_SERVER_H
#define WILTON_WEBSOCKET_SERVER_H

#include "staticlib/pion/tcp_server.hpp"
#include "staticlib/pion/pion_exception.hpp"
#include "staticlib/json.hpp"
#include "staticlib/support.hpp"
#include "staticlib/utils.hpp"

#include "wilton/support/misc.hpp"

//#include <boost/beast/core.hpp>
//#include <boost/beast/websocket.hpp>
//#include "websocket_handler.hpp"
#include "websocket_worker.hpp"
#include <functional>
#include <map>
#include <set>
#include <thread>
//#include <boost/asio/ip/tcp.hpp>

// enum class server_regime {
//     socket_regime, websocket_regime
// };

//namespace websocket = boost::beast::websocket;

// handler type
//using websocket_handler_type = std::string;//std::function<void(void*, staticlib::pion::tcp_connection_ptr&)>;
//using handlers_map_type = std::map<std::string, websocket_handler_type>;
//using onmessage_handle_type = std::function<void(uint64_t, uint64_t, std::string, std::string, std::string)>;

//using websocket_handler_type = std::function<void(ws_worker*)>;

template <typename handler_type>
class ws_view {

//    sl::json::value callbackScript;
public:
    std::string path; // Analog for module /server/views/hi
    std::string method; // ONOPEN, ONCLOSE, ONMESSAGE, ONERROR
    sl::json::value json_data;
    sl::json::value* json_data_ptr;
    bool is_setted;

    ws_view(): json_data_ptr(nullptr),is_setted(false) {}

    ws_view(const ws_view& view) {
        path     = view.path; 
        method   = view.method; 
        json_data  = view.json_data.clone();
        json_data_ptr = &json_data;
    }

    ws_view& operator =(const ws_view& view) {
        this->path     = view.path;
        this->method   = view.method;
//        this->data     = view.data;
//        this->module   = view.module;
//        this->function = view.function;
        this->json_data  = view.json_data.clone();
        this->json_data_ptr = &(this->json_data);
        this->is_setted = view.is_setted;
        return *this;
    }

    void setup_values(const sl::json::value& json){
        if (sl::json::type::object != json.json_type()) throw sl::support::exception(TRACEMSG(
                "Invalid 'views' entry: must be an 'object'," +
                " entry: [" + json.dumps() + "]"));
//        sl::json::value callbackScript;
        for (const sl::json::field& fi : json.as_object()) {
            auto& name = fi.name();
            if ("method" == name) {
                method = fi.as_string_nonempty_or_throw(name);
            } else if ("path" == name) {
                path = fi.as_string_nonempty_or_throw(name);
            } else if ("callbackScript" == name) {
                wilton::support::check_json_callback_script(fi);
                json_data = fi.val().clone();
                json_data_ptr = &json_data;
            } else {
                throw sl::support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
            }
        }
        is_setted = true;
//        std::cout << "setup_values json_data: [" <<  json_data.dumps() << "]" << std::endl;
    }
};

typedef  std::vector<std::shared_ptr<websocket_worker_data>> old_ws_handler_type;

class ws_views {
public:
    ws_view<onopen_handler> onopen_view;
    ws_view<onclose_handler> onclose_view;
    ws_view<onerror_handler> onerror_view;
    ws_view<onmessage_handler> onmessage_view;

    ws_views() {}
    ws_views(const ws_views& views) {
        onopen_view    = views.onopen_view;
        onclose_view   = views.onclose_view;
        onerror_view   = views.onerror_view;
        onmessage_view = views.onmessage_view;
    }

};

//const std::string on_open = "ONOPEN";
//const std::string on_close = "ONCLOSE";
//const std::string on_message = "ONMESSAGE";
//const std::string on_error = "ONERROR";

//template<typename T>
//T* choose_map_by_method(const std::string& method, T& onopen_map, T& onclose_map, T& onmessage_map,
//        T& onerror_map) {
//    if (on_open == method) {
//        return &onopen_map;
//    } else if (on_close == method) {
//        return &onclose_map;
//    } else if (on_message == method) {
//        return &onmessage_map;
//    } else if (on_error == method) {
//        return &onerror_map;
//    } else {
//        throw staticlib::pion::pion_exception("Invalid WebSocket method: [" + method + "]");
//    }
//}



class wilton_websocket_server : public staticlib::pion::tcp_server
{
//    std::map<uint64_t, std::shared_ptr<websocket_handler>> ws_handlers;
//    std::vector<std::shared_ptr<worker_data>> paths;
//    std::vector<ws_views> views;

    std::map<std::string, websocket_worker_data> websocket_data_storage;

public:

    wilton_websocket_server(uint32_t number_of_threads, uint16_t port,
        asio::ip::address_v4 ip_address/*, std::vector<std::shared_ptr<worker_data>> paths, std::vector<ws_views> views*/);
    ~wilton_websocket_server() STATICLIB_NOEXCEPT {}

    /**
    * Handles a new TCP connection; derived classes SHOULD override this
    * since the default behavior does nothing
    *
    * @param tcp_conn the new TCP connection to handle
    */
    virtual void handle_connection(staticlib::pion::tcp_connection_ptr& tcp_conn) override;

    void add_websocket_handler(const std::string& resource, websocket_worker_data websocket_handler_data){
        websocket_data_storage.emplace(resource, websocket_handler_data);
    }
};



#endif /* WILTON_WEBSOCKET_SERVER_H */

