

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
#include "websocket_handler.hpp"
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
using websocket_handler_type = std::string;//std::function<void(void*, staticlib::pion::tcp_connection_ptr&)>;
using handlers_map_type = std::map<std::string, websocket_handler_type>;
using onmessage_handle_type = std::function<void(uint64_t, uint64_t, std::string, std::string, std::string)>;

class ws_view {

//    sl::json::value callbackScript;
public:
    std::string path; // Analog for module /server/views/hi
    std::string method; // ONOPEN, ONCLOSE, ONMESSAGE, ONERROR
    websocket_handler_type handler;
//    std::string renscript_params;
    std::string module;
    std::string function;
//    ws_view(){}
    ws_view(const sl::json::value& json) {
        if (sl::json::type::object != json.json_type()) throw sl::support::exception(TRACEMSG(
                "Invalid 'views' entry: must be an 'object'," +
                " entry: [" + json.dumps() + "]"));
        sl::json::value callbackScript;
        for (const sl::json::field& fi : json.as_object()) {
            auto& name = fi.name();
            if ("method" == name) {
                method = fi.as_string_nonempty_or_throw(name);
            } else if ("path" == name) {
                path = fi.as_string_nonempty_or_throw(name);
            } else if ("callbackScript" == name) {
                wilton::support::check_json_callback_script(fi);
                callbackScript = fi.val().clone();
            } else {
                throw sl::support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
            }
        }

        std::cout << "callback data" << callbackScript.dumps() << std::endl;
        handler = callbackScript.dumps();

//        for (auto fi : callbackScript) {
//            auto& name = fi.name();
//            if ("module" == name) {
//                module = fi.as_string_nonempty_or_throw(name);
//            } else if ("func" == name) {
//                function = fi.as_string_nonempty_or_throw(name);
//            } else {
//                throw sl::support::exception(TRACEMSG("Unknown data field for callback script: [" + name + "]"));
//            }
//        }
    }
};

const std::string on_open = "ONOPEN";
const std::string on_close = "ONCLOSE";
const std::string on_message = "ONMESSAGE";
const std::string on_error = "ONERROR";

template<typename T>
T* choose_map_by_method(const std::string& method, T& onopen_map, T& onclose_map, T& onmessage_map,
        T& onerror_map) {
    if (on_open == method) {
        return &onopen_map;
    } else if (on_close == method) {
        return &onclose_map;
    } else if (on_message == method) {
        return &onmessage_map;
    } else if (on_error == method) {
        return &onerror_map;
    } else {
        throw staticlib::pion::pion_exception("Invalid WebSocket method: [" + method + "]");
    }
}



class wilton_websocket_server : public staticlib::pion::tcp_server
{
    std::map<uint64_t, std::shared_ptr<websocket_handler>> ws_handlers;
//    std::map<std::string, std::thread> message_threads;
    std::set<std::thread> message_threads;
    uint64_t id;
    /*std::function<void(uint64_t, uint64_t, std::string, std::string, std::string)>*/
    onmessage_handle_type recieve_handler;

    void onmessage_handler(std::string resource, std::shared_ptr<websocket_handler> ws, onmessage_handle_type cb_handler){
        // Нужно сделать запуск обработчтка для ws_handler
//        cb_handler(resource);
        std::cout << "onmessage_handler start" << std::endl;
        ws->recieve(resource, cb_handler);
    }
public:

    handlers_map_type ononpen_handlers;
    handlers_map_type onclose_handlers;
    handlers_map_type onmessage_handlers;
    handlers_map_type onerror_handlers;

    struct script_data{
        std::string callback_script; // Analog for module /server/views/hi
        std::string callback_method; // ONOPEN, ONCLOSE, ONMESSAGE
    }; // Хотя вообще это лучше наверное будет в json хранить.
    std::vector<ws_view> views;

    wilton_websocket_server(uint32_t number_of_threads, uint16_t port,
        asio::ip::address_v4 ip_address, std::vector<ws_view> &views, uint64_t id);
    ~wilton_websocket_server() STATICLIB_NOEXCEPT {}

    /**
    * Handles a new TCP connection; derived classes SHOULD override this
    * since the default behavior does nothing
    *
    * @param tcp_conn the new TCP connection to handle
    */
    virtual void handle_connection(staticlib::pion::tcp_connection_ptr& tcp_conn) override;


    void add_handler(const std::string& method, const std::string& resource,
            websocket_handler_type websocket_handler);

    void run_handler(uint64_t wss_id, uint64_t ws_id, std::string method, std::string resource, std::string message);

    uint64_t get_id() const;

    void send(uint64_t ws_id, std::string data);
};



#endif /* WILTON_WEBSOCKET_SERVER_H */

